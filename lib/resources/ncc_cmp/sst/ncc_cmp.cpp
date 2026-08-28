#include "ncc_cmp.h"
#include "dataEvent.h"
#include "ncc_cmp_pkg.h"
#include <algorithm>
#include <cstdint>

using namespace SST;

namespace {

const char *nccModeName(uint32_t mode) {
  switch (mode) {
  case NCC_CMP_PKG::NCC_MODE_IDLE:
    return "idle";
  case NCC_CMP_PKG::NCC_MODE_LOAD_S_A:
    return "load_s_a";
  case NCC_CMP_PKG::NCC_MODE_LOAD_S_A2:
    return "load_s_a2";
  case NCC_CMP_PKG::NCC_MODE_LOAD_S_AB:
    return "load_s_ab";
  case NCC_CMP_PKG::NCC_MODE_LOAD_S_B:
    return "load_s_b";
  case NCC_CMP_PKG::NCC_MODE_CMP:
    return "cmp";
  default:
    return "idle";
  }
}

const char *portName(uint32_t port) {
  switch (port) {
  case 0:
    return "ncc";
  case 1:
    return "rst";
  default:
    return "?";
  }
}

// Pack the low `bits` of an unsigned integer into a little-endian byte vector
// of size ceil(bits/8). Used to emit max_cnt onto the word port.
std::vector<uint8_t> uintToBytes(uint64_t value, size_t bits) {
  size_t bytes = (bits + 7) / 8;
  std::vector<uint8_t> buf(bytes, 0);
  for (size_t j = 0; j < bytes; j++) {
    buf[j] = static_cast<uint8_t>((value >> (j * 8)) & 0xFF);
  }
  return buf;
}

// Sign-extend the `[bits-1:lsb]` slice of a byte buffer into an int64_t.
int64_t bytesToSignedSlice(const std::vector<uint8_t> &buf, size_t bits,
                           size_t lsb) {
  if (bits == 0 || lsb >= bits)
    return 0;

  uint64_t v = 0;
  size_t take = std::min(buf.size(), static_cast<size_t>(8));
  for (size_t j = 0; j < take; j++) {
    v |= static_cast<uint64_t>(buf[j]) << (j * 8);
  }

  if (bits < 64) {
    uint64_t input_mask = (static_cast<uint64_t>(1) << bits) - 1;
    v &= input_mask;
  }

  size_t slice_bits = bits - lsb;
  v >>= lsb;

  if (slice_bits < 64) {
    uint64_t mask = (static_cast<uint64_t>(1) << slice_bits) - 1;
    v &= mask;
    uint64_t sign_bit = static_cast<uint64_t>(1) << (slice_bits - 1);
    if (v & sign_bit) {
      v |= ~mask;
    }
  }

  return static_cast<int64_t>(v);
}

struct NccTerms {
  __int128 num;
  __int128 denom;
};

NccTerms calcNccTerms(int64_t s_a, int64_t s_a2, int64_t s_ab, int64_t s_b,
                      size_t k_log2) {
  __int128 scale = static_cast<__int128>(1) << k_log2;
  __int128 s_a_w = static_cast<__int128>(s_a);
  __int128 s_b_w = static_cast<__int128>(s_b);

  NccTerms terms;
  terms.num = static_cast<__int128>(s_ab) * scale - (s_a_w * s_b_w);
  terms.denom = static_cast<__int128>(s_a2) * scale - (s_a_w * s_a_w);
  return terms;
}

long double nccScore(const NccTerms &terms) {
  long double num = static_cast<long double>(terms.num);
  return (num * num) / static_cast<long double>(terms.denom);
}

} // namespace

Ncc_cmp::Ncc_cmp(SST::ComponentId_t id, SST::Params &params)
    : DRRAResource(id, params) {
  // BULK_BITWIDTH is emitted verbatim (uppercase); lowercase is never emitted.
  bulk_bitwidth     = params.find<size_t>(
      "BULK_BITWIDTH", params.find<size_t>("bulk_bitwidth", 64));
  k_log2            = params.find<size_t>("K_LOG2", 14);
  linear_bitwidth   = params.find<size_t>("LINEAR_BITWIDTH", 28);
  quad_bitwidth     = params.find<size_t>("QUAD_BITWIDTH", 42);
  linear_rshift     = params.find<size_t>("LINEAR_RSHIFT", 5);
  product_bitwidth  = params.find<size_t>("PRODUCT_BITWIDTH", 144);

  if (linear_bitwidth == 0 || quad_bitwidth == 0) {
    out.fatal(CALL_INFO, -1,
              "LINEAR_BITWIDTH and QUAD_BITWIDTH must be > 0\n");
  }
  if (linear_bitwidth > 64 || quad_bitwidth > 64) {
    out.fatal(CALL_INFO, -1,
              "LINEAR_BITWIDTH and QUAD_BITWIDTH must each be <= 64\n");
  }
  if (linear_rshift >= linear_bitwidth) {
    out.fatal(CALL_INFO, -1,
              "LINEAR_RSHIFT (%zu) must be < LINEAR_BITWIDTH (%zu)\n",
              linear_rshift, linear_bitwidth);
  }
  if (2 * linear_rshift >= quad_bitwidth) {
    out.fatal(CALL_INFO, -1,
              "QUAD_RSHIFT (=2*%zu) must be < QUAD_BITWIDTH (%zu)\n",
              linear_rshift, quad_bitwidth);
  }
  if (k_log2 >= 127) {
    out.fatal(CALL_INFO, -1, "K_LOG2 (%zu) must be < 127\n", k_log2);
  }

  instructionHandlers = NCC_CMP_PKG::createInstructionHandlers(this);
  ncc_modes.fill(NCC_CMP_PKG::NCC_MODE_IDLE);
}

bool Ncc_cmp::clockTick(SST::Cycle_t currentCycle) {

  if (!portsToActivate.empty() && currentCycle % 10 == 0) {
    for (const auto &[sid, ports] : portsToActivate) {
      activatePortsForSlot(sid, ports);
    }
    portsToActivate.clear();
  }

  // Input wires are held registers now; gap cycles are modelled by the
  // producer/SWB driving 0, not by clearing here. Drain any values the SWB
  // delivered into the held input buffers.
  for (int i = 0; i < resource_size; i++) {
    while (Event *event = data_links[i]->recv()) {
      handleEventWithSlotID(event, i);
      delete event;
    }
  }

  // Execute ncc / rst at the Sample phase (sub-7): the input routed by the SWB
  // at the Route phase (sub-5) is available, and max_cnt drives the registered
  // word output, committed at sub-9 -> one cycle of latency.
  if (currentCycle % 10 == 7) {
    if (isPortActive(0)) {
      int64_t agu_addr = agus[0].getAddressForCycle(getPortActiveCycle(0));
      uint32_t config = static_cast<uint32_t>(agu_addr) &
                        static_cast<uint32_t>(ncc_modes.size() - 1);
      doNcc(ncc_modes[config]);
    }
    if (isPortActive(1)) {
      doReset();
    }
  }

  bool result = DRRAResource::clockTick(currentCycle);
  return result;
}

void Ncc_cmp::handleActivation(uint32_t slot_id, uint32_t ports) {
  portsToActivate[slot_id] = ports;
}

void Ncc_cmp::handleEventWithSlotID(SST::Event *event, uint32_t slot_id) {
  DataEvent *dataEvent = dynamic_cast<DataEvent *>(event);
  if (!dataEvent)
    return;

  out.output("NCC_CMP received data (slot=%d, size=%dbits, data=%s)\n",
             slot_id, dataEvent->size,
             formatRawDataToWords(dataEvent->payload).c_str());
  data_buffers[slot_id] = dataEvent->payload;
}

void Ncc_cmp::doNcc(uint32_t mode) {
  switch (mode) {
  case NCC_CMP_PKG::NCC_MODE_IDLE:
    return;
  case NCC_CMP_PKG::NCC_MODE_LOAD_S_A:
  case NCC_CMP_PKG::NCC_MODE_LOAD_S_A2:
  case NCC_CMP_PKG::NCC_MODE_LOAD_S_AB:
  case NCC_CMP_PKG::NCC_MODE_LOAD_S_B:
    doLoad(mode);
    return;
  case NCC_CMP_PKG::NCC_MODE_CMP:
    doCompare();
    return;
  default:
    return;
  }
}

void Ncc_cmp::doLoad(uint32_t mode) {
  if (data_buffers[0].empty()) {
    out.output(" NCC_CMP load idle (no input data on slot 0)\n");
    return;
  }
  switch (mode) {
  case NCC_CMP_PKG::NCC_MODE_LOAD_S_A: {
    int64_t v =
        bytesToSignedSlice(data_buffers[0], linear_bitwidth, linear_rshift);
    s_a_cur = v;
    out.output(" NCC_CMP load S_A  = %lld\n", static_cast<long long>(v));
    logTraceEvent("ncc_load", slot_id, true, 'X',
                  {{"reg", std::string("S_A")},
                   {"value", static_cast<long long>(v)}});
    break;
  }
  case NCC_CMP_PKG::NCC_MODE_LOAD_S_A2: {
    int64_t v =
        bytesToSignedSlice(data_buffers[0], quad_bitwidth, getQuadRshift());
    s_a2_cur = v;
    out.output(" NCC_CMP load S_A2 = %lld\n", static_cast<long long>(v));
    logTraceEvent("ncc_load", slot_id, true, 'X',
                  {{"reg", std::string("S_A2")},
                   {"value", static_cast<long long>(v)}});
    break;
  }
  case NCC_CMP_PKG::NCC_MODE_LOAD_S_AB: {
    int64_t v =
        bytesToSignedSlice(data_buffers[0], quad_bitwidth, getQuadRshift());
    s_ab_cur = v;
    out.output(" NCC_CMP load S_AB = %lld\n", static_cast<long long>(v));
    logTraceEvent("ncc_load", slot_id, true, 'X',
                  {{"reg", std::string("S_AB")},
                   {"value", static_cast<long long>(v)}});
    break;
  }
  case NCC_CMP_PKG::NCC_MODE_LOAD_S_B: {
    int64_t v =
        bytesToSignedSlice(data_buffers[0], linear_bitwidth, linear_rshift);
    s_b = v;
    out.output(" NCC_CMP load S_B  = %lld\n", static_cast<long long>(v));
    logTraceEvent("ncc_load", slot_id, true, 'X',
                  {{"reg", std::string("S_B")},
                   {"value", static_cast<long long>(v)}});
    break;
  }
  default:
    out.fatal(CALL_INFO, -1, "NCC_CMP invalid load mode: %u\n", mode);
  }
}

void Ncc_cmp::doCompare() {
  bool win = currentBeatsBest();
  uint64_t pos = local_cnt;
  local_cnt++;
  if (win) {
    promoteBest();
    max_cnt = pos;
    out.output(" NCC_CMP new max at position %llu\n",
               static_cast<unsigned long long>(max_cnt));
    logTraceEvent(
        "ncc_update", slot_id, true, 'X',
        {{"position", static_cast<long long>(max_cnt)},
         {"local_cnt", static_cast<long long>(local_cnt)}});
  } else {
    out.output(" NCC_CMP no update at position %llu (local_cnt=%llu)\n",
               static_cast<unsigned long long>(pos),
               static_cast<unsigned long long>(local_cnt));
  }
  emitMaxCount();
}

bool Ncc_cmp::currentBeatsBest() const {
  NccTerms cur = calcNccTerms(s_a_cur, s_a2_cur, s_ab_cur, s_b, k_log2);
  if (cur.denom <= 0)
    return false;
  if (!best_valid)
    return true;

  NccTerms best = calcNccTerms(s_a_best, s_a2_best, s_ab_best, s_b, k_log2);
  if (best.denom <= 0)
    return true;

  return nccScore(cur) > nccScore(best);
}

void Ncc_cmp::promoteBest() {
  s_a_best = s_a_cur;
  s_a2_best = s_a2_cur;
  s_ab_best = s_ab_cur;
  best_valid = true;
}

void Ncc_cmp::doReset() {
  out.output(" NCC_CMP reset\n");
  clearAllState();
  logTraceEvent("ncc_reset", slot_id, true, 'X', {});
  emitMaxCount();
}

void Ncc_cmp::clearAllState() {
  s_a_cur = s_a2_cur = s_ab_cur = 0;
  s_a_best = s_a2_best = s_ab_best = 0;
  s_b = 0;
  best_valid = false;
  local_cnt = 0;
  max_cnt = 0;
}

void Ncc_cmp::emitMaxCount() {
  uint64_t masked =
      (word_bitwidth >= 64)
          ? max_cnt
          : (max_cnt & ((static_cast<uint64_t>(1) << word_bitwidth) - 1));
  // Registered output from max_cnt: latched at the clock edge -> 1 cycle
  // latency, matching ncc_cmp.sv.j2.
  driveOutput(0, PortChannel::WORD, uintToBytes(masked, word_bitwidth),
              word_bitwidth, /*registered=*/true);
  out.output(" NCC_CMP emit word max_cnt=%llu\n",
             static_cast<unsigned long long>(masked));
}

void Ncc_cmp::handleNCC(const NCC_CMP_PKG::NCCInstruction &instr) {
  if (instr.config >= ncc_modes.size()) {
    out.fatal(CALL_INFO, -1, "Invalid NCC config: %d\n", instr.config);
  }
  out.output("ncc (slot=%d, config=%d, mode=%s)\n", instr.slot, instr.config,
             nccModeName(instr.mode));
  ncc_modes[instr.config] = instr.mode;
}

void Ncc_cmp::handleEVT(const NCC_CMP_PKG::EVTInstruction &instr) {
  if (instr.port >= 2) {
    out.fatal(CALL_INFO, -1, "Invalid NCC EVT port: %d\n", instr.port);
  }
  out.output("evt (slot=%d, option=%d, port=%s, init_addr_sd=%d, init_addr=%d)\n",
             instr.slot, instr.option, portName(instr.port), instr.init_addr_sd,
             instr.init_addr);
  agus[instr.port].addEvent(
      std::string("ncc_") + portName(instr.port), [] {}, 5, instr.init_addr);
}

void Ncc_cmp::handleREP(const NCC_CMP_PKG::REPInstruction &instr) {
  if (instr.port >= 2) {
    out.fatal(CALL_INFO, -1, "Invalid NCC REP port: %d\n", instr.port);
  }
  out.output("rep (slot=%d, port=%s, iter=%d, step=%d, delay=%d)\n",
             instr.slot, portName(instr.port), instr.iter, instr.step,
             instr.delay);
  try {
    agus[instr.port].addRepetition(instr.iter, instr.delay, instr.step);
  } catch (const std::exception &e) {
    out.fatal(CALL_INFO, -1, "NCC_CMP REP failed: %s\n", e.what());
  }
}

void Ncc_cmp::handleREPX(const NCC_CMP_PKG::REPXInstruction &instr) {
  if (instr.port >= 2) {
    out.fatal(CALL_INFO, -1, "Invalid NCC REPX port: %d\n", instr.port);
  }
  out.output("repx (slot=%d, port=%s, iter=%d, step=%d, delay=%d)\n",
             instr.slot, portName(instr.port), instr.iter, instr.step,
             instr.delay);
  auto repetition_op = agus[instr.port].getLastRepetitionOperator();
  uint32_t iter = instr.iter << NCC_CMP_PKG::NCC_CMP_INSTR_REP_ITER_BITWIDTH |
                  repetition_op.getIterations();
  uint32_t step = instr.step << NCC_CMP_PKG::NCC_CMP_INSTR_REP_STEP_BITWIDTH |
                  repetition_op.getStep();
  uint32_t delay = instr.delay
                       << NCC_CMP_PKG::NCC_CMP_INSTR_REP_DELAY_BITWIDTH |
                   repetition_op.getDelay();
  try {
    agus[instr.port].adjustRepetition(iter, delay, step);
  } catch (const std::exception &e) {
    out.fatal(CALL_INFO, -1, "NCC_CMP REPX failed: %s\n", e.what());
  }
}

void Ncc_cmp::handleTRANS(const NCC_CMP_PKG::TRANSInstruction &instr) {
  if (instr.port >= 2) {
    out.fatal(CALL_INFO, -1, "Invalid NCC TRANS port: %d\n", instr.port);
  }
  out.output("trans (slot=%d, port=%s, delay=%d)\n", instr.slot,
             portName(instr.port), instr.delay);
  try {
    agus[instr.port].addTransition(instr.delay);
  } catch (const std::exception &e) {
    out.fatal(CALL_INFO, -1, "NCC_CMP TRANS failed: %s\n", e.what());
  }
}
