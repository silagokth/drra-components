#include "dpu_2cycle_mac.h"
#include "dataEvent.h"
#include "dpu_operations.h"
#include "timingOperators.h"

using namespace SST;

Dpu_2cycle_mac::Dpu_2cycle_mac(SST::ComponentId_t id, SST::Params &params)
    : DRRAResource(id, params) {
  fsmHandlers.resize(num_fsms);
  dpuHandlers = DPU_Operations::createHandlers(this);
  instructionHandlers = DPU_2CYCLE_MAC_PKG::createInstructionHandlers(this);
}

bool Dpu_2cycle_mac::clockTick(SST::Cycle_t currentCycle) {
  executeScheduledEventsForCycle(currentCycle);

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
        fsmHandlers[current_fsm]();
        break;
      }
    }
  }

  return false;
}

void Dpu_2cycle_mac::handleDPU(
    const DPU_2CYCLE_MAC_PKG::DPUInstruction &instr) {
  out.output("dpu (slot=%d, option=%d, mode=%d, immediate=%d)\n", instr.slot,
             instr.option, instr.mode, instr.immediate);

  // replace the data buffer with the immediate value if needed
  if (instr.mode == DPU_2CYCLE_MAC_PKG::DPU_MODE::DPU_MODE_ADD_CONST ||
      instr.mode == DPU_2CYCLE_MAC_PKG::DPU_MODE::DPU_MODE_SUBT_ABS ||
      instr.mode == DPU_2CYCLE_MAC_PKG::DPU_MODE::DPU_MODE_MULT_CONST ||
      instr.mode == DPU_2CYCLE_MAC_PKG::DPU_MODE::DPU_MODE_MAX_MIN_CONST ||
      instr.mode == DPU_2CYCLE_MAC_PKG::DPU_MODE::DPU_MODE_LD_IR) {
    data_buffers[1] = uint64ToVector(instr.immediate);
  }

  // Add the reset event
  next_timing_states[current_fsm].addEvent(
      "dpu_reset_" + std::to_string(current_event_number), 5,
      [this] { accumulate_register.clear(); });

  // Add the event handler
  fsmHandlers[current_fsm] = DPU_Operations::getDPUHandler(
      this, (DPU_2CYCLE_MAC_PKG::DPU_MODE)instr.mode);
}

void Dpu_2cycle_mac::handleREP(
    const DPU_2CYCLE_MAC_PKG::REPInstruction &instr) {
  out.output("rep (slot=%d, port=%d, level=%d, iter=%d, step=%d, delay=%d)\n",
             instr.slot, instr.port, instr.level, instr.iter, instr.step,
             instr.delay);

  auto rep = instr;
  // For now, we only support increasing repetition levels (and no skipping)
  if (rep.level != port_last_rep_level[rep.port] + 1) {
    out.fatal(CALL_INFO, -1, "Invalid repetition level (last=%u, curr=%u)\n",
              port_last_rep_level[rep.port], rep.level);
  } else {
    port_last_rep_level[rep.port] = rep.level;
  }

  // add repetition to the timing model
  try {
    next_timing_states[0].addRepetition(rep.iter, rep.delay, rep.level,
                                        rep.step);
  } catch (const std::exception &e) {
    out.fatal(CALL_INFO, -1, "Failed to add repetition: %s\n", e.what());
  }
}

void Dpu_2cycle_mac::handleREPX(
    const DPU_2CYCLE_MAC_PKG::REPXInstruction &instr) {
  out.output("repx (slot=%d, port=%d, level=%d, iter=%d, step=%d, delay=%d)\n",
             instr.slot, instr.port, instr.level, instr.iter, instr.step,
             instr.delay);

  auto repx = instr;
  auto repetition_op =
      next_timing_states[0].getRepetitionOperatorFromLevel(repx.level);
  uint32_t iter = repx.iter << 6 | repetition_op.getIterations();
  uint32_t step = repx.step << 6 | repetition_op.getStep();
  uint32_t delay = repx.delay << 6 | repetition_op.getDelay();
  try {
    next_timing_states[0].adjustRepetition(iter, delay, repx.level, step);
  } catch (const std::exception &e) {
    out.fatal(CALL_INFO, -1, "REPX failed: %s\n", e.what());
  }
}

void Dpu_2cycle_mac::handleFSM(
    const DPU_2CYCLE_MAC_PKG::FSMInstruction &instr) {
  out.output("fsm (slot=%d, port=%d, delay_0=%d, delay_1=%d, delay_2=%d)\n",
             instr.slot, instr.port, instr.delay_0, instr.delay_1,
             instr.delay_2);

  auto fsm = instr;
  // add transition to the timing model
  try {
    next_timing_states[0].addTransition(
        fsm.delay_0, "event_" + std::to_string(current_event_number),
        [this, fsm] {
          out.output(" FSM switched to %d\n", fsm.port);
          current_fsm = fsm.port;
        });
    current_event_number++;
  } catch (const std::exception &e) {
    out.fatal(CALL_INFO, -1, "Failed to add transition: %s\n", e.what());
  }
}

void Dpu_2cycle_mac::handleOperation(
    std::string name, std::function<int64_t(int64_t, int64_t)> operation) {
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
}
