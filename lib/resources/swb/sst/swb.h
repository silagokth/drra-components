#ifndef _SWB_H
#define _SWB_H

#include "drra_resource.h"
#include "swb_pkg.h"
#include <cstdint>
#include <map>
#include <unordered_map>
#include <vector>

class Swb : public DRRAResource {
public:
  /* Element Library Info */
  SST_ELI_REGISTER_COMPONENT(Swb, "drra", "swb",
                             SST_ELI_ELEMENT_VERSION(1, 0, 0), "Swb component",
                             COMPONENT_CATEGORY_UNCATEGORIZED)

  /* Element Library Params */
  static std::vector<SST::ElementInfoParam> getComponentParams() {
    auto params = DRRAResource::getBaseParams();
    return params;
  }
  SST_ELI_DOCUMENT_PARAMS(getComponentParams())

  /* Element Library Ports */
  static std::vector<SST::ElementInfoPort> getComponentPorts() {
    auto ports = DRRAResource::getBasePorts();
    ports.push_back(
        {"slot_port%(portnum)d",
         "Link(s) to resources slots (slot_port0, slot_port1, etc.)"});
    ports.push_back({"cell_port%(portnum)d",
                     "Link(s) to cells (cell_port0, cell_port1, etc.)"});
    return ports;
  }
  SST_ELI_DOCUMENT_PORTS(getComponentPorts())

  /* Element Library Statistics */
  static std::vector<SST::ElementInfoStatistic> getComponentStatistics() {
    auto stats = DRRAResource::getBaseStatistics();
    return stats;
  }
  SST_ELI_DOCUMENT_STATISTICS(getComponentStatistics())

  /* Constructor */
  Swb(SST::ComponentId_t id, SST::Params &params);

  /* Destructor */
  ~Swb() {};

  bool clockTick(SST::Cycle_t currentCycle) override;

  // Instruction format
  using DRRAResource::format;
  void handleEVT(const SWB_PKG::EVTInstruction &instr);
  void handleREP(const SWB_PKG::REPInstruction &instr);
  void handleREPX(const SWB_PKG::REPXInstruction &instr);
  void handleTRANS(const SWB_PKG::TRANSInstruction &instr);
  void handleSWB(const SWB_PKG::SWBInstruction &instr);
  void handleROUTE(const SWB_PKG::ROUTEInstruction &instr);

  using DRRAResource::out;

private:
  // Activation handler override from DRRAResource
  void handleActivation(uint32_t slot_id, uint32_t ports) override;

  void switchToNextOption_swb();
  void resetOption_swb();
  void switchToNextOption_route();
  void resetOption_route();

  // Advance one conf_manager option register by a clock: commit the applied
  // option `cur` (Q) from the AGU next-state `nxt` (D), then re-sample D from
  // the AGU on `port`. See latchOption() in swb.cpp for the register semantics.
  void latchOption(uint32_t port, uint32_t &cur, uint32_t &nxt,
                   const char *tag);

  // Communication handlers
  void handleSlotEventWithID(Event *event, uint32_t id);
  void handleCellEventWithID(Event *event, uint32_t id);

  // ---- Register/wire routing engine ------------------------------------
  // The link handlers snapshot every producer's output wire; evaluate() routes
  // them in one pure-function pass per cycle at the Route phase. Mirrors the
  // swb.sv.j2 always_comb crossbar and adds no intracell latency.
  void deliverToSlot(uint32_t target_slot, PortChannel ch,
                     const PortValue &value);
  void deliverToCell(uint32_t dir, const PortValue &value);
  // Route phase: recompute every consumer input wire, cell output wire and the
  // intracell local channel as a pure function of (latched config, producer
  // snapshots, neighbor cell inputs). Unrouted wires are driven to 0.
  void evaluate();

  // Cell directions
  enum CellDirection { NW, N, NE, W, C, E, SW, S, SE };
  std::string cell_directions_str[9] = {"NW", "N",  "NE", "W", "C",
                                        "E",  "SW", "S",  "SE"};

  // Map input ports to output ports ([source] = target)
  std::vector<std::map<uint32_t, uint32_t>> connection_maps;
  std::vector<std::map<uint32_t, std::set<uint32_t>>> sending_routes_maps;
  std::vector<std::map<uint32_t, std::set<uint32_t>>> receiving_routes_maps;

  // Slot links
  std::vector<Link *> slot_links;

  // Cell links
  std::vector<Link *> cell_links;

  // Register-model routing state.
  //   slot_out_snapshot : (source slot, channel) -> last value the source drove
  //   cell_in_snapshot  : direction -> last bulk value from that neighbor
  //   local_channel_snapshot : intracell bulk local channel (DIR_LOCAL)
  //   *_delivered : last value pushed to each target, to suppress redundant
  //                 forwards (change-driven propagation).
  std::map<PortKey, PortValue> slot_out_snapshot;
  std::map<uint32_t, PortValue> cell_in_snapshot;
  PortValue local_channel_snapshot;
  std::map<PortKey, PortValue> slot_in_delivered;
  std::map<uint32_t, PortValue> cell_out_delivered;

  std::vector<uint32_t> current_config_option = {0, 0};
  std::vector<uint32_t> current_rep_level = {0, 0};
  std::vector<uint32_t> last_config_trans = {0, 0};

  // Per-option "touched-this-epoch" flags (sized num_configs). An option holds a
  // complete crossbar/route snapshot, but each swb/route instruction writes only
  // one link. Reusing an option in a later epoch must REPLACE it, not merge new
  // links onto the stale ones, so the first (re)configuration of an option after
  // its selecting AGU has finished clears the whole option before applying the
  // link. The epoch boundary is the port going active->inactive (checkAGULifetime
  // retires the AGU), tracked by prev_*_port_active below. Mirrors swb_opt_touched
  // / route_opt_touched in conf_manager.sv.j2.
  std::vector<uint8_t> swb_opt_touched;
  std::vector<uint8_t> route_opt_touched;
  bool prev_swb_port_active = false;
  bool prev_route_port_active = false;

  // conf_manager's current_option register (see latchOption()): current* is the
  // applied option Q read by evaluate(); next* is the AGU next-state D.
  uint32_t currentFsmOption_swb = 0;   // Q: SWB (intracell) option
  uint32_t currentFsmOption_route = 0; // Q: ROUTE (intercell) option
  uint32_t nextFsmOption_swb = 0;      // D: SWB agu_address
  uint32_t nextFsmOption_route = 0;    // D: ROUTE agu_address
  uint32_t currentEventNumber = 0;


  std::unordered_map<uint32_t, uint32_t> portsToActivate;
};

#endif // _SWB_H
