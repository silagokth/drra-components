#pragma once

#include "drra_agu.h"
#include "drra_component.h"
#include "timingModel.h"

#include <algorithm>
#include <sst/core/link.h>
#include <sst/core/params.h>
#include <sst/core/timeConverter.h>

using namespace SST;

class DRRAResource : public DRRAComponent {
public:
  DRRAResource(ComponentId_t id, Params &params);

  virtual ~DRRAResource() {}

  virtual bool clockTick(Cycle_t currentCycle) override;

  virtual void decodeInstr(uint32_t instr);

  virtual void handleActivation(uint32_t slot_id, uint32_t ports);

  void handleEventBase(Event *event);

protected:
  static inline std::vector<SST::ElementInfoParam> getBaseParams() {
    std::vector<SST::ElementInfoParam> params = DRRAComponent::getBaseParams();
    params.push_back({"slot_id",
                      "Slot ID for the resource. If the resource occupies "
                      "multiple slots, slot ID of the first slot occupied.",
                      "-1"});
    params.push_back(
        {"resource_size",
         "Number of slots occupied by the resource (default to 1 slot)", "1"});
    params.push_back({"num_agus", "Number of AGUs in the resource", "1"});
    params.push_back(
        {"has_io_input_connection", "Has IO input connection", "0"});
    params.push_back(
        {"has_io_output_connection", "Has IO output connection", "0"});
    return params;
  }

  static inline std::vector<SST::ElementInfoPort> getBasePorts() {
    std::vector<SST::ElementInfoPort> ports;
    ports.push_back(
        {"controller_port%(portnum)d", "Link to the controller port", {""}});
    ports.push_back({"data_port%(portnum)d", "Link to the data port", {""}});
    ports.push_back({"io_input_port", "Link to the IO input port", {""}});
    ports.push_back({"io_output_port", "Link to the IO output port", {""}});
    return ports;
  }

  static inline std::vector<SST::ElementInfoStatistic> getBaseStatistics() {
    std::vector<SST::ElementInfoStatistic> stats =
        DRRAComponent::getBaseStatistics();
    return stats;
  }

  bool isPortActive(uint32_t port) { return active_ports[port]; }

  void activatePort(uint32_t port);

  void activatePortsForSlot(uint32_t slot_id, uint32_t ports);

  void checkAGULifetime(Cycle_t currentCycle);

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
    return agus[port].getEventsForCycle(cycle);
    // return {};
  }

  void executeScheduledEventsForCycle(Cycle_t currentSSTCycle);

  uint64_t vectorToUint64(std::vector<uint8_t> data);
  int64_t vectorToInt64(std::vector<uint8_t> data);
  std::vector<uint8_t> uint64ToVector(uint64_t data, bool saturate = true);
  std::vector<uint8_t> int64ToVector(int64_t data, bool saturate = true);

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
  int32_t slot_id;
  std::vector<int32_t> slot_ids;
  uint8_t resource_size = 1; // Default to 1 slot
  uint32_t num_fsms;

  // Data buffers
  std::map<uint32_t, std::vector<uint8_t>> data_buffers;

  // Timing model state for each ports
  // std::map<uint32_t, TimingState> current_timing_states;
  // std::map<uint32_t, TimingState> next_timing_states;

  // AGUs
  uint8_t num_agus;
  std::map<uint32_t, DRRA_AGU> agus;
  std::map<uint32_t, DRRA_AGU> next_agus;

  // std::map<std::string, std::function<void()>> events_handlers_map;
  std::map<uint32_t, int32_t> port_last_rep_level;
};
