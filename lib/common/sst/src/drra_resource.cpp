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
  num_fsms = resource_size * params.find<uint32_t>("fsm_per_slot", 4);

  // One AGU per port. The AGU index is the relative port number, so
  // getRelativePortNum() is the only place (slot, port) is mapped to an AGU.
  num_agus = resource_size * params.find<uint32_t>("fsm_per_slot", 4);
  agu_array.init(num_agus, &out);

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

// The subcycle contract. See the comment on SUB_ACTIVATE in drra_resource.h.
bool DRRAResource::clockTick(Cycle_t currentCycle) {
  const uint32_t subcycle = currentCycle % 10;

  switch (subcycle) {
  case SUB_ACTIVATE:
    applyPendingActivations();
    break;
  case SUB_INSTRUCTION:
    drainInstructionPort();
    break;
  case SUB_AGU_UPDATE:
    agu_array.update();
    break;
  default:
    break;
  }

  logic(subcycle);
  return false;
}

void DRRAResource::applyPendingActivations() {
  if (portsToActivate.empty())
    return;
  for (const auto &[slot, ports] : portsToActivate) {
    activatePortsForSlot(slot, ports);
  }
  portsToActivate.clear();
}

void DRRAResource::drainInstructionPort() {
  while (!instruction_queue.empty()) {
    uint32_t instr = instruction_queue.front();
    instruction_queue.pop_front();

    instrBuffer = instr;
    decodeInstr(instr);

    if (debug_enabled) {
      Instruction instruction(instr);
      logTraceEvent("instruction", slot_id, true, 'X',
                    {{"instruction", instruction.toString()},
                     {"instruction_bin", instruction.toBinaryString()},
                     {"instruction_hex", instruction.toHexString()}});
    }
  }
}

void DRRAResource::logic(uint32_t subcycle) {
  for (const PortAction &action : port_actions) {
    if (action.subcycle != subcycle)
      continue;
    if (!agu_array.addrValid(action.port))
      continue;

    const int64_t address = agu_array.addr(action.port);
    if (action.max_address != 0 &&
        static_cast<uint64_t>(address) >= action.max_address) {
      out.fatal(CALL_INFO, -1,
                "Invalid AGU address %ld on port %u (%s), max %lu\n",
                static_cast<long>(address), action.port, action.name.c_str(),
                action.max_address);
    }

    out.output("Port %u: %s @ %ld\n", action.port, action.name.c_str(),
               static_cast<long>(address));
    action.action(address);

    logTraceEvent(action.name, slot_id, true, 'X',
                  {{"port", static_cast<int>(action.port)},
                   {"address", static_cast<int>(address)}});
  }
}

void DRRAResource::registerPortAction(uint32_t port, uint32_t subcycle,
                                      const std::string &name,
                                      std::function<void(int64_t)> action,
                                      uint64_t max_address) {
  if (subcycle < SUB_FIRST_ACTION || subcycle > 9) {
    out.fatal(CALL_INFO, -1,
              "Port action '%s' registered at subcycle %u: AGU addresses are "
              "only stable from subcycle %u\n",
              name.c_str(), subcycle, SUB_FIRST_ACTION);
  }
  if (port >= agu_array.size()) {
    out.fatal(CALL_INFO, -1,
              "Port action '%s' on port %u, but the resource has %u AGUs\n",
              name.c_str(), port, agu_array.size());
  }
  port_actions.push_back({port, subcycle, name, std::move(action),
                          max_address});
}

void DRRAResource::forbidPort(uint32_t port, const std::string &reason) {
  forbidden_ports[port] = reason;
}

void DRRAResource::decodeInstr(uint32_t instr) {
  Instruction instruction(instr, format);
  uint32_t instrOpcode = instruction.opcode;
  instructionHandlers[instrOpcode](instr);
};

void DRRAResource::handleActivation(uint32_t slot_id, uint32_t ports) {
  portsToActivate[slot_id] = ports;
}

void DRRAResource::handleEventBase(Event *event) {
  // Controller events are handler-delivered (clock-independent); wake the clock
  // if the idle-skip paused it.
  ensureClockRunning();
  if (!event)
    return;

  // Check if the event is an ActEvent
  ActEvent *actEvent = dynamic_cast<ActEvent *>(event);
  if (actEvent) {
    handleActivation(actEvent->slot_id, actEvent->ports);
    logTraceEvent("activation", slot_id, true, 'X',
                  {{"ports", std::to_string(actEvent->ports)}});
    return;
  }

  // Check if the event is an InstrEvent. The instruction port is registered:
  // the instruction is buffered here and decoded at SUB_INSTRUCTION, so decode
  // never races with a clock handler at the same simulated time.
  InstrEvent *instrEvent = dynamic_cast<InstrEvent *>(event);
  if (instrEvent) {
    instruction_queue.push_back(instrEvent->instruction);
    return;
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

void DRRAResource::handleEVT(uint32_t slot, uint32_t port, uint32_t option,
                             uint64_t init_addr) {
  uint32_t port_num = getRelativePortNum(slot, port);
  out.output("evt (slot=%u, port=%u -> agu %u, option=%u, init_addr=%lu)\n",
             slot, port, port_num, option, init_addr);

  auto forbidden = forbidden_ports.find(port_num);
  if (forbidden != forbidden_ports.end()) {
    out.fatal(CALL_INFO, -1, "EVT on unsupported port %u: %s\n", port_num,
              forbidden->second.c_str());
  }

  try {
    agu_array.evt(port_num, init_addr);
  } catch (const std::exception &e) {
    out.fatal(CALL_INFO, -1, "EVT failed: %s\n", e.what());
  }
}

void DRRAResource::handleREP(uint32_t slot, uint32_t port, bool ext,
                             uint32_t iter, uint32_t step, uint32_t delay,
                             uint32_t iter_bits, uint32_t step_bits,
                             uint32_t delay_bits) {
  uint32_t port_num = getRelativePortNum(slot, port);
  out.output("rep (slot=%d, ext=%d, port=%d, iter=%d, step=%d, delay=%d)\n",
             slot, ext, port_num, iter, step, delay);

  try {
    agu_array.rep(port_num, ext, iter, step, delay, iter_bits, step_bits,
                  delay_bits);
  } catch (const std::exception &e) {
    out.fatal(CALL_INFO, -1, "REP failed: %s\n", e.what());
  }
}

void DRRAResource::handleTRANS(uint32_t slot, uint32_t port, uint32_t delay) {
  uint32_t port_num = getRelativePortNum(slot, port);
  out.output("trans (slot=%d, port=%d, delay=%d)\n", slot, port_num, delay);

  try {
    agu_array.trans(port_num, delay);
  } catch (const std::exception &e) {
    out.fatal(CALL_INFO, -1, "Failed to add transition: %s\n", e.what());
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
  int64_t result = 0;
  for (size_t i = 0; i < data.size(); i++) {
    result |= data[i] << (i * 8);
  }
  return result;
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
