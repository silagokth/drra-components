#include "vpu.h"
#include "dataEvent.h"
#include "vpu_operations.h"
#include "vpu_pkg.h"

using namespace SST;

Vpu::Vpu(SST::ComponentId_t id, SST::Params &params)
    : DRRAResource(id, params) {
  // Params
  fractional_bitwidth = params.find<uint32_t>("FRACTIONAL_BITWIDTH", 0);

  vpuHandlers = VPU_Operations::createHandlers(this);
  instructionHandlers = VPU_PKG::createInstructionHandlers(this);
  fsmHandlers.assign(num_fsms, vpuHandlers.at(VPU_PKG::VPU_MODE::VPU_MODE_IDLE));
  imm_buffers.assign(num_fsms, {});
  input_latch_0.assign(io_data_width / 8, 0);
  input_latch_1.assign(io_data_width / 8, 0);
  // initialize current configuration options to 0
  for (int i = 0; i < 4; i++) {
    current_config_option[i] = 0;
    port_last_rep_level[i] = -1;
  }
}

// Inputs, operation and FSM update, in the order the ten-tick version ran
// them before the base executed its priority-9 events. The lifetime check in
// finishCycle still runs after this, so the last result of a burst is not
// dropped.
void Vpu::onPhaseBeforeEvents(uint8_t phase) {
  // Data events queued so far this cycle; the last one on a link wins, as it
  // did when every subcycle polled. Drained at both phases: the priority-5
  // latch events read these buffers, and so does the operation at 9.
  for (int i = 0; i < resource_size; i++) {
    while (Event *event = data_links[i]->recv()) {
      handleEventWithSlotID(event, i);
      delete event;
    }
  }
  if (phase != 9)
    return;

  out.output(" Current FSM: %u\n", current_fsm);
  out.output(" fsmHandlers size: %lu\n", fsmHandlers.size());
  fsmHandlers[current_fsm]();

  // Update FSM for next execution based on AGU output (one cycle delayed).
  if (isPortActive(0)) {
    int64_t agu_address = agus[0].getAddressForCycle(getPortActiveCycle(0));
    if (agu_address >= 0 && agu_address != current_fsm) {
      current_fsm = agu_address;
      out.output(" FSM switched to FSM #%u\n", current_fsm);
    }
  }
}

// Input buffers were cleared at subcycle 0 so that gap cycles (where the RF
// AGU enable is 0 and its output is 0) are modelled correctly; clearing them
// once the operation has consumed them is the same thing.
void Vpu::onPhaseAfterEvents(uint8_t phase) {
  if (phase != 9)
    return;
  for (int i = 0; i < resource_size; i++) {
    std::fill(data_buffers[i].begin(), data_buffers[i].end(), 0);
  }
}

void Vpu::onActivationApplied(uint32_t slot_id) {
  current_config_option[slot_id] = 0;
}

void Vpu::onActivationsApplied() {
  last_config_level = -1;
  last_config_trans = -1;
}

void Vpu::handleActivation(uint32_t slot_id, uint32_t ports) {
  deferActivation(slot_id, ports);
}

void Vpu::handleEventWithSlotID(SST::Event *event, uint32_t slot_id) {
  DataEvent *dataEvent = dynamic_cast<DataEvent *>(event);
  if (dataEvent) {
    // Store the data in the buffer of the slot
    out.output("Received data (slot=%d, size=%dbits, data=%s)\n", slot_id,
               dataEvent->size,
               formatRawDataToWords(dataEvent->payload).c_str());
    std::vector<uint8_t> data = dataEvent->payload;
    data_buffers[slot_id] = data;
  }
}

void Vpu::handleVPU(const VPU_PKG::VPUInstruction &instr) {
  out.output("vpu (slot=%d, config=%d, mode=%d, immediate=%d)\n", instr.slot,
             instr.config, instr.mode, instr.immediate);

  // rtl/mode_selector.sv.j2 registers the immediate for every VPU instruction,
  // regardless of mode; mirror that. (The test that used to stand here was a
  // `a == b || c || d || ...` chain, i.e. always true, so it did this anyway.)
  imm_buffers[instr.config] = uint64ToVector(instr.immediate);

  fsmHandlers[instr.config] =
      VPU_Operations::getVPUHandler(this, (VPU_PKG::VPU_MODE)instr.mode);
}

void Vpu::handleEVT(const VPU_PKG::EVTInstruction &instr) {
  out.output("evt (slot=%d, port=%d)\n", instr.slot, instr.port);

  switch (instr.port) {
  case VPU_PKG::EVT_PORT::EVT_PORT_VPU:
    agus[VPU_PKG::EVT_PORT::EVT_PORT_VPU].addEvent(
        "vpu_event_" +
            std::to_string(
                current_config_option[VPU_PKG::EVT_PORT::EVT_PORT_VPU]),
        [this] {});
    break;
  case VPU_PKG::EVT_PORT::EVT_PORT_RST:
    agus[VPU_PKG::EVT_PORT::EVT_PORT_RST].addEvent(
        "vpu_reset_" +
            std::to_string(
                current_config_option[VPU_PKG::EVT_PORT::EVT_PORT_RST]),
        [this] {
          out.output(" VPU accumulate register cleared\n");
          accumulate_register.clear();
        });
    break;
  case VPU_PKG::EVT_PORT::EVT_PORT_IN0:
    agus[VPU_PKG::EVT_PORT::EVT_PORT_IN0].addEvent(
        "vpu_in0_latch_" +
            std::to_string(
                current_config_option[VPU_PKG::EVT_PORT::EVT_PORT_IN0]),
        [this] {
          input_latch_0 = data_buffers[0];
          auto vals0 = byteVectorToInt64Vector(input_latch_0);
          out.output(" IN0 latched: %s\n", int64VectorToString(vals0).c_str());
          logTraceEvent("in0_latch", slot_id, true, 'X',
                        {{"data", int64VectorToString(vals0)}});
        });
    break;
  case VPU_PKG::EVT_PORT::EVT_PORT_IN1:
    agus[VPU_PKG::EVT_PORT::EVT_PORT_IN1].addEvent(
        "vpu_in1_latch_" +
            std::to_string(
                current_config_option[VPU_PKG::EVT_PORT::EVT_PORT_IN1]),
        [this] {
          input_latch_1 = data_buffers[1];
          auto vals1 = byteVectorToInt64Vector(input_latch_1);
          out.output(" IN1 latched: %s\n", int64VectorToString(vals1).c_str());
          logTraceEvent("in1_latch", slot_id, true, 'X',
                        {{"data", int64VectorToString(vals1)}});
        });
    break;

  default:
    out.fatal(CALL_INFO, -1, "Invalid EVT port: %d\n", instr.port);
    break;
  };
}

void Vpu::handleREP(const VPU_PKG::REPInstruction &instr) {
  out.output("rep (slot=%d, port=%d, iter=%d, step=%d, delay=%d)\n", instr.slot,
             instr.port, instr.iter, instr.step, instr.delay);

  auto rep = instr;

  // Add repetition to the timing model
  try {
    agus[instr.port].addRepetition(rep.iter, rep.delay, rep.step);
  } catch (const std::exception &e) {
    out.fatal(CALL_INFO, -1, "Failed to add repetition: %s\n", e.what());
  }
}

void Vpu::handleREPX(const VPU_PKG::REPXInstruction &instr) {
  out.output("repx (slot=%d, port=%d, iter=%d, step=%d, delay=%d)\n",
             instr.slot, instr.port, instr.iter, instr.step, instr.delay);

  auto repx = instr;
  auto repetition_op = agus[instr.port].getLastRepetitionOperator();
  uint32_t iter = repx.iter << VPU_PKG::VPU_INSTR_REP_ITER_BITWIDTH |
                  repetition_op.getIterations();
  uint32_t step = repx.step << VPU_PKG::VPU_INSTR_REP_STEP_BITWIDTH |
                  repetition_op.getStep();
  uint32_t delay = repx.delay << VPU_PKG::VPU_INSTR_REP_DELAY_BITWIDTH |
                   repetition_op.getDelay();

  try {
    agus[instr.port].adjustRepetition(iter, delay, step);
  } catch (const std::exception &e) {
    out.fatal(CALL_INFO, -1, "REPX failed: %s\n", e.what());
  }
}

void Vpu::handleTRANS(const VPU_PKG::TRANSInstruction &instr) {
  out.output("trans (slot=%d, port=%d, delay=%d)\n", instr.slot, instr.port,
             instr.delay);

  // Add transition to the timing model
  try {
    agus[instr.port].addTransition(instr.delay);
  } catch (const std::exception &e) {
    out.fatal(CALL_INFO, -1, "Failed to add transition: %s\n", e.what());
  }
}

std::vector<int64_t>
Vpu::byteVectorToInt64Vector(const std::vector<uint8_t> &byte_vector) {
  if (byte_vector.size() * 8 % word_bitwidth != 0) {
    throw std::invalid_argument(
        "Byte vector size is not a multiple of word bitwidth");
  }

  std::vector<int64_t> result;

  int num_words = (byte_vector.size() * 8) / word_bitwidth;
  for (int i = 0; i < num_words; i++) {
    uint64_t raw = 0;
    for (size_t j = 0; j < word_bitwidth / 8; j++) {
      raw |= static_cast<uint64_t>(byte_vector[i * (word_bitwidth / 8) + j])
             << (j * 8);
    }
    // Sign-extend from word_bitwidth to 64 bits, exactly as
    // DRRAResource::vectorToInt64 does for the narrow resources. Without this
    // every lane read back as an unsigned 0..2^W-1 value, so max(x, 0) — i.e.
    // ReLU — saw -5 as 65531 and left it alone.
    if (word_bitwidth > 0 && word_bitwidth < 64) {
      uint64_t sign_bit = 1ULL << (word_bitwidth - 1);
      if (raw & sign_bit) {
        raw |= ~((1ULL << word_bitwidth) - 1);
      }
    }
    result.push_back(static_cast<int64_t>(raw));
  }

  return result;
}

std::vector<uint8_t>
Vpu::int64VectorToByteVector(const std::vector<int64_t> &int_vector) {
  std::vector<uint8_t> byte_vector;

  for (const auto &value : int_vector) {
    for (size_t j = 0; j < word_bitwidth / 8; j++) {
      byte_vector.push_back((value >> (j * 8)) & 0xFF);
    }
  }

  return byte_vector;
}

std::string Vpu::int64VectorToString(const std::vector<int64_t> &vec) {
  std::string result = "[";
  for (size_t i = 0; i < vec.size(); i++) {
    result += std::to_string(vec[i]);
    if (i != vec.size() - 1) {
      result += ", ";
    }
  }
  result += "]";
  return result;
}

void Vpu::handleOperation(std::string name,
                          std::function<std::vector<int64_t>(
                              std::vector<int64_t>, std::vector<int64_t>)>
                              operation,
                          bool use_immediate) {
  std::vector<int64_t> data0 = byteVectorToInt64Vector(input_latch_0);
  // *_imm modes read their scalar operand from the instruction immediate that
  // the VPU instruction stored for this FSM state; every other mode reads the
  // in1 latch. Previously all modes read the in1 latch, so `max_imm` silently
  // ignored its immediate and used whatever was on in1.
  std::vector<int64_t> data1;
  if (use_immediate) {
    const std::vector<uint8_t> &imm = imm_buffers[current_fsm];
    data1.push_back(imm.empty() ? 0 : vectorToInt64(imm));
  } else {
    data1 = byteVectorToInt64Vector(input_latch_1);
  }
  std::vector<int64_t> result = operation(data0, data1);

  DataEvent *dataEvent = new DataEvent(DataEvent::PortType::WriteWide);
  dataEvent->payload = int64VectorToByteVector(result);
  dataEvent->size = dataEvent->payload.size() * 8;
  data_links[0]->send(dataEvent);

  out.output("VPU %s operation (in0=%s, in1=%s, out=%s, acc=%s)\n",
             name.c_str(), int64VectorToString(data0).c_str(),
             int64VectorToString(data1).c_str(),
             int64VectorToString(result).c_str(),
             accumulate_register.size() > 0
                 ? int64VectorToString(accumulate_register).c_str()
                 : "[]");
  logTraceEvent("operation", slot_id, true, 'X',
                {{"data0", int64VectorToString(data0)},
                 {"data1", int64VectorToString(data1)},
                 {"result", int64VectorToString(result)},
                 {"accumulator", accumulate_register.size() > 0
                                     ? int64VectorToString(accumulate_register)
                                     : "[]"}});
}
