#include "acc.h"
#include "acc_operations.h"
#include "acc_pkg.h"
#include "dataEvent.h"
#include <cstdint>
#include <numeric>

using namespace SST;

Acc::Acc(SST::ComponentId_t id, SST::Params &params) : DRRAResource(id, params) {
  bulk_bitwidth = params.find<size_t>("bulk_bitwidth", 64);
  size_t default_acc_reg_bitwidth =
      params.find<size_t>("acc_reg_bitwidth", bulk_bitwidth);
  acc_reg_bitwidth =
      params.find<size_t>("ACC_REG_BITWIDTH", default_acc_reg_bitwidth);
  include_multiplier = params.find<bool>("INCLUDE_MULTIPLIER", true);
  if (acc_reg_bitwidth == 0) {
    out.fatal(CALL_INFO, -1, "ACC_REG_BITWIDTH must be greater than zero\n");
  }
  if (acc_reg_bitwidth < word_bitwidth) {
    out.fatal(CALL_INFO, -1,
              "ACC_REG_BITWIDTH=%zu must be >= word_bitwidth=%zu\n",
              acc_reg_bitwidth, word_bitwidth);
  }
  if (acc_reg_bitwidth > 64) {
    out.fatal(CALL_INFO, -1,
              "ACC_REG_BITWIDTH=%zu exceeds the SST model's int64_t accumulator\n",
              acc_reg_bitwidth);
  }

  fsmHandlers.resize(num_fsms);
  accHandlers = ACC_Operations::createHandlers(this, include_multiplier);
  instructionHandlers = ACC_PKG::createInstructionHandlers(this);

  for (int i = 0; i < 2; i++) {
    current_config_option[i] = 0;
    port_last_rep_level[i] = -1;
  }
  for (uint32_t i = 0; i < num_fsms; i++) {
    fsmHandlers[i] = accHandlers.at(ACC_PKG::ACC_MODE::ACC_MODE_IDLE);
  }
}

bool Acc::clockTick(SST::Cycle_t currentCycle) {

  if (!portsToActivate.empty() && currentCycle % 10 == 0) {
    for (const auto &[sid, ports] : portsToActivate) {
      activatePortsForSlot(sid, ports);
      current_config_option[sid] = 0;
    }
    portsToActivate.clear();
    last_config_level = -1;
    last_config_trans = -1;
  }

  // Clear input buffers at the start of each logical cycle so that gap cycles
  // (where the RF AGU enable is 0 and its output is 0) are modelled correctly.
  if (currentCycle % 10 == 0) {
    for (int i = 0; i < resource_size; i++) {
      std::fill(data_buffers[i].begin(), data_buffers[i].end(), 0);
    }
  }

  // Absorb any incoming data events on this cycle.
  for (int i = 0; i < resource_size; i++) {
    Event *event = data_links[i]->recv();
    if (event) {
      handleEventWithSlotID(event, i);
    }
  }

  // Execute accumulation at priority 9.
  if (currentCycle % 10 == 9) {
    
    // Update FSM for next execution based on AGU output (one cycle delayed).
    for (const auto &[port_id, is_active] : active_ports) {
      if (!is_active)
      continue;
      if (port_id == 0) {
        int64_t agu_address = agus[0].getAddressForCycle(getPortActiveCycle(0));
        if (agu_address >= 0 && static_cast<uint32_t>(agu_address) != current_fsm) {
          current_fsm = static_cast<uint32_t>(agu_address);
          out.output(" ACC FSM switched to #%u\n", current_fsm);
        }
        break;
      }
    }
    // Gated on port 0 being active.
    if (isPortActive(0)) {
      out.output(" Current FSM: %u\n", current_fsm);
      out.output(" fsmHandlers size: %lu\n", fsmHandlers.size());
      fsmHandlers[current_fsm]();
    }
  }

  bool result = DRRAResource::clockTick(currentCycle);
  return result;
}

void Acc::handleActivation(uint32_t slot_id, uint32_t ports) {
  portsToActivate[slot_id] = ports;
}

void Acc::handleEventWithSlotID(SST::Event *event, uint32_t slot_id) {
  DataEvent *dataEvent = dynamic_cast<DataEvent *>(event);
  if (!dataEvent)
    return;

  out.output("ACC received data (slot=%d, size=%dbits, data=%s)\n", slot_id,
             dataEvent->size,
             formatRawDataToWords(dataEvent->payload).c_str());
  data_buffers[slot_id] = dataEvent->payload;
}

void Acc::handleACC(const ACC_PKG::ACCInstruction &instr) {
  auto mode = static_cast<ACC_PKG::ACC_MODE>(instr.mode);
  out.output("acc (slot=%d, config=%d, mode=%d)\n", instr.slot, instr.config,
             instr.mode);
  auto handler = accHandlers.find(mode);
  if (handler == accHandlers.end()) {
    out.fatal(CALL_INFO, -1, "Unsupported ACC mode: %d\n",
              static_cast<int>(mode));
  }
  fsmHandlers[instr.config] = handler->second;
}

void Acc::handleEVT(const ACC_PKG::EVTInstruction &instr) {
  out.output("evt (slot=%d, port=%s)\n", instr.slot,
             instr.port == 0 ? "acc" : "rst");

  switch (instr.port) {
  case ACC_PKG::EVT_PORT::EVT_PORT_ACC:
    agus[ACC_PKG::EVT_PORT::EVT_PORT_ACC].addEvent("acc_event", [this] {});
    break;
  case ACC_PKG::EVT_PORT::EVT_PORT_RST:
    agus[ACC_PKG::EVT_PORT::EVT_PORT_RST].addEvent("acc_reset", [this] {
      clearAccumulator();
    });
    break;
  default:
    out.fatal(CALL_INFO, -1, "Invalid ACC EVT port: %d\n", instr.port);
  }
}

void Acc::handleREP(const ACC_PKG::REPInstruction &instr) {
  out.output("rep (slot=%d, port=%s, iter=%d, step=%d, delay=%d)\n", instr.slot,
             instr.port == 0 ? "acc" : "rst", instr.iter, instr.step,
             instr.delay);
  try {
    agus[instr.port].addRepetition(instr.iter, instr.delay, instr.step);
  } catch (const std::exception &e) {
    out.fatal(CALL_INFO, -1, "ACC REP failed: %s\n", e.what());
  }
}

void Acc::handleREPX(const ACC_PKG::REPXInstruction &instr) {
  out.output("repx (slot=%d, port=%s, iter=%d, step=%d, delay=%d)\n",
             instr.slot, instr.port == 0 ? "acc" : "rst", instr.iter,
             instr.step, instr.delay);
  auto repetition_op = agus[instr.port].getLastRepetitionOperator();
  uint32_t iter = instr.iter << ACC_PKG::ACC_INSTR_REP_ITER_BITWIDTH |
                  repetition_op.getIterations();
  uint32_t step = instr.step << ACC_PKG::ACC_INSTR_REP_STEP_BITWIDTH |
                  repetition_op.getStep();
  uint32_t delay = instr.delay << ACC_PKG::ACC_INSTR_REP_DELAY_BITWIDTH |
                   repetition_op.getDelay();
  try {
    agus[instr.port].adjustRepetition(iter, delay, step);
  } catch (const std::exception &e) {
    out.fatal(CALL_INFO, -1, "ACC REPX failed: %s\n", e.what());
  }
}

void Acc::handleTRANS(const ACC_PKG::TRANSInstruction &instr) {
  out.output("trans (slot=%d, port=%s, delay=%d)\n", instr.slot,
             instr.port == 0 ? "acc" : "rst", instr.delay);
  try {
    agus[instr.port].addTransition(instr.delay);
  } catch (const std::exception &e) {
    out.fatal(CALL_INFO, -1, "ACC TRANS failed: %s\n", e.what());
  }
}

void Acc::handleOperation(std::string name,
                          std::function<void(int64_t)> operation) {
  if (data_buffers[0].size() == 0) {
    out.output(" ACC idle (no input data buffered on slot 0)\n");
    return;
  }

  int64_t input = vectorToInt64(data_buffers[0]);
  int64_t before = accumulate_register;
  operation(input);
  normalizeAccumulator();

  out.output("ACC %s (in=%ld, before=%ld, after=%ld)\n", name.c_str(), input,
             before, accumulate_register);

  logTraceEvent("accumulate", slot_id, true, 'X',
                {{"mode", name},
                 {"input", static_cast<int>(input)},
                 {"accumulator", static_cast<int>(accumulate_register)}});

  emitAccumulator();
}

std::vector<int64_t>
Acc::byteVectorToInt64Vector(const std::vector<uint8_t> &buf) const {
  size_t word_bytes = word_bitwidth / 8;
  size_t num_words = (buf.size() * 8) / word_bitwidth;
  std::vector<int64_t> result;
  for (size_t i = 0; i < num_words; i++) {
    int64_t word = 0;
    for (size_t j = 0; j < word_bytes; j++)
      word |= static_cast<int64_t>(buf[i * word_bytes + j]) << (j * 8);
    result.push_back(word);
  }
  return result;
}

void Acc::handleVectorOperation(std::string name,
                                std::function<void(std::vector<int64_t>)> operation) {
  if (data_buffers[0].empty()) {
    out.output(" ACC idle (no input data on slot 0)\n");
    return;
  }

  std::vector<int64_t> words = byteVectorToInt64Vector(data_buffers[0]);
  int64_t before = accumulate_register;
  operation(words);
  normalizeAccumulator();

  out.output("ACC %s (words=%zu, before=%ld, after=%ld)\n",
             name.c_str(), words.size(), before, accumulate_register);
  logTraceEvent("accumulate", slot_id, true, 'X',
                {{"mode", name},
                 {"accumulator", static_cast<int>(accumulate_register)}});
  emitAccumulator();
}

void Acc::clearAccumulator() {
  out.output(" ACC accumulator register cleared\n");
  accumulate_register = 0;
  emitAccumulator();
}

void Acc::normalizeAccumulator() {
  if (acc_reg_bitwidth >= 64) {
    return;
  }

  uint64_t mask = (UINT64_C(1) << acc_reg_bitwidth) - 1;
  uint64_t value = static_cast<uint64_t>(accumulate_register) & mask;
  uint64_t sign_bit = UINT64_C(1) << (acc_reg_bitwidth - 1);
  if ((value & sign_bit) != 0) {
    value |= ~mask;
  }
  accumulate_register = static_cast<int64_t>(value);
}

std::vector<uint8_t> Acc::int64ToBulkVector(int64_t data) const {
  size_t bulk_bytes = (bulk_bitwidth + 7) / 8;
  std::vector<uint8_t> buf(bulk_bytes, 0);
  uint64_t udata = static_cast<uint64_t>(data);
  for (size_t j = 0; j < std::min<size_t>(bulk_bytes, 8); j++) {
    buf[j] = (udata >> (j * 8)) & 0xFF;
  }
  // Sign-extend upper bytes if the value is negative and bulk > 64 bits.
  if (data < 0) {
    for (size_t j = 8; j < bulk_bytes; j++) {
      buf[j] = 0xFF;
    }
  }
  return buf;
}

const std::vector<uint8_t> &Acc::getDataBuffer(uint32_t slot_id) const {
  return data_buffers.at(slot_id);
}

const std::vector<int64_t> &Acc::getOperandRegister() const {
  return operand_register;
}

void Acc::setOperandRegister(std::vector<int64_t> value) {
  operand_register = std::move(value);
}

void Acc::emitAccumulator() {
  DataEvent *dataEvent = new DataEvent(DataEvent::PortType::WriteWide);
  dataEvent->size = bulk_bitwidth;
  dataEvent->payload = int64ToBulkVector(accumulate_register);
  data_links[0]->send(dataEvent);
  out.output(" ACC emit bulk (value=%ld, bits=%zu)\n", accumulate_register,
             bulk_bitwidth);
}
