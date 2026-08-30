#include "win2.h"
#include "dataEvent.h"
#include "timingOperators.h"
#include "win2_pkg.h"

#include <algorithm>
#include <cstdint>
#include <string>

using namespace SST;

namespace {

const char *portName(uint32_t port) {
  switch (port) {
  case 0:
    return "input";
  case 1:
    return "offset";
  default:
    return "?";
  }
}

} // namespace

Win2::Win2(SST::ComponentId_t id, SST::Params &params)
    : DRRAResource(id, params) {
  // BULK_BITWIDTH is emitted verbatim (uppercase); lowercase is never emitted.
  bulk_bitwidth = params.find<size_t>(
      "BULK_BITWIDTH", params.find<size_t>("bulk_bitwidth", 256));
  word_bitwidth = params.find<size_t>("word_bitwidth", 16);

  if (word_bitwidth == 0) {
    out.fatal(CALL_INFO, -1, "word_bitwidth must be > 0\n");
  }
  if (bulk_bitwidth == 0 || (bulk_bitwidth % word_bitwidth) != 0) {
    out.fatal(CALL_INFO, -1,
              "bulk_bitwidth (%zu) must be a positive multiple of "
              "word_bitwidth (%zu)\n",
              bulk_bitwidth, word_bitwidth);
  }

  bulk_bytes = (bulk_bitwidth + 7) / 8;
  word_bytes = (word_bitwidth + 7) / 8;
  num_lanes  = bulk_bitwidth / word_bitwidth;

  line_A.assign(bulk_bytes, 0);
  line_B.assign(bulk_bytes, 0);

  instructionHandlers = WIN2_PKG::createInstructionHandlers(this);
}

bool Win2::clockTick(SST::Cycle_t currentCycle) {
  if (!portsToActivate.empty() && currentCycle % 10 == 0) {
    for (const auto &[sid, ports] : portsToActivate) {
      activatePortsForSlot(sid, ports);
    }
    portsToActivate.clear();
  }

  // Latch the bulk input wire into the held input register before the input
  // AGU event (absorbAndShift) samples it.
  receiveDataInputs();

  // Sample phase (sub-7): the AGU-gated state updates run inside the base
  // clockTick -- input absorbAndShift and offset latch, both at priority 7 (see
  // handleEVT). The SWB routed this cycle's bulk input at the Route phase
  // (sub-5), so absorb samples the current value.
  bool result = DRRAResource::clockTick(currentCycle);

  // Emit the aligned slice at the Sample phase (sub-7), AFTER absorb/offset have
  // updated the window above. The slice is a registered output (win2.sv.j2's
  // registered bulk_data_out_0): driveOutput defers to the sub-9 commit, so it
  // appears one cycle later.
  if (currentCycle % 10 == 7) {
    emitSlice();
  }

  return result;
}

void Win2::handleActivation(uint32_t slot_id, uint32_t ports) {
  portsToActivate[slot_id] = ports;
}

void Win2::absorbAndShift() {
  // Sample the held bulk input wire (latched by receiveDataInputs()). The held
  // register naturally holds the value on the wire this cycle, replacing the
  // old drain-the-FIFO-for-freshest-payload hack.
  const std::vector<uint8_t> &in = readInput(0, PortChannel::BULK).data;

  line_A = line_B;
  line_B.assign(bulk_bytes, 0);
  size_t take = std::min(in.size(), bulk_bytes);
  std::copy(in.begin(), in.begin() + take, line_B.begin());

  out.output(" WIN2 shift buffer: line_A<-line_B, line_B<-bulk_in (data=%s)\n",
             formatRawDataToWords(in).c_str());
  logTraceEvent("win2_buffer_shift", slot_id, true, 'X',
                {{"line_A", formatRawDataToWords(line_A)},
                 {"line_B", formatRawDataToWords(line_B)}});
}

void Win2::latchOffset() {
  // Sticky offset register: read the offset AGU's address for the current
  // active cycle and latch its low log2(NUM_LANES) bits. Called only on
  // offset-AGU fires; between fires offset_reg holds its last value.
  int64_t addr =
      agus[PORT_OFFSET].getAddressForCycle(getPortActiveCycle(PORT_OFFSET));
  if (addr >= 0) {
    offset_reg = static_cast<uint32_t>(addr) &
                 static_cast<uint32_t>(num_lanes - 1);
    out.output(" WIN2 latch offset_reg=%u\n", offset_reg);
  }
}

void Win2::emitSlice() {
  std::vector<uint8_t> payload = takeAlignedSlice(offset_reg);

  // Registered output: driveOutput defers to the clock-edge commit
  // (commitOutputRegisters at sub-9), so the slice computed from this cycle's
  // window appears one cycle later, matching the registered bulk_data_out_0 in
  // win2.sv.j2.
  driveOutput(0, PortChannel::BULK, payload, bulk_bitwidth, /*registered=*/true);

  out.output(" WIN2 emit aligned slice (offset=%u, data=%s)\n", offset_reg,
             formatRawDataToWords(payload).c_str());
  logTraceEvent("win2_bulk_output", slot_id, true, 'X',
                {{"offset", static_cast<int>(offset_reg)},
                 {"data", formatRawDataToWords(payload)}});
}

std::vector<uint8_t> Win2::takeAlignedSlice(uint32_t offset) const {
  // window[off*WORD_BITWIDTH +: BULK_BITWIDTH], where window = {line_B, line_A}
  // and line_A is at the LSB half. Equivalently: a byte vector of length
  // 2*bulk_bytes formed by line_A followed by line_B, then the slice starting
  // at `offset` lanes (= offset*word_bitwidth bits) inclusive of bulk_bitwidth
  // bits.
  std::vector<uint8_t> window;
  window.reserve(2 * bulk_bytes);
  window.insert(window.end(), line_A.begin(), line_A.end());
  window.insert(window.end(), line_B.begin(), line_B.end());

  std::vector<uint8_t> slice(bulk_bytes, 0);

  // Bit-accurate slice for word_bitwidth values that are not byte-aligned.
  // Common DRRA configuration has word_bitwidth a multiple of 8, but we keep
  // this general.
  size_t start_bit = static_cast<size_t>(offset) * word_bitwidth;
  for (size_t b = 0; b < bulk_bitwidth; b++) {
    size_t src_bit = start_bit + b;
    size_t src_byte = src_bit >> 3;
    size_t src_off  = src_bit & 7u;
    if (src_byte >= window.size())
      break;
    uint8_t bit = (window[src_byte] >> src_off) & 1u;
    slice[b >> 3] |= static_cast<uint8_t>(bit << (b & 7u));
  }
  return slice;
}

void Win2::handleEVT(const WIN2_PKG::EVTInstruction &instr) {
  out.output(
      "evt (slot=%d, port=%s, option=%d, init_addr_sd=%d, init_addr=%d)\n",
      instr.slot, portName(instr.port), instr.option, instr.init_addr_sd,
      instr.init_addr);

  if (instr.port >= 2) {
    out.fatal(CALL_INFO, -1, "Invalid WIN2 EVT port: %d\n", instr.port);
  }

  std::string event_name =
      std::string("win2_") + portName(instr.port) + "_" +
      std::to_string(current_event_number);

  // Install the AGU-gated work as the event lambda. The base class fires
  // these only on cycles where the AGU has a valid address (mirroring the
  // RTL's agu_valid). The per-lane initial address is attached to this
  // event so chained lanes each keep their own base address. Priority 7 places
  // the window/offset update at the Sample phase (sub-7), after the SWB routed
  // this cycle's bulk input (sub-5) and before emitSlice (which runs after the
  // base clockTick in the same sub-7 tick, reading the just-updated window).
  switch (instr.port) {
  case PORT_INPUT:
    agus[PORT_INPUT].addEvent(
        event_name, [this] { absorbAndShift(); }, 7, instr.init_addr);
    break;
  case PORT_OFFSET:
    agus[PORT_OFFSET].addEvent(
        event_name, [this] { latchOffset(); }, 7, instr.init_addr);
    break;
  default:
    out.fatal(CALL_INFO, -1, "Invalid WIN2 EVT port: %d\n", instr.port);
  }

  current_event_number++;
}

void Win2::handleCONF(const WIN2_PKG::CONFInstruction &instr) {
  // win2 has no stored configuration; conf is accepted and ignored.
  out.output("conf (slot=%d)\n", instr.slot);
}

void Win2::handleREP(const WIN2_PKG::REPInstruction &instr) {
  out.output("rep (slot=%d, ext=%d, port=%s, iter=%d, step=%d, delay=%d)\n",
             instr.slot, instr.ext, portName(instr.port), instr.iter,
             instr.step, instr.delay);
  if (instr.port >= 2) {
    out.fatal(CALL_INFO, -1, "Invalid WIN2 REP port: %d\n", instr.port);
  }
  try {
    if (!instr.ext) {
      // base: add a new repetition (low half of iter/step/delay)
      agus[instr.port].addRepetition(instr.iter, instr.delay, instr.step);
    } else {
      // extension: fold the high bits into the last repetition
      auto repetition_op = agus[instr.port].getLastRepetitionOperator();
      uint32_t iter = instr.iter << WIN2_PKG::WIN2_INSTR_REP_ITER_BITWIDTH |
                      repetition_op.getIterations();
      uint32_t step = instr.step << WIN2_PKG::WIN2_INSTR_REP_STEP_BITWIDTH |
                      repetition_op.getStep();
      uint32_t delay = instr.delay << WIN2_PKG::WIN2_INSTR_REP_DELAY_BITWIDTH |
                       repetition_op.getDelay();
      agus[instr.port].adjustRepetition(iter, delay, step);
    }
  } catch (const std::exception &e) {
    out.fatal(CALL_INFO, -1, "WIN2 REP failed: %s\n", e.what());
  }
}

void Win2::handleTRANS(const WIN2_PKG::TRANSInstruction &instr) {
  out.output("trans (slot=%d, port=%s, delay=%d)\n", instr.slot,
             portName(instr.port), instr.delay);
  if (instr.port >= 2) {
    out.fatal(CALL_INFO, -1, "Invalid WIN2 TRANS port: %d\n", instr.port);
  }
  try {
    agus[instr.port].addTransition(instr.delay);
    current_event_number++;
  } catch (const std::exception &e) {
    out.fatal(CALL_INFO, -1, "WIN2 TRANS failed: %s\n", e.what());
  }
}
