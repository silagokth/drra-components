#include "dpu_Q88.h"
#include <cmath>

using namespace SST;

DPUQ88::DPUQ88(SST::ComponentId_t id, SST::Params &params)
    : DRRAResource(id, params) {}

void DPUQ88::init(unsigned int phase) {
  out.verbose(CALL_INFO, 1, 0, "Initialized\n");
}

void DPUQ88::setup() {
  for (int i = 0; i < resource_size; i++) {
    for (int j = 0; j < word_bitwidth / 8; j++) {
      data_buffers[i].push_back(0);
    }
  }
}

void DPUQ88::complete(unsigned int phase) {}

void DPUQ88::finish() { out.verbose(CALL_INFO, 1, 0, "Finishing\n"); }

bool DPUQ88::clockTick(SST::Cycle_t currentCycle) {
  executeScheduledEventsForCycle(currentCycle);

  // Deal with data events
  for (int i = 0; i < resource_size; i++) {
    Event *event = data_links[i]->recv();
    if (event) {
      //i is slot_id, so if dpu, i=0, 1
      handleEventWithSlotID(event, i);
    }
  }

  // Execute DPU operation (priotity 5 - default)
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

void DPUQ88::handleEventWithSlotID(SST::Event *event, uint32_t slot_id) {
  DataEvent *dataEvent = dynamic_cast<DataEvent *>(event);
  if (dataEvent) {
    bool anyPortActive = false;
    for (const auto &port : active_ports) {
      if (isPortActive(port.first)) {
        anyPortActive = true;
        break;
      }
    }
    if (dataEvent->portType != DataEvent::PortType::WriteNarrow) {
      out.output("portID: %d\n", dataEvent->portType);
      out.fatal(CALL_INFO, -1, "Invalid port type\n");
    }
    
    // Store the data in the buffer of the slot
    out.output("Received data (slot=%d, size=%dbits, data=%s)\n", slot_id,
               dataEvent->size,
               formatRawDataToWords(dataEvent->payload).c_str());
    std::vector<uint8_t> data = dataEvent->payload;
    data_buffers[slot_id] = data;
  }
}

void DPUQ88::decodeInstr(uint32_t instr) {
  uint32_t instrOpcode = getInstrOpcode(instr);

  switch (instrOpcode) {
  case REP:
    handleRep(instr);
    break;
  case REPX:
    handleRepx(instr);
    break;
  case FSM:
    handleFSM(instr);
    break;
  case DPU_OP:
    handleDPU(instr);
    break;
  default:
    out.fatal(CALL_INFO, -1, "Invalid opcode: %u\n", instrOpcode);
  }
}

void DPUQ88::handleRep(uint32_t instr) {
  // Instruction fields
  uint32_t port = getInstrField(instr, 2, 22);
  uint32_t level = getInstrField(instr, 4, 18);
  uint32_t iter = getInstrField(instr, 6, 12);
  uint32_t step = getInstrField(instr, 6, 6);
  uint32_t delay = getInstrField(instr, 6, 0);

  out.output("rep (slot=%d, port=%d, level=%d, iter=%d, step=%d, delay=%d)\n",
             getInstrSlot(instr), port, level, iter, step, delay);

  // For now, we only support increasing repetition levels (and no skipping)
  if (level != port_last_rep_level[port] + 1) {
    out.fatal(CALL_INFO, -1, "Invalid repetition level (last=%u, curr=%u)\n",
              port_last_rep_level[port], level);
  } else {
    port_last_rep_level[port] = level;
  }

  // add repetition to the timing model
  try {
    next_timing_states[0].addRepetition(iter, delay, level, step);
  } catch (const std::exception &e) {
    out.fatal(CALL_INFO, -1, "Failed to add repetition: %s\n", e.what());
  }
}

void DPUQ88::handleRepx(uint32_t instr) {
  // Instruction fields
  uint32_t port = getInstrField(instr, 2, 22);
  uint32_t level = getInstrField(instr, 4, 18);
  uint32_t iter_msb = getInstrField(instr, 6, 12);
  uint32_t step_msb = getInstrField(instr, 6, 6);
  uint32_t delay_msb = getInstrField(instr, 6, 0);

  auto repetition_op =
      next_timing_states[0].getRepetitionOperatorFromLevel(level);
  uint32_t iter = iter_msb << 6 | repetition_op.getIterations();
  uint32_t step = step_msb << 6 | repetition_op.getStep();
  uint32_t delay = delay_msb << 6 | repetition_op.getDelay();
  try {
    next_timing_states[0].adjustRepetition(iter, delay, level, step);
  } catch (const std::exception &e) {
    out.fatal(CALL_INFO, -1, "REPX failed: %s\n", e.what());
  }
}

void DPUQ88::handleFSM(uint32_t instr) {
  // Instruction fields
  uint32_t port = getInstrField(instr, 3, 21);
  uint32_t delay_0 = getInstrField(instr, 7, 14);
  uint32_t delay_1 = getInstrField(instr, 7, 7);
  uint32_t delay_2 = getInstrField(instr, 7, 0);
  // TODO: what are the use cases for delay_1 and delay_2?

  // add transition to the timing model
  try {
    next_timing_states[0].addTransition(
        delay_0, "event_" + std::to_string(current_event_number), [this, port] {
          out.output(" FSM switched to %d\n", port);
          current_fsm = port;
        });
    current_event_number++;
  } catch (const std::exception &e) {
    out.fatal(CALL_INFO, -1, "Failed to add transition: %s\n", e.what());
  }
}

void DPUQ88::handleDPU(uint32_t instr) {
  uint32_t option = getInstrField(instr, 2, 22);
  DPU_MODE mode = (DPU_MODE)getInstrField(instr, 5, 17);
  uint64_t immediate = getInstrField(instr, 16, 1);
  //accumulate_register = 0;
  out.output("dpu (slot=%d, option=%u, mode=%u, immediate=%u)\n",
             getInstrSlot(instr), option, mode, immediate);

  // replace the data buffer with the immediate value if needed
  if (mode == DPU_MODE::ADD_CONST || mode == DPU_MODE::SUBT_ABS ||
      mode == DPU_MODE::MULT_CONST || mode == DPU_MODE::MAX_MIN_CONST ||
      mode == DPU_MODE::LD_IR) {
      int16_t im_16=(int16_t)(immediate& 0xFFFF);
      data_buffers[1] = int16ToVector(im_16);
  }

  // Add the reset event
  next_timing_states[current_fsm].addEvent(
      "dpu_reset_" + std::to_string(current_event_number), 5, [this] {
        accumulate_register = 0;
      });
  // Add the event handler
  fsmHandlers[current_fsm] = getDPUHandler(mode);
}

void DPUQ88::handleOperation(std::string name,
                          std::function<int16_t(int16_t, int16_t)> operation) {
  // Ensure both buffers exist
  if (data_buffers[0].size() == 0 || data_buffers[1].size() == 0) {
    out.fatal(CALL_INFO, -1, "Data buffers not found (data0=%d, data1=%d)\n",
              data_buffers[0].size(), data_buffers[1].size());
  }

  int16_t data0 = vectorToint16(data_buffers[0]);
  int16_t data1 = vectorToint16(data_buffers[1]);
  int16_t result = operation(data0, data1);

  DataEvent *dataEvent = new DataEvent(DataEvent::PortType::WriteNarrow);
  dataEvent->size = word_bitwidth;
  dataEvent->payload = int16ToVector(result);
  data_links[0]->send(dataEvent);

  out.output("DPU %s operation (in0=%d, in1=%d, out=%d)\n", name.c_str(),
             data0, data1, result);
}

