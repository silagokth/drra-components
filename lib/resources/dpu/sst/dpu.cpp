#include "dpu.h"
#include "dataEvent.h"
#include "dpu_operations.h"
#include "dpu_pkg.h"

using namespace SST;

Dpu::Dpu(SST::ComponentId_t id, SST::Params &params)
    : DRRAResource(id, params) {
  // Params
  fractional_bitwidth = params.find<uint32_t>("FRACTIONAL_BITWIDTH", 0);

  fsmHandlers.resize(num_fsms);
  imm_buffers.resize(num_fsms);
  dpuHandlers = DPU_Operations::createHandlers(this);
  instructionHandlers = DPU_PKG::createInstructionHandlers(this);
  // initialize current configuration options to 0
  for (int i = 0; i < 2; i++) {
    current_config_option[i] = 0;
    port_last_rep_level[i] = -1;
  }
  for (uint32_t i = 0; i < num_fsms; i++) {
    fsmHandlers[i] = dpuHandlers.at(DPU_PKG::CONF_MODE::CONF_MODE_IDLE);
  }

  // One tick per cycle instead of ten. The DPU only ever works at subcycle 9,
  // so drop the base 10x clock and wake on a self-link at that subcycle. A
  // clock cannot do this: SST aligns every clock to multiples of its period
  // (Clock::schedule), so a one-cycle period can only fire at subcycle 0.
  unregisterClock(tc, clockHandler);
  tick_link = configureSelfLink(
      "dpu_tick", tc, new Event::Handler2<Dpu, &Dpu::onCycleTick>(this));
}

// Links cannot be sent on before setup, so the first tick is primed here.
void Dpu::setup() { tick_link->send(9, new DpuTickEvent()); }

void Dpu::onCycleTick(SST::Event *event) {
  delete event;
  _currentSSTCycle = getCurrentSimTime(tc);
  const uint64_t current_cycle = _currentSSTCycle / 10;

  // Activations apply from the cycle after they arrive.
  if (!portsToActivate.empty()) {
    bool applied = false;
    for (auto it = portsToActivate.begin(); it != portsToActivate.end();) {
      if (activation_arrival_cycle[it->first] >= current_cycle) {
        ++it;
        continue;
      }
      activatePortsForSlot(it->first, it->second);
      current_config_option[it->first] = 0;
      activation_arrival_cycle.erase(it->first);
      it = portsToActivate.erase(it);
      applied = true;
    }
    if (applied) {
      last_config_level = -1;
      last_config_trans = -1;
    }
  }

  gatherEventsForCycle();
  executeEventsInPriorityRange(0, 8);

  // Data events queued anywhere in this cycle; the last one on a link wins,
  // as it did when every subcycle polled.
  for (int i = 0; i < resource_size; i++) {
    while (Event *data_event = data_links[i]->recv()) {
      handleEventWithSlotID(data_event, i);
      delete data_event;
    }
  }

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

  executeEventsInPriorityRange(9, 9);
  finishCycle(_currentSSTCycle);

  // Buffers were cleared at subcycle 0; clearing them after the operation is
  // the same thing, since nothing reads them until the next tick.
  for (int i = 0; i < resource_size; i++) {
    std::fill(data_buffers[i].begin(), data_buffers[i].end(), 0);
  }

  tick_link->send(10, new DpuTickEvent());
}

bool Dpu::clockTick(SST::Cycle_t currentCycle) {
  if (portsToActivate.size() > 0 && currentCycle % 10 == 0) {
    for (const auto &port : portsToActivate) {
      activatePortsForSlot(port.first, port.second);
      current_config_option[port.first] = 0;
    }
    portsToActivate.clear();
    last_config_level = -1;
    last_config_trans = -1;
  }

  if (currentCycle % 10 == 0) {
    for (int i = 0; i < resource_size; i++) {
      std::fill(data_buffers[i].begin(), data_buffers[i].end(), 0);
    }
  }

  // Deal with data events
  for (int i = 0; i < resource_size; i++) {
    Event *event = data_links[i]->recv();
    if (event) {
      handleEventWithSlotID(event, i);
      delete event;
    }
  }

  // Execute DPU operation at sub 9 BEFORE the base clockTick runs the
  // lifetime check (which would otherwise deactivate port 0 on its last
  // active cycle, causing the final MULT to be skipped).
  if (currentCycle % 10 == 9) {
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

  bool result = DRRAResource::clockTick(currentCycle);
  return result;
}

void Dpu::handleActivation(uint32_t slot_id, uint32_t ports) {
  portsToActivate[slot_id] = ports;
  activation_arrival_cycle[slot_id] = getCurrentSimTime(tc) / 10;
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

void Dpu::handleEVT(const DPU_PKG::EVTInstruction &instr) {
  out.output("evt (slot=%d, port=%d)\n", instr.slot, instr.port);

  switch (instr.port) {
  case DPU_PKG::EVT_PORT::EVT_PORT_DPU:
    agus[DPU_PKG::EVT_PORT::EVT_PORT_DPU].addEvent(
        "dpu_event_" +
            std::to_string(
                current_config_option[DPU_PKG::EVT_PORT::EVT_PORT_DPU]),
        [this] {});
    break;
  case DPU_PKG::EVT_PORT::EVT_PORT_RST:
    agus[DPU_PKG::EVT_PORT::EVT_PORT_RST].addEvent(
        "dpu_reset_" +
            std::to_string(
                current_config_option[DPU_PKG::EVT_PORT::EVT_PORT_RST]),
        [this] {
          out.output(" DPU accumulate register cleared\n");
          accumulate_register.clear();
        });
    break;
  default:
    out.fatal(CALL_INFO, -1, "Invalid EVT port: %d\n", instr.port);
    break;
  };
}

void Dpu::handleREP(const DPU_PKG::REPInstruction &instr) {
  out.output("rep (slot=%d, ext=%d, port=%s, iter=%d, step=%d, delay=%d)\n",
             instr.slot, instr.ext, instr.port == 0 ? "dpu" : "rst", instr.iter,
             instr.step, instr.delay);

  try {
    if (!instr.ext) {
      // base: add a new repetition (low half of iter/step/delay)
      agus[instr.port].addRepetition(instr.iter, instr.delay, instr.step);
    } else {
      // extension: fold the high bits into the last repetition
      auto repetition_op = agus[instr.port].getLastRepetitionOperator();
      uint32_t iter = instr.iter << DPU_PKG::DPU_INSTR_REP_ITER_BITWIDTH |
                      repetition_op.getIterations();
      uint32_t step = instr.step << DPU_PKG::DPU_INSTR_REP_STEP_BITWIDTH |
                      repetition_op.getStep();
      uint32_t delay = instr.delay << DPU_PKG::DPU_INSTR_REP_DELAY_BITWIDTH |
                       repetition_op.getDelay();
      agus[instr.port].adjustRepetition(iter, delay, step);
    }
  } catch (const std::exception &e) {
    out.fatal(CALL_INFO, -1, "REP failed: %s\n", e.what());
  }
}

void Dpu::handleTRANS(const DPU_PKG::TRANSInstruction &instr) {
  out.output("trans (slot=%d, port=%s, delay=%d)\n", instr.slot,
             instr.port == 0 ? "dpu" : "rst", instr.delay);

  // Add transition to the timing model
  try {
    agus[instr.port].addTransition(instr.delay);
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
