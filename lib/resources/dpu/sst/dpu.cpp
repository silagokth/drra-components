#include "dpu.h"
#include "dataEvent.h"
#include "dpu_operations.h"
#include <cmath>

using namespace SST;

DPU::DPU(SST::ComponentId_t id, SST::Params &params)
    : DRRAResource(id, params) {
  fsmHandlers.resize(num_fsms);
  dpuHandlers = DPU_Operations::createHandlers(this);
  instructionHandlers = DPU_PKG::createInstructionHandlers(this);
}

bool DPU::clockTick(SST::Cycle_t currentCycle) {
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

void DPU::handleEventWithSlotID(SST::Event *event, uint32_t slot_id) {
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

void DPU::handleREP(const DPU_PKG::REPInstruction &rep) {

  out.output("rep (slot=%d, port=%d, level=%d, iter=%d, step=%d, delay=%d)\n",
             rep.slot, rep.port, rep.level, rep.iter, rep.step, rep.delay);

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

void DPU::handleREPX(const DPU_PKG::REPXInstruction &repx) {
  auto repetition_op =
      next_timing_states[0].getRepetitionOperatorFromLevel(repx.level);
  uint32_t iter = repx.iter_msb << 6 | repetition_op.getIterations();
  uint32_t step = repx.step_msb << 6 | repetition_op.getStep();
  uint32_t delay = repx.delay_msb << 6 | repetition_op.getDelay();
  try {
    next_timing_states[0].adjustRepetition(iter, delay, repx.level, step);
  } catch (const std::exception &e) {
    out.fatal(CALL_INFO, -1, "REPX failed: %s\n", e.what());
  }
}

void DPU::handleFSM(const DPU_PKG::FSMInstruction &fsm) {
  // TODO: what are the use cases for delay_1 and delay_2?
  out.output("fsm (slot=%d, port=%d, delay_0=%d, delay_1=%d, delay_2=%d)\n",
             fsm.slot, fsm.port, fsm.delay_0, fsm.delay_1, fsm.delay_2);

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

void DPU::handleDPU(const DPU_PKG::DPUInstruction &dpu) {
  out.output("dpu (slot=%d, option=%u, mode=%u, immediate=%u)\n", dpu.slot,
             dpu.option, dpu.mode, dpu.immediate);

  // replace the data buffer with the immediate value if needed
  if (dpu.mode == DPU_PKG::DPU_MODE::ADD_CONST ||
      dpu.mode == DPU_PKG::DPU_MODE::SUBT_ABS ||
      dpu.mode == DPU_PKG::DPU_MODE::MULT_CONST ||
      dpu.mode == DPU_PKG::DPU_MODE::MAX_MIN_CONST ||
      dpu.mode == DPU_PKG::DPU_MODE::LD_IR) {
    data_buffers[1] = uint64ToVector(dpu.immediate);
  }

  // Add the reset event
  next_timing_states[current_fsm].addEvent(
      "dpu_reset_" + std::to_string(current_event_number), 5,
      [this] { accumulate_register.clear(); });

  // Add the event handler
  fsmHandlers[current_fsm] =
      DPU_Operations::getDPUHandler(this, (DPU_PKG::DPU_MODE)dpu.mode);
}

void DPU::handleOperation(std::string name,
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
}
