#include "bu.h"

BU::BU(SST::ComponentId_t id, SST::Params &params) : DRRAResource(id, params) {}

void BU::init(unsigned int phase) {}

void BU::setup() {}

void BU::complete(unsigned int phase) {}

void BU::finish() {}

void BU::decodeInstr(uint32_t instr) {
  uint32_t instrOpcode = getInstrOpcode(instr);

  switch (instrOpcode) {
  case FSM:
    handleFSM(instr);
    break;
  case BU_OP:
    handleBU(instr);
    break;

  default:
    out.fatal(CALL_INFO, -1, "Invalid opcode: %u\n", instrOpcode);
  }
}

bool BU::clockTick(SST::Cycle_t currentCycle) {
  executeScheduledEventsForCycle(currentCycle);

  return false;
}

void BU::handleFSM(uint32_t instr) {
  // Instruction fields
  uint32_t port = getInstrField(instr, 3, 21);
  uint32_t delay_0 = getInstrField(instr, 7, 14);
  uint32_t delay_1 = getInstrField(instr, 7, 7);
  uint32_t delay_2 = getInstrField(instr, 7, 0);

  try {
    next_timing_states[0].addTransition(
        delay_0, "event_" + std::to_string(current_event_number), [this, port] {
          out.output("FSM switched to %d\n", port);
          current_fsm = port;
        });
    current_event_number++;
  } catch (const std::exception &e) {
    out.fatal(CALL_INFO, -1, "Failed to add transition: %s\n", e.what());
  }
}

void BU::handleBU(uint32_t instr) {
  uint32_t option = getInstrField(instr, 2, 22);

  BU::Radixes radix = (BU::Radixes)getInstrField(instr, 2, 20);
  switch (radix) {
  case RADIX_2:
    out.output("BU operation with radix 2\n");
    break;
  case RADIX_4:
    out.output("BU operation with radix 4\n");
    break;

  default:
    out.fatal(CALL_INFO, -1,
              "Radix mode not supported. Only RADIX_2 and RADIX_4 are "
              "currently implemented.\n");
  }

  BU::Decimation decimation = (BU::Decimation)getInstrField(instr, 1, 19);
  switch (decimation) {
  case DIT:
    out.output("BU operation with decimation-in-time\n");
    break;
  case DIF:
    out.output("BU operation with decimation-in-frequency\n");
    break;

  default:
    out.fatal(CALL_INFO, -1,
              "Decimation mode not supported. Only DIT and DIF are "
              "currently implemented.\n");
  }

  out.output("bu (slot=%d, option=%d, radix=%d, decimation=%d)\n",
             getInstrSlot(instr), option, radix, decimation);

  // Add the event handler
  fsmHandlers[current_fsm] = getBUHandler(radix, decimation);
}
