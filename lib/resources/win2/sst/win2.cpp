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
  bulk_bitwidth = params.find<size_t>("bulk_bitwidth", 256);
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

  // Let AGU-gated events fire first (default priority is 5). The input AGU
  // lambda may shift the buffer; the offset AGU lambda may latch offset_reg.
  bool result = DRRAResource::clockTick(currentCycle);

  // Emit the aligned slice every logical cycle, after any same-cycle AGU
  // events have run. The RTL drives bulk_data_out_0 combinationally from
  // {line_B, line_A} and offset_reg every cycle; this DataEvent send is the
  // SST-level model of that continuous output.
  if (currentCycle % 10 == 8) {
    emitSlice();
  }

  return result;
}

void Win2::handleActivation(uint32_t slot_id, uint32_t ports) {
  portsToActivate[slot_id] = ports;
}

void Win2::absorbAndShift() {
  // Drain the input data link's FIFO, keeping only the freshest payload in
  // data_buffers[0]. This is the event-based approximation of "sample the
  // wire right now" — stale loopbacks queued by upstream routing are
  // discarded so the shift uses the most recent payload.
  while (Event *event = data_links[0]->recv()) {
    if (DataEvent *dataEvent = dynamic_cast<DataEvent *>(event)) {
      data_buffers[0] = dataEvent->payload;
    }
    delete event;
  }

  line_A = line_B;
  line_B.assign(bulk_bytes, 0);
  size_t take = std::min(data_buffers[0].size(), bulk_bytes);
  std::copy(data_buffers[0].begin(), data_buffers[0].begin() + take,
            line_B.begin());

  out.output(" WIN2 shift buffer: line_A<-line_B, line_B<-bulk_in (data=%s)\n",
             formatRawDataToWords(data_buffers[0]).c_str());
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
  // Continuous-output model: every logical cycle, send a DataEvent carrying
  // window[offset_reg*WORD +: BULK]. No gate on AGU validity, matching the
  // RTL combinational assign.
  std::vector<uint8_t> payload = takeAlignedSlice(offset_reg);

  DataEvent *dataEvent = new DataEvent(DataEvent::PortType::WriteWide);
  dataEvent->size = bulk_bitwidth;
  dataEvent->payload = payload;
  data_links[0]->send(dataEvent);

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

void Win2::handleDSU(const WIN2_PKG::DSUInstruction &instr) {
  out.output(
      "dsu (slot=%d, port=%s, option=%d, init_addr_sd=%d, init_addr=%d)\n",
      instr.slot, portName(instr.port), instr.option, instr.init_addr_sd,
      instr.init_addr);

  if (instr.port >= 2) {
    out.fatal(CALL_INFO, -1, "Invalid WIN2 DSU port: %d\n", instr.port);
  }

  agus[instr.port].setInitialAddress(instr.init_addr);

  std::string event_name =
      std::string("win2_") + portName(instr.port) + "_" +
      std::to_string(current_event_number);

  // Install the AGU-gated work as the event lambda. The base class fires
  // these only on cycles where the AGU has a valid address (mirroring the
  // RTL's agu_valid).
  switch (instr.port) {
  case PORT_INPUT:
    agus[PORT_INPUT].addEvent(event_name, [this] { absorbAndShift(); });
    break;
  case PORT_OFFSET:
    agus[PORT_OFFSET].addEvent(event_name, [this] { latchOffset(); });
    break;
  default:
    out.fatal(CALL_INFO, -1, "Invalid WIN2 DSU port: %d\n", instr.port);
  }

  current_event_number++;
}

void Win2::handleREP(const WIN2_PKG::REPInstruction &instr) {
  out.output("rep (slot=%d, port=%s, iter=%d, step=%d, delay=%d)\n", instr.slot,
             portName(instr.port), instr.iter, instr.step, instr.delay);
  if (instr.port >= 2) {
    out.fatal(CALL_INFO, -1, "Invalid WIN2 REP port: %d\n", instr.port);
  }
  try {
    agus[instr.port].addRepetition(instr.iter, instr.delay, instr.step);
  } catch (const std::exception &e) {
    out.fatal(CALL_INFO, -1, "WIN2 REP failed: %s\n", e.what());
  }
}

void Win2::handleREPX(const WIN2_PKG::REPXInstruction &instr) {
  out.output("repx (slot=%d, port=%s, iter=%d, step=%d, delay=%d)\n",
             instr.slot, portName(instr.port), instr.iter, instr.step,
             instr.delay);
  if (instr.port >= 2) {
    out.fatal(CALL_INFO, -1, "Invalid WIN2 REPX port: %d\n", instr.port);
  }
  auto repetition_op = agus[instr.port].getLastRepetitionOperator();
  uint32_t iter = instr.iter << WIN2_PKG::WIN2_INSTR_REPX_ITER_BITWIDTH |
                  repetition_op.getIterations();
  uint32_t step = instr.step << WIN2_PKG::WIN2_INSTR_REPX_STEP_BITWIDTH |
                  repetition_op.getStep();
  uint32_t delay = instr.delay << WIN2_PKG::WIN2_INSTR_REPX_DELAY_BITWIDTH |
                   repetition_op.getDelay();
  try {
    agus[instr.port].adjustRepetition(iter, delay, step);
  } catch (const std::exception &e) {
    out.fatal(CALL_INFO, -1, "WIN2 REPX failed: %s\n", e.what());
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
