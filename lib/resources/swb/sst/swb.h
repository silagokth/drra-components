#ifndef _SWB_H
#define _SWB_H

#include "drra_resource.h"
#include "swb_pkg.h"
#include <cstdint>
#include <map>
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

  // Communication handlers
  void handleSlotEventWithID(Event *event, uint32_t id);
  void handleCellEventWithID(Event *event, uint32_t id);

  // Cell directions
  enum CellDirection { NW, N, NE, W, C, E, SW, S, SE };
  std::string cell_directions_str[9] = {"NW", "N",  "NE", "W", "C",
                                        "E",  "SW", "S",  "SE"};

  // Map input ports to output ports ([source] = target)
  std::vector<std::map<uint32_t, uint32_t>> connection_maps;
  std::vector<std::map<uint32_t, uint32_t>> next_connection_maps;
  std::vector<std::map<uint32_t, std::vector<uint32_t>>> sending_routes_maps;
  std::vector<std::map<uint32_t, std::vector<uint32_t>>>
      next_sending_routes_maps;
  std::vector<std::map<uint32_t, std::vector<uint32_t>>> receiving_routes_maps;
  std::vector<std::map<uint32_t, std::vector<uint32_t>>>
      next_receiving_routes_maps;

  // Slot links
  std::vector<Link *> slot_links;

  // Cell links
  std::vector<Link *> cell_links;

  std::vector<uint32_t> current_config_option = {0, 0};
  std::vector<uint32_t> current_rep_level = {0, 0};
  std::vector<uint32_t> last_config_trans = {0, 0};

  uint32_t currentFsmOption_swb = 0;
  uint32_t currentFsmOption_route = 0;
  uint32_t currentEventNumber = 0;
};

#endif // _SWB_H
