#ifndef _SWB_H
#define _SWB_H

#include "conf_manager.h"
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

  // Instruction format
  using DRRAResource::format;
  void handleCONF(const SWB_PKG::CONFInstruction &instr);
  void handleSWB(const SWB_PKG::SWBInstruction &instr);
  void handleROUTE(const SWB_PKG::ROUTEInstruction &instr);

  using DRRAResource::out;

private:
  // Which crossbar and route configuration is live follows the AGU address,
  // so it is read from the AGU array rather than driven by an event.
  void logic(uint32_t subcycle) override;

  // Communication handlers
  void handleSlotEventWithID(Event *event, uint32_t id);
  void handleCellEventWithID(Event *event, uint32_t id);

  // The SWB's own configuration store (see conf_manager.h for why it is not
  // the common one). Written by CONF, read at the option selected below.
  SwbConfManager conf;

  // Slot links
  std::vector<Link *> slot_links;

  // Cell links
  std::vector<Link *> cell_links;

  uint32_t currentFsmOption_swb = 0;
  uint32_t currentFsmOption_route = 0;
};

#endif // _SWB_H
