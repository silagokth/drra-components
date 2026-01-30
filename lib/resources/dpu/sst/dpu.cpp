#include "dpu.h"
#include "dataEvent.h"
#include "dpu_operations.h"
#include "dpu_pkg.h"

using namespace SST;

Dpu::Dpu(SST::ComponentId_t id, SST::Params &params)
    : DRRAResource(id, params) {
  fsmHandlers.resize(num_fsms);
  dpuHandlers = DPU_Operations::createHandlers(this);
  instructionHandlers = DPU_PKG::createInstructionHandlers(this);
}

bool Dpu::clockTick(SST::Cycle_t currentCycle) {
  bool result = DRRAResource::clockTick(currentCycle);

  if (portsToActivate.size() > 0 && currentCycle % 10 == 0) {
    for (const auto &port : portsToActivate) {
      activatePortsForSlot(port.first, port.second);
    }
    portsToActivate.clear();
    current_config_option = 0;
    last_config_level = -1;
    last_config_trans = -1;
  }

  // Deal with data events
  for (int i = 0; i < resource_size; i++) {
    Event *event = data_links[i]->recv();
    if (event) {
      handleEventWithSlotID(event, i);
    }
  }

  // Execute DPU operation (priotity 9)
  if (currentCycle % 10 == 9) {
    for (const auto &port : active_ports) {
      if (isPortActive(port.first)) {
        out.output("Executing DPU operation for port %u\n", port.first);
        out.output(" Current FSM: %u\n", current_fsm);
        out.output(" fsmHandlers size: %lu\n", fsmHandlers.size());
        fsmHandlers[current_fsm]();
        break;
      }
    }
  }

  return result;
}

void Dpu::handleActivation(uint32_t slot_id, uint32_t ports) {
  portsToActivate[slot_id] = ports;
}

void Dpu::handleEventWithSlotID(SST::Event *event, uint32_t slot_id) {
  DataEvent *dataEvent = dynamic_cast<DataEvent *>(event);
  if (dataEvent) {
    bool anyPortActive = false;
    for (const auto &port : active_ports) {
      if (isPortActive(port.first)) {
        anyPortActive = true;
        break;
      }
    }
    if (dataEvent->portType != DataEvent::PortType::WriteNarrow)
      out.fatal(CALL_INFO, -1, "Invalid port type\n");

    // Store the data in the buffer of the slot
    out.output("Received data (slot=%d, size=%dbits, data=%s)\n", slot_id,
               dataEvent->size,
               formatRawDataToWords(dataEvent->payload).c_str());
    std::vector<uint8_t> data = dataEvent->payload;
    data_buffers[slot_id] = data;
  }
}

void Dpu::handleDPU(const DPU_PKG::DPUInstruction &instr) {
  out.output("dpu (slot=%d, option=%d, mode=%d, immediate=%d)\n", instr.slot,
             instr.option, instr.mode, instr.immediate);

  // replace the data buffer with the immediate value if needed
  if (instr.mode == DPU_PKG::DPU_MODE::DPU_MODE_ADD_CONST ||
      instr.mode == DPU_PKG::DPU_MODE::DPU_MODE_SUBT_ABS ||
      instr.mode == DPU_PKG::DPU_MODE::DPU_MODE_MULT_CONST ||
      instr.mode == DPU_PKG::DPU_MODE::DPU_MODE_MAX_MIN_CONST ||
      instr.mode == DPU_PKG::DPU_MODE::DPU_MODE_LD_IR) {
    data_buffers[1] = uint64ToVector(instr.immediate);
  }

  current_config_option = instr.option;

  // Add the reset event
  next_timing_states[0].addEvent(
      "dpu_reset_" + std::to_string(instr.option), 5, [this, instr] {
        out.output(" DPU accumulate register cleared\n");
        accumulate_register.clear();
        current_fsm = instr.option;
      });

  // Add the event handler
  fsmHandlers[instr.option] =
      DPU_Operations::getDPUHandler(this, (DPU_PKG::DPU_MODE)instr.mode);
}

void Dpu::handleREP(const DPU_PKG::REPInstruction &instr) {
  out.output("rep (slot=%d, iter=%d, step=%d, delay=%d)\n", instr.slot,
             instr.iter, instr.step, instr.delay);

  // Increment the last configuration level
  last_config_level++;
  out.output(" Repetition level set to %u\n", last_config_level);

  // print port_last_rep_level for debugging
  for (const auto &port_level : port_last_rep_level) {
    out.output("  port %u last rep level: %d\n", port_level.first,
               port_level.second);
  }

  auto rep = instr;
  // For now, we only support increasing repetition levels (and no skipping)
  if (last_config_level != port_last_rep_level[current_config_option] + 1) {
    out.fatal(CALL_INFO, -1, "Invalid repetition level (last=%u, curr=%u)\n",
              port_last_rep_level[current_config_option], last_config_level);
  } else {
    port_last_rep_level[current_config_option] = last_config_level;
  }

  // add repetition to the timing model
  try {
    next_timing_states[0].addRepetition(rep.iter, rep.delay, last_config_level,
                                        rep.step);
  } catch (const std::exception &e) {
    out.fatal(CALL_INFO, -1, "Failed to add repetition: %s\n", e.what());
  }
}

void Dpu::handleREPX(const DPU_PKG::REPXInstruction &instr) {
  out.output("repx (slot=%d, iter=%d, step=%d, delay=%d)\n", instr.slot,
             instr.iter, instr.step, instr.delay);

  auto repx = instr;
  auto repetition_op =
      next_timing_states[0].getRepetitionOperatorFromLevel(last_config_level);
  uint32_t iter = repx.iter << DPU_PKG::DPU_INSTR_REP_ITER_BITWIDTH |
                  repetition_op.getIterations();
  uint32_t step = repx.step << DPU_PKG::DPU_INSTR_REP_STEP_BITWIDTH |
                  repetition_op.getStep();
  uint32_t delay = repx.delay << DPU_PKG::DPU_INSTR_REP_DELAY_BITWIDTH |
                   repetition_op.getDelay();
  try {
    next_timing_states[0].adjustRepetition(iter, delay, last_config_level,
                                           step);
  } catch (const std::exception &e) {
    out.fatal(CALL_INFO, -1, "REPX failed: %s\n", e.what());
  }
}

void Dpu::handleTRANS(const DPU_PKG::TRANSInstruction &instr) {
  out.output("fsm (slot=%d, delay=%d)\n", instr.slot, instr.delay);

  last_config_trans++;

  auto fsm = instr;
  // add transition to the timing model
  try {
    next_timing_states[0].addTransition(
        fsm.delay,
        "dpu_trans_" + std::to_string(last_config_trans) + "_" +
            std::to_string(last_config_trans + 1),
        [this, fsm] {
          out.output(" FSM switched to %d\n", last_config_trans + 1);
          current_fsm = last_config_trans + 1;
        });
    current_event_number++;
  } catch (const std::exception &e) {
    out.fatal(CALL_INFO, -1, "Failed to add transition: %s\n", e.what());
  }
}

void Dpu::handleOperation(std::string name,
                          std::function<int64_t(int64_t, int64_t)> operation) {
  // Ensure both buffers exist
  if (data_buffers[0].size() == 0 || data_buffers[1].size() == 0) {
    out.fatal(CALL_INFO, -1, "Data buffers not found (data0=%lu, data1=%lu)\n",
              data_buffers[0].size(), data_buffers[1].size());
  }

  int64_t data0 = vectorToInt64(data_buffers[0]);
  int64_t data1 = vectorToInt64(data_buffers[1]);
  int64_t result = operation(data0, data1);

  DataEvent *dataEvent = new DataEvent(DataEvent::PortType::WriteNarrow);
  dataEvent->size = word_bitwidth;
  dataEvent->payload = int64ToVector(result);
  data_links[0]->send(dataEvent);

  out.output("DPU %s operation (in0=%ld, in1=%ld, out=%ld, acc=%ld)\n",
             name.c_str(), data0, data1, result,
             accumulate_register.size() > 0 ? vectorToInt64(accumulate_register)
                                            : 0);
  logTraceEvent(
      "operation", slot_id, true, 'X',
      {{"data0", static_cast<int>(data0)},
       {"data1", static_cast<int>(data1)},
       {"result", static_cast<int>(result)},
       {"accumulator", static_cast<int>(accumulate_register.size() > 0
                                            ? vectorToInt64(accumulate_register)
                                            : 0)}});
}
