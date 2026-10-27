#include "dpu.h"
#include "dataEvent.h"
#include "dpu_operations.h"
#include "dpu_pkg.h"

using namespace SST;

Dpu::Dpu(SST::ComponentId_t id, SST::Params &params)
    : DRRAResource(id, params) {
  // Params
  fractional_bitwidth = params.find<uint32_t>("FRACTIONAL_BITWIDTH", 0);
  saturate = params.find<uint32_t>("SATURATE", 1) != 0;

  fsmHandlers.resize(num_fsms);
  fsmModes.assign(num_fsms, DPU_PKG::CONF_MODE::CONF_MODE_IDLE);
  dpuHandlers = DPU_Operations::createHandlers(this);
  instructionHandlers = DPU_PKG::createInstructionHandlers(this);
  for (uint32_t i = 0; i < num_fsms; i++) {
    fsmHandlers[i] = dpuHandlers.at(DPU_PKG::CONF_MODE::CONF_MODE_IDLE);
  }

  // The DPU port only selects the FSM, which logic() reads straight off the
  // AGU, so it registers no action. The reset port does have one. It only
  // records the clear: logic() applies it now for a single-cycle mode, or a
  // cycle later for a MAC product, which reaches the accumulator then.
  registerPortAction(DPU_PKG::EVT_PORT::EVT_PORT_RST, 5, "dpu_reset",
                     [this](int64_t) { reset_requested = true; });
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
      delete event;
    }
  }

  if (subcycle != 9)
    return;

  out.output(" Current FSM: %u\n", current_fsm);
  out.output(" fsmHandlers size: %lu\n", fsmHandlers.size());

  // Mirrors logic.sv.j2: stage 2 of last cycle's MAC goes first and uses the
  // clear delayed with its product. It holds the adder, the accumulator and
  // out0, so a non-MAC mode issued in the same cycle is dropped; a further
  // MAC's stage 1 still runs.
  bool reset_now = reset_requested;
  reset_requested = false;
  bool stage2 = mac_pending;
  mac_pending = false;
  if (stage2) {
    if (mac_reset_pending) {
      out.output(" DPU accumulate register cleared\n");
      accumulate_register.clear();
    }
    finishMAC();
  }
  mac_reset_pending = reset_now;

  if (fsmModes[current_fsm] == DPU_PKG::CONF_MODE::CONF_MODE_MAC) {
    // Stage 1. A clear now is for this product and is applied in stage 2.
    fsmHandlers[current_fsm]();
  } else if (stage2) {
    if (fsmModes[current_fsm] != DPU_PKG::CONF_MODE::CONF_MODE_IDLE)
      out.output(" DPU mode %d dropped: the cycle belongs to MAC stage 2\n",
                 static_cast<int>(fsmModes[current_fsm]));
  } else {
    if (reset_now) {
      out.output(" DPU accumulate register cleared\n");
      accumulate_register.clear();
    }
    fsmHandlers[current_fsm]();
  }

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

  // Add the event handler to the config index
  fsmHandlers[instr.option] =
      DPU_Operations::getDPUHandler(this, (DPU_PKG::CONF_MODE)instr.mode);
  fsmModes[instr.option] = (DPU_PKG::CONF_MODE)instr.mode;
}

void Dpu::sendOut1(int64_t value) {
  if (data_links.size() < 2 || data_links[1] == nullptr)
    return;
  DataEvent *dataEvent = new DataEvent(DataEvent::PortType::WriteNarrow);
  dataEvent->size = word_bitwidth;
  dataEvent->payload = int64ToVector(value);
  data_links[1]->send(dataEvent);
  out.output("DPU out1=%ld\n", value);
}

void Dpu::readOperands(int64_t &data0, int64_t &data1) {
  // Ensure both buffers exist
  if (data_buffers[0].size() == 0 || data_buffers[1].size() == 0) {
    out.fatal(CALL_INFO, -1, "Data buffers not found (data0=%lu, data1=%lu)\n",
              data_buffers[0].size(), data_buffers[1].size());
  }

  data0 = vectorToInt64(data_buffers[0]);
  data1 = vectorToInt64(data_buffers[1]);
}

void Dpu::issueMACProduct(int64_t product) {
  mac_pending = true;
  mac_product = product;
}

void Dpu::finishMAC() {
  int64_t acc = accumulate_register.size() > 0
                    ? vectorToInt64(accumulate_register)
                    : 0;
  int64_t result = saturate
                       ? DPU_Operations::add_sat(acc, mac_product)
                       : DPU_Operations::wrap(acc + mac_product, word_bitwidth);
  accumulate_register = int64ToVector(result);

  DataEvent *dataEvent = new DataEvent(DataEvent::PortType::WriteNarrow);
  dataEvent->size = word_bitwidth;
  dataEvent->payload = int64ToVector(result);
  data_links[0]->send(dataEvent);

  out.output("DPU MAC stage 2 (product=%ld, out=%ld)\n", mac_product, result);
  logTraceEvent("operation", slot_id, true, 'X',
                {{"product", static_cast<int>(mac_product)},
                 {"result", static_cast<int>(result)},
                 {"accumulator", static_cast<int>(result)}});
}

void Dpu::handleOperation(std::string name,
                          std::function<int64_t(int64_t, int64_t)> operation) {
  int64_t data0, data1;
  readOperands(data0, data1);
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
