#pragma once

#include "drra_agu.h"
#include "drra_component.h"
#include "port_register.h"
#include "timingModel.h"

#include <algorithm>
#include <map>
#include <sst/core/link.h>
#include <sst/core/params.h>
#include <sst/core/timeConverter.h>
#include <utility>

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
    params.push_back(
        {"number_of_fsms",
         "Number of control/activation ports per slot (the resource's resolved "
         "FSM_PER_SLOT -- the width of the `activate` wire). This is NOT the "
         "AGU count and NOT the config-table depth; those are separate concepts "
         "(NUM_AGUS and NUM_CONFIGS). Vesyla's SST conf emits it as "
         "\"number_of_fsms\" and also passes \"FSM_PER_SLOT\" verbatim; the base "
         "reads number_of_fsms, then FSM_PER_SLOT, then this default. num_fsms "
         "is this scaled by resource_size.",
         "4"});
    params.push_back(
        {"NUM_CONFIGS",
         "Config-table depth: how many stored configurations the instruction's "
         "`option`/`config` field selects among (the RTL NUM_CONFIGS -- formerly "
         "FSM_MAX_STATES -- and the swb config tables). A per-resource concept, "
         "declared as an arch.json custom_property and emitted verbatim "
         "(uppercase). Distinct from FSM_PER_SLOT (control ports) and NUM_AGUS "
         "(physical AGUs).",
         "4"});
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

  // Override the AGU/port-management space (num_agus) and ensure the agus map
  // holds at least that many entries. The base defaults num_agus to
  // resource_size * PORTS_PER_SLOT (see the ctor); a resource calls this only
  // when it needs a different bound -- e.g. io_mux, whose physical AGU count is
  // data-dependent and can exceed that default.
  void setNumAgus(uint8_t n);

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

  // ---- Register/wire data model (Phase 0 scaffolding) -------------------
  // Drive an output wire. `port` is the data-link index (0..resource_size-1).
  //   registered == false : combinational output, propagates this cycle on a
  //                         value change (e.g. RF read, IO/IOSRAM/IO_MUX mux).
  //   registered == true  : stored as next-state, latched at the clock edge by
  //                         commitOutputRegisters() -> one cycle of latency
  //                         (e.g. DPU out0_reg, NCC/ACC/WIN2 registered out).
  void driveOutput(uint32_t port, PortChannel ch, std::vector<uint8_t> data,
                   size_t bits, bool registered);
  // Drive the wire to the idle (all-zero) value, matching the RTL '0 default.
  void driveOutputIdle(uint32_t port, PortChannel ch, bool registered);
  // Clock edge: latch registered outputs (q <= d) and emit changed values.
  void commitOutputRegisters();
  // Store a value sampled from an incoming DataEvent into an input wire.
  void latchInput(uint32_t port, PortChannel ch, PortValue value);
  // Drain all data links and latch delivered values into the held input wires.
  // Consumers call this early in their clockTick; readInput() then returns the
  // currently-held value (no per-cycle clear -- idle is delivered as 0 by the
  // producer/SWB, matching the RTL).
  void receiveDataInputs();
  // Read the currently-held input wire value.
  const PortValue &readInput(uint32_t port, PortChannel ch);
  // Send a DataEvent carrying the current wire value to the switch box.
  void emitPortChange(uint32_t port, PortChannel ch, const PortValue &value);

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
  // num_fsms: total control/activation ports = resource_size * FSM_PER_SLOT.
  // num_configs: config-table depth (NUM_CONFIGS) -- how many stored configs the
  // instruction's option/config field selects among. These are DISTINCT: a slot
  // exposes num_fsms control ports but may store a different number of configs.
  uint32_t num_fsms;
  uint32_t num_configs;

  // Data buffers
  std::map<uint32_t, std::vector<uint8_t>> data_buffers;

  // Register/wire data model (Phase 0 scaffolding). Output wires modelled as
  // registers (q drives the wire, d is next-state); input wires hold the last
  // sampled value. Keyed by (data-link index, channel).
  std::map<PortKey, PortRegister> out_ports;
  std::map<PortKey, PortValue> in_ports;

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
