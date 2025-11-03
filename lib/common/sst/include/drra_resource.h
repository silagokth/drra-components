
#include "activationEvent.h"
#include "drra_component.h"
#include "instructionEvent.h"
#include "timingModel.h"

#include <cmath>

#include <sst/core/link.h>
#include <sst/core/params.h>
#include <sst/core/timeConverter.h>
#include <string>

using namespace SST;

class DRRAResource : public DRRAComponent {
public:
  DRRAResource(ComponentId_t id, Params &params) : DRRAComponent(id, params) {
    // Resource-specific params
    slot_id = params.find<int16_t>("slot_id", -1);
    io_data_width = params.find<uint32_t>("io_data_width", 256);

    // Configure output
    out.setPrefix(getType() + " [" + std::to_string(cell_coordinates[0]) + "_" +
                  std::to_string(cell_coordinates[1]) + "_" +
                  std::to_string(slot_id) + "] - ");

    has_io_input_connection =
        params.find<bool>("has_io_input_connection", false);
    has_io_output_connection =
        params.find<bool>("has_io_output_connection", false);

    // Resource size
    resource_size = params.find<uint8_t>("resource_size", 1);
    for (uint8_t i = 0; i < resource_size; i++) {
      for (uint8_t j = 0; j < PORTS_PER_SLOT; j++) {
        active_ports[i * PORTS_PER_SLOT + j] = false;
        active_ports_cycles[i * PORTS_PER_SLOT + j] = 0;
        next_timing_states[i * PORTS_PER_SLOT + j] = TimingState();
        next_timing_states[i * PORTS_PER_SLOT + j].addEvent("event_0",
                                                            [this] {});
        port_last_rep_level[i * PORTS_PER_SLOT + j] = -1;
      }
    }
    num_fsms = resource_size * params.find<uint32_t>("fsm_per_slot", 4);

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

    // Write to trace file
    trace_file.open(trace_name, std::ios::app);
    trace_file << "{\"name\": \"thread_name\", \"ph\": \"M\", \"pid\": 0, "
                  "\"tid\": 1"
               << std::setw(3) << std::setfill('0') << cell_coordinates[0]
               << std::setw(3) << std::setfill('0') << cell_coordinates[1]
               << std::setw(3) << std::setfill('0') << slot_id
               << ", \"args\": {\"name\": \"" << getType() << "\"}},\n";
    trace_file.close();
  }

  virtual ~DRRAResource() {}

  virtual bool clockTick(Cycle_t currentCycle) override {
    executeScheduledEventsForCycle(currentCycle);

    return false;
  }

  virtual void decodeInstr(uint32_t instr) {
    Instruction instruction(instr, format);
    uint32_t instrOpcode = instruction.opcode;
    instructionHandlers[instrOpcode](instr);
  };

  virtual void handleActivation(uint32_t slot_id, uint32_t ports) {};

  void handleEventBase(Event *event) {
    if (event) {
      // Check if the event is an ActEvent
      ActEvent *actEvent = dynamic_cast<ActEvent *>(event);
      if (actEvent) {
        activatePortsForSlot(actEvent->slot_id, actEvent->ports);
        handleActivation(actEvent->slot_id, actEvent->ports);
        logTraceEvent("activation", slot_id, true,
                      {{"ports", std::to_string(actEvent->ports)}});
        return;
      }

      // Check if the event is an InstrEvent
      InstrEvent *instrEvent = dynamic_cast<InstrEvent *>(event);
      if (instrEvent) {
        instrBuffer = instrEvent->instruction;
        Instruction instruction(instrBuffer);
        decodeInstr(instrBuffer);
        logTraceEvent("instruction", slot_id, true,
                      {{"instruction", instruction.toString()},
                       {"instruction_bin", instruction.toBinaryString()},
                       {"instruction_hex", instruction.toHexString()}});
        return;
      }
    }
  }

protected:
  static std::vector<SST::ElementInfoParam> getBaseParams() {
    std::vector<SST::ElementInfoParam> params = DRRAComponent::getBaseParams();
    params.push_back({"slot_id",
                      "Slot ID for the resource. If the resource occupies "
                      "multiple slots, slot ID of the first slot occupied.",
                      "-1"});
    params.push_back(
        {"resource_size",
         "Number of slots occupied by the resource (default to 1 slot)", "1"});
    params.push_back(
        {"has_io_input_connection", "Has IO input connection", "0"});
    params.push_back(
        {"has_io_output_connection", "Has IO output connection", "0"});
    return params;
  }

  static std::vector<SST::ElementInfoPort> getBasePorts() {
    std::vector<SST::ElementInfoPort> ports;
    ports.push_back(
        {"controller_port%(portnum)d", "Link to the controller port", {""}});
    ports.push_back({"data_port%(portnum)d", "Link to the data port", {""}});
    ports.push_back({"io_input_port", "Link to the IO input port", {""}});
    ports.push_back({"io_output_port", "Link to the IO output port", {""}});
    return ports;
  }

  static std::vector<SST::ElementInfoStatistic> getBaseStatistics() {
    std::vector<SST::ElementInfoStatistic> stats =
        DRRAComponent::getBaseStatistics();
    return stats;
  }

  bool isPortActive(uint32_t port) { return active_ports[port]; }

  void activatePort(uint32_t port) {
    active_ports[port] = true;
    current_timing_states[port] = next_timing_states[port];
    next_timing_states[port] = TimingState();
    next_timing_states[port].addEvent("event_0", [this] {});
    current_timing_states[port].build();
    // out.output("port %d timing: %s\n", port,
    //            current_timing_states[port].toString().c_str());
    port_last_rep_level[port] = -1;
    active_ports_cycles[port] = 0;
  }

  void activatePortsForSlot(uint32_t slot_id, uint32_t ports) {
    uint8_t slot_pos = std::distance(
        slot_ids.begin(), std::find(slot_ids.begin(), slot_ids.end(), slot_id));
    for (uint8_t i = 0; i < PORTS_PER_SLOT; i++) {
      if ((ports & (1 << i)) >> i) {
        activatePort(slot_pos * PORTS_PER_SLOT + i);
        out.output("Activated port %d\n", slot_pos * PORTS_PER_SLOT + i);
      }
    }
  }

  uint32_t getRelativePortNum(uint32_t slot_id, uint32_t port_id) {
    uint8_t slot_pos = std::distance(
        slot_ids.begin(), std::find(slot_ids.begin(), slot_ids.end(), slot_id));
    return slot_pos * PORTS_PER_SLOT + port_id;
  }

  uint32_t getPortActiveCycle(uint32_t port) {
    return active_ports_cycles[port];
  }

  void incrementPortActiveCycle(uint32_t port) { active_ports_cycles[port]++; }

  std::set<std::shared_ptr<const TimingEvent>>
  getPortEventsForCycle(uint32_t port, uint32_t cycle) {
    return current_timing_states[port].getEventsForCycle(cycle);
  }

  void executeScheduledEventsForCycle(Cycle_t currentSSTCycle) {
    // if second subcycle of the cycle -> gather events for the cycle
    if (currentSSTCycle % 10 == 1) {
      for (auto &port : active_ports) { // for each port
        if (isPortActive(port.first)) { // if port is active
          auto events =
              getPortEventsForCycle(port.first, getPortActiveCycle(port.first));

          // add events to the list
          for (auto event : events) {
            events_for_cycle.push_back(event);
            corresponding_ports.push_back(port.first);
          }
        }
      }
      if (events_for_cycle.size() > 0) {
        out.output("Events to execute for cycle %lu: %lu (",
                   currentSSTCycle / 10, events_for_cycle.size());
        for (int i = 0; i < events_for_cycle.size(); i++) {
          auto event = events_for_cycle[i];
          auto port = corresponding_ports[i];
          out.print("port %d prio %d", port, event->getPriority());
          if (i != events_for_cycle.size() - 1) {
            out.print(", ");
          }
        }
        out.print(")\n");
      }
    }

    // execute events with priority equal to the current subcycle
    for (size_t i = 0; i < events_for_cycle.size(); i++) {
      auto event = events_for_cycle[i];
      auto port = corresponding_ports[i];
      if (event->getPriority() == currentSSTCycle % 10) {
        // out.output("Executing event port %d prio %d\n", port,
        //            event->getPriority());
        event->execute();
        if (trace_name != "") {
          logTraceEvent(event->getName(), slot_id, true,
                        {{"port", (int)port}, {"event", event->getName()}});
        }
        current_timing_states[port].incrementLevels();
        // out.output("port %d incremented levels\n", port);
      }
    }

    if (currentSSTCycle % 10 == 9) {
      for (auto &port : active_ports) {
        if (isPortActive(port.first)) {
          // out.output("incrementing port %d active cycle (old: %lu)\n",
          //            port.first, getPortActiveCycle(port.first));
          incrementPortActiveCycle(port.first);
        }
      }
      events_for_cycle.clear();
      corresponding_ports.clear();
    }
  }

  uint64_t vectorToUint64(std::vector<uint8_t> data) {
    uint64_t result = 0;
    for (size_t i = 0; i < data.size(); i++) {
      result |= data[i] << (i * 8);
    }
    return result;
  }

  int64_t vectorToInt64(std::vector<uint8_t> data) {
    int64_t result = 0;
    for (size_t i = 0; i < data.size(); i++) {
      result |= data[i] << (i * 8);
    }
    return result;
  }

  std::vector<uint8_t> uint64ToVector(uint64_t data, bool saturate = true) {
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

  std::vector<uint8_t> int64ToVector(int64_t data, bool saturate = true) {
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

  // Links
  std::vector<Link *> controller_links;
  std::vector<Link *> data_links;
  Link *io_input_link = nullptr;
  Link *io_output_link = nullptr;

  // Activation
  std::map<uint32_t, bool> active_ports;
  std::map<uint32_t, uint32_t> active_ports_cycles;

  // Event execution
  std::vector<std::shared_ptr<const TimingEvent>> events_for_cycle;
  std::vector<uint32_t> corresponding_ports;

  // IO settings
  uint32_t io_data_width; // in bits
  bool has_io_input_connection, has_io_output_connection;

  // Slot settings
  int16_t slot_id;
  std::vector<int16_t> slot_ids;
  uint8_t resource_size = 1; // Default to 1 slot
  uint32_t num_fsms;

  // Data buffers
  std::map<uint32_t, std::vector<uint8_t>> data_buffers;

  // Timing model state for each ports
  std::map<uint32_t, TimingState> current_timing_states;
  std::map<uint32_t, TimingState> next_timing_states;
  // std::map<std::string, std::function<void()>> events_handlers_map;
  std::map<uint32_t, int32_t> port_last_rep_level;
};
