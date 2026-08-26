#include "dpu.h"
#include "dataEvent.h"
#include "dpu_operations.h"
#include "dpu_pkg.h"

using namespace SST;

Dpu::Dpu(SST::ComponentId_t id, SST::Params &params)
    : DRRAResource(id, params) {
  fsmHandlers.resize(num_fsms);
  imm_buffers.resize(num_fsms);
  dpuHandlers = DPU_Operations::createHandlers(this);
  instructionHandlers = DPU_PKG::createInstructionHandlers(this);
  for (uint32_t i = 0; i < num_fsms; i++) {
    fsmHandlers[i] = dpuHandlers.at(DPU_PKG::CONF_MODE::CONF_MODE_IDLE);
  }

  // The DPU port only selects the FSM, which logic() reads straight off the
  // AGU, so it registers no action. The reset port does have one.
  registerPortAction(DPU_PKG::EVT_PORT::EVT_PORT_RST, 5, "dpu_reset",
                     [this](int64_t) {
                       out.output(" DPU accumulate register cleared\n");
                       accumulate_register.clear();
                     });
}

void Dpu::logic(uint32_t subcycle) {
  DRRAResource::logic(subcycle);

  if (subcycle == 0) {
    for (int i = 0; i < resource_size; i++) {
      std::fill(data_buffers[i].begin(), data_buffers[i].end(), 0);
    }
  }

  // Operands can land at any subcycle: the producing resource reads early in
  // the cycle and the switchbox forwards in its link handler.
  for (int i = 0; i < resource_size; i++) {
    Event *event = data_links[i]->recv();
    if (event) {
      handleEventWithSlotID(event, i);
    }
  }

  if (subcycle != 9)
    return;

  out.output(" Current FSM: %u\n", current_fsm);
  out.output(" fsmHandlers size: %lu\n", fsmHandlers.size());
  fsmHandlers[current_fsm]();

  // Update the FSM for the next execution from the AGU output (one cycle
  // delayed). Port 0 is still active here even on its last scheduled cycle:
  // the AGU array retires a port at subcycle 2 of the following cycle.
  if (isPortAddressValid(DPU_PKG::EVT_PORT::EVT_PORT_DPU)) {
    int64_t agu_address = getPortAddress(DPU_PKG::EVT_PORT::EVT_PORT_DPU);
    if (agu_address != current_fsm) {
      current_fsm = agu_address;
      out.output(" FSM switched to FSM #%u\n", current_fsm);
    }
  }
}

void Dpu::handleEventWithSlotID(SST::Event *event, uint32_t slot_id) {
  DataEvent *dataEvent = dynamic_cast<DataEvent *>(event);
  if (dataEvent) {
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

void Dpu::handleCONF(const DPU_PKG::CONFInstruction &instr) {
  out.output("conf (slot=%d, option=%d, mode=%d, immediate=%d)\n", instr.slot,
             instr.option, instr.mode, instr.immediate);

  // Add the immediate value to the imm_buffers if the mode requires it
  if (instr.mode == DPU_PKG::CONF_MODE::CONF_MODE_ADD_CONST ||
      instr.mode == DPU_PKG::CONF_MODE::CONF_MODE_SUBT_ABS ||
      instr.mode == DPU_PKG::CONF_MODE::CONF_MODE_MULT_CONST ||
      instr.mode == DPU_PKG::CONF_MODE::CONF_MODE_MAX_MIN_CONST ||
      instr.mode == DPU_PKG::CONF_MODE::CONF_MODE_LD_IR) {
    imm_buffers[instr.option] = uint64ToVector(instr.immediate);
  }

  // Add the event handler to the config index
  fsmHandlers[instr.option] =
      DPU_Operations::getDPUHandler(this, (DPU_PKG::CONF_MODE)instr.mode);
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
