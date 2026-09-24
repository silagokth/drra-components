#include "drra_resource.h"
#include "activationEvent.h"
#include "instructionEvent.h"

#include <cmath>

using namespace SST;

DRRAResource::DRRAResource(ComponentId_t id, Params &params)
    : DRRAComponent(id, params) {
  // Resource-specific params
  slot_id = params.find<int32_t>("slot_id", -1);
  io_data_width = params.find<uint32_t>("io_data_width", 256);

  // Configure output
  out.setPrefix(getType() + " [" + std::to_string(cell_coordinates[0]) + "_" +
                std::to_string(cell_coordinates[1]) + "_" +
                std::to_string(slot_id) + "] - ");

  has_io_input_connection = params.find<bool>("has_io_input_connection", false);
  has_io_output_connection =
      params.find<bool>("has_io_output_connection", false);

  // Resource size
  resource_size = params.find<uint8_t>("resource_size", 1);
  for (uint8_t i = 0; i < resource_size; i++) {
    for (uint8_t j = 0; j < PORTS_PER_SLOT; j++) {
      active_ports[i * PORTS_PER_SLOT + j] = false;
      active_ports_cycles[i * PORTS_PER_SLOT + j] = 0;
      // next_timing_states[i * PORTS_PER_SLOT + j] = TimingState();
      // next_timing_states[i * PORTS_PER_SLOT + j].addEvent("event_0",
      //                                                     [this] {});
      port_last_rep_level[i * PORTS_PER_SLOT + j] = -1;
    }
  }
  num_fsms = resource_size * params.find<uint32_t>("fsm_per_slot", 4);

  // AGUs
  // num_agus = params.find<uint8_t>("num_agus", 1);
  num_agus = resource_size * params.find<uint32_t>("fsm_per_slot", 4);
  for (uint8_t i = 0; i < num_agus; i++) {
    agus[i] = DRRA_AGU();
  }

  // Initialize data buffers to zero
  for (int i = 0; i < resource_size; i++) {
    for (int j = 0; j < word_bitwidth / 8; j++) {
      data_buffers[i].push_back(0);
    }
  }

  // Annotate slot IDs
  for (uint8_t i = 0; i < resource_size; i++) {
    slot_ids.push_back(slot_id + i);
  }

  // Configure links
  for (uint8_t i = 0; i < resource_size; i++) {
    if (isPortConnected("controller_port" + std::to_string(i))) {
      controller_links.push_back(configureLink(
          "controller_port" + std::to_string(i),
          new Event::Handler2<DRRAResource, &DRRAResource::handleEventBase>(
              this)));
    } else {
      controller_links.push_back(nullptr);
    }

    if (isPortConnected("data_port" + std::to_string(i))) {
      data_links.push_back(configureLink("data_port" + std::to_string(i)));
      //, new Event::Handler<DRRAResource>(this,
      //&DRRAResource::handleEvent)));
    } else {
      data_links.push_back(nullptr);
    }
  }
  sst_assert(controller_links.size() == resource_size, CALL_INFO, -1,
             "Controller links size mismatch");
  sst_assert(data_links.size() == resource_size, CALL_INFO, -1,
             "Data links size mismatch");

  // Configure IO links
  if (has_io_input_connection) {
    if (isPortConnected("io_input_port")) {
      io_input_link = configureLink("io_input_port");
    }
  }
  if (has_io_output_connection) {
    if (isPortConnected("io_output_port")) {
      io_output_link = configureLink("io_output_port");
    }
  }

  // Write to trace file (only when debug/monitoring is enabled)
  if (debug_enabled) {
    trace_file.open(trace_name, std::ios::app);
    trace_file << "{\"name\": \"thread_name\", \"ph\": \"M\", \"pid\": 0, "
                  "\"tid\": 1"
               << std::setw(3) << std::setfill('0') << cell_coordinates[0]
               << std::setw(3) << std::setfill('0') << cell_coordinates[1]
               << std::setw(3) << std::setfill('0') << slot_id
               << ", \"args\": {\"name\": \"" << getType() << "\"}},\n";
    trace_file.close();
  }
}

bool DRRAResource::clockTick(Cycle_t currentCycle) {
  executeScheduledEventsForCycle(currentCycle);
  return false;
}

void DRRAResource::decodeInstr(uint32_t instr) {
  Instruction instruction(instr, format);
  uint32_t instrOpcode = instruction.opcode;
  instructionHandlers[instrOpcode](instr);
};

void DRRAResource::handleActivation(uint32_t slot_id, uint32_t ports) {
  activatePortsForSlot(slot_id, ports);
}

void DRRAResource::handleEventBase(Event *event) {
  // Controller events are handler-delivered (clock-independent); wake the clock
  // if the idle-skip paused it.
  ensureClockRunning();
  // Trace timestamps come from _currentSSTCycle, which a resource only
  // refreshes on its own tick. Resources that tick once per cycle would
  // otherwise stamp handler-delivered events with the previous tick's time.
  if (debug_enabled)
    _currentSSTCycle = getCurrentSimTime(tc);
  if (event) {
    // Check if the event is an ActEvent
    ActEvent *actEvent = dynamic_cast<ActEvent *>(event);
    if (actEvent) {
      for (uint32_t d = 0; d < ActEvent::NUM_LOOP_LEVELS; d++) {
        currentLoopVars[d] = actEvent->loop_vars[d];
      }
      handleActivation(actEvent->slot_id, actEvent->ports);
      logTraceEvent("activation", slot_id, true, 'X',
                    {{"ports", std::to_string(actEvent->ports)},
                     {"loop_var", std::to_string(actEvent->loop_vars[0])}});
      delete event;
      return;
    }

    // Check if the event is an InstrEvent
    InstrEvent *instrEvent = dynamic_cast<InstrEvent *>(event);
    if (instrEvent) {
      instrBuffer = instrEvent->instruction;
      Instruction instruction(instrBuffer);
      decodeInstr(instrBuffer);
      logTraceEvent("instruction", slot_id, true, 'X',
                    {{"instruction", instruction.toString()},
                     {"instruction_bin", instruction.toBinaryString()},
                     {"instruction_hex", instruction.toHexString()}});
      delete event;
      return;
    }
    // A handler-delivered event is owned by the receiver.
    delete event;
  }
}

void DRRAResource::activatePort(uint32_t port) {
  out.output("Activating port %d\n", port);
  active_ports[port] = true;
  out.output("Building AGU for port %d\n", port);
  if (agus[port].isEmpty()) {
    // RTL AGU with no explicit config still produces one default address
    // cycle. Model it with a single event so checkAGULifetime can retire the
    // port instead of leaving it active across epochs.
    agus[port].addEvent("default_act_" + std::to_string(port), [] {}, 1);
  }
  agus[port].build();
  // Resolve each offset term's loop level to its broadcast counter.
  agus[port].clearOffsetTerms();
  auto terms_it = portOffsetTerms.find(port);
  if (terms_it != portOffsetTerms.end()) {
    for (const auto &term : terms_it->second) {
      uint32_t level = term.second;
      if (level >= ActEvent::NUM_LOOP_LEVELS) {
        level = 0;
      }
      agus[port].addOffsetTerm(term.first, currentLoopVars[level]);
    }
  }
  port_last_rep_level[port] = -1;
  active_ports_cycles[port] = 0;
}

void DRRAResource::checkAGULifetime(Cycle_t currentSSTCycle) {
  // Check if AGUs should be disabled
  for (int i = 0; i < num_agus; i++) {
    if (isPortActive(i)) {
      if (agus[i].isEmpty())
        continue;

      uint64_t last_agu_cycle = agus[i].getLastScheduledCycle();
      Cycle_t current_active_cycle = getPortActiveCycle(i);
      out.output(
          "Checking AGU %d lifetime: current cycle %lu. AGU has been "
          "active for %lu. AGU should be active for %lu more cycles (%lu "
          "cycles in total).\n",
          i, currentSSTCycle / 10, current_active_cycle + 1,
          last_agu_cycle - current_active_cycle, last_agu_cycle + 1);
      if (current_active_cycle >= last_agu_cycle) {
        out.output("Deactivating port %d as AGU is inactive\n", i);
        active_ports[i] = false;
        active_ports_cycles[i] = 0;
        agus[i].reset();
        out.output("AGU %d disabled at cycle %lu\n", i, currentSSTCycle / 10);
      }
    }
  }
}

void DRRAResource::activatePortsForSlot(uint32_t slot_id, uint32_t ports) {
  uint8_t slot_pos = std::distance(
      slot_ids.begin(), std::find(slot_ids.begin(), slot_ids.end(), slot_id));
  for (uint8_t i = 0; i < PORTS_PER_SLOT; i++) {
    if ((ports & (1 << i)) >> i) {
      activatePort(slot_pos * PORTS_PER_SLOT + i);
      out.output("Activated port %d\n", slot_pos * PORTS_PER_SLOT + i);
    }
  }
}

void DRRAResource::gatherEventsForCycle() {
  {
    for (uint32_t port = 0; port < active_ports.size(); port++) {
      if (isPortActive(port)) { // if port is active
        auto events = getPortEventsForCycle(port, getPortActiveCycle(port));
        out.output(
            "Port %d has %lu events for cycle %lu (port active cycle %lu)\n",
            port, events.size(), _currentSSTCycle / 10,
            getPortActiveCycle(port));
        // add events to the list
        for (const auto &event : events) {
          events_for_cycle.push_back(event);
          corresponding_ports.push_back(port);
        }
      }
    }
    // if (events_for_cycle.size() > 0) {
    //   out.output("Events to execute for cycle %lu: %lu (", currentSSTCycle /
    //   10,
    //              events_for_cycle.size());
    //   for (int i = 0; i < events_for_cycle.size(); i++) {
    //     auto event = events_for_cycle[i];
    //     out.print("name %s,", event->getName().c_str());
    //     auto port = corresponding_ports[i];
    //     out.print("port %d prio %d", port, event->getPriority());
    //     if (i != events_for_cycle.size() - 1) {
    //       out.print(", ");
    //     }
    //   }
    //   out.print(")\n");
    // }
  }

}

// Executes in priority order, and in insertion order within one priority,
// which is the order the subcycle ticks produced.
void DRRAResource::executeEventsInPriorityRange(uint32_t lowest,
                                                uint32_t highest) {
  // Trace stamps stay on the subcycle an event belongs to, so a resource
  // running a whole cycle in one tick traces like the ten-tick path.
  const Cycle_t cycle_base = (_currentSSTCycle / 10) * 10;
  const Cycle_t saved_cycle = _currentSSTCycle;
  for (uint32_t priority = lowest; priority <= highest; priority++) {
    for (size_t i = 0; i < events_for_cycle.size(); i++) {
      auto event = events_for_cycle[i];
      auto port = corresponding_ports[i];
      if (event->getPriority() != priority)
        continue;
      out.output("Executing event port %d prio %d\n", port,
                 event->getPriority());
      event->execute();
      _currentSSTCycle = cycle_base + priority;
      logTraceEvent(event->getName(), slot_id, true, 'X',
                    {{"port", (int)port}, {"event", event->getName()}});
      _currentSSTCycle = saved_cycle;
    }
  }
}

void DRRAResource::finishCycle(Cycle_t currentSSTCycle) {
  checkAGULifetime(currentSSTCycle);
  for (uint32_t port = 0; port < active_ports.size(); port++) {
    if (isPortActive(port)) {
      incrementPortActiveCycle(port);
    }
  }
  events_for_cycle.clear();
  corresponding_ports.clear();
}

void DRRAResource::executeScheduledEventsForCycle(Cycle_t currentSSTCycle) {
  // if second subcycle of the cycle -> gather events for the cycle
  if (currentSSTCycle % 10 == 1) {
    gatherEventsForCycle();
  }

  executeEventsInPriorityRange(currentSSTCycle % 10, currentSSTCycle % 10);

  if (currentSSTCycle % 10 == 9) {
    finishCycle(currentSSTCycle);
  }
}

uint64_t DRRAResource::vectorToUint64(std::vector<uint8_t> data) {
  uint64_t result = 0;
  for (size_t i = 0; i < data.size(); i++) {
    result |= data[i] << (i * 8);
  }
  return result;
}

int64_t DRRAResource::vectorToInt64(std::vector<uint8_t> data) {
  uint64_t raw = 0;
  for (size_t i = 0; i < data.size(); i++) {
    raw |= static_cast<uint64_t>(data[i]) << (i * 8);
  }
  // Sign-extend from word_bitwidth to 64 bits. RTL DPU operand ports are
  // `logic signed` (see multiplier.sv.j2 / dpu.sv.j2), so an operand whose
  // MSB is set is negative. Symmetric with int64ToVector, which writes
  // signed-saturated values.
  if (word_bitwidth > 0 && word_bitwidth < 64) {
    uint64_t sign_bit = 1ULL << (word_bitwidth - 1);
    if (raw & sign_bit) {
      raw |= ~((1ULL << word_bitwidth) - 1);
    }
  }
  return static_cast<int64_t>(raw);
}

std::vector<uint8_t> DRRAResource::uint64ToVector(uint64_t data,
                                                  bool saturate) {
  std::vector<uint8_t> result;
  if (saturate) {
    if (data > pow(2, word_bitwidth) - 1) {
      data = pow(2, word_bitwidth) - 1;
    }
  }
  for (size_t i = 0; i < 8; i++) {
    result.push_back((data >> (i * 8)) & 0xFF);
    if (i == word_bitwidth / 8 - 1) {
      break;
    }
  }
  assert(result.size() == word_bitwidth / 8);
  return result;
}

std::vector<uint8_t> DRRAResource::int64ToVector(int64_t data, bool saturate) {
  std::vector<uint8_t> result;
  if (saturate) {
    if (data < 0) {
      if (data < -pow(2, word_bitwidth - 1)) {
        data = -pow(2, word_bitwidth - 1);
      }
    } else {
      if (data > pow(2, word_bitwidth - 1) - 1) {
        data = pow(2, word_bitwidth - 1) - 1;
      }
    }
  }
  for (size_t i = 0; i < 8; i++) {
    result.push_back((data >> (i * 8)) & 0xFF);
    if (i == word_bitwidth / 8 - 1) {
      break;
    }
  }
  assert(result.size() == word_bitwidth / 8);
  return result;
}
