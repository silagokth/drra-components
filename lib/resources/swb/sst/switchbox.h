#ifndef _SWITCHBOX_H
#define _SWITCHBOX_H

#include "drra.h"

using namespace std;
using namespace SST;

class Switchbox : public DRRAResource {
public:
  /* Element Library Info */
  SST_ELI_REGISTER_COMPONENT(Switchbox, // Class name
                             "drra",    // Name of library
                             "swb",     // Lookup name for component
                             SST_ELI_ELEMENT_VERSION(1, 0,
                                                     0),  // Component version
                             "Switchbox component",       // Description
                             COMPONENT_CATEGORY_PROCESSOR // Category
  )

  /* Element Library Params */
  static vector<ElementInfoParam> getComponentParams() {
    auto params = DRRAResource::getBaseParams();
    return params;
  }
  SST_ELI_DOCUMENT_PARAMS(getComponentParams())

  /* Element Library Ports */
  static vector<ElementInfoPort> getComponentPorts() {
    auto ports = DRRAResource::getBasePorts();
    ports.push_back(
        {"slot_port%(portnum)d",
         "Link(s) to resources slots (slot_port0, slot_port1, etc.)"});
    ports.push_back({"cell_port%(portnum)d",
                     "Link(s) to cells (cell_port0, cell_port1, etc.)"});
    return ports;
  }
  SST_ELI_DOCUMENT_PORTS(getComponentPorts())

  SST_ELI_DOCUMENT_STATISTICS()

  /* Constructor */
  Switchbox(ComponentId_t id, Params &params);

  /* Destructor */
  ~Switchbox() {};

private:
  // Decode instruction
  void decodeInstr(uint32_t instr) override;

  void handleActivation(uint32_t slot_id, uint32_t ports) override;

  void switchToNextOption_swb() {
    currentFsmOption_swb++;
    out.output("Switching to FSM port %u\n", currentFsmOption_swb);
  }

  void resetOption_swb() {
    currentFsmOption_swb = 0;
    out.output("Reset FSM to 0\n");
  }

  void switchToNextOption_route() {
    currentFsmOption_route++;
    out.output("Switching to FSM port %u\n", currentFsmOption_route);
  }

  void resetOption_route() {
    currentFsmOption_route = 0;
    out.output("Reset FSM to 0\n");
  }

  void printRouteMapsForFSM(uint32_t fsmNumber, bool current = true) {
    auto _sendingRoutes = current ? sending_routes_maps[fsmNumber]
                                  : next_sending_routes_maps[fsmNumber];
    auto _receivingRoutes = current ? receiving_routes_maps[fsmNumber]
                                    : next_receiving_routes_maps[fsmNumber];
    if (current)
      out.output("Routes for FSM %u:\n", fsmNumber);
    else
      out.output("Next routes for FSM %u:\n", fsmNumber);

    out.output("  Sending routes (%u):\n", _sendingRoutes.size());
    for (const auto &route : _sendingRoutes) {
      out.output("    from slot %u to cells ", route.first);
      for (const auto &cell : route.second) {
        out.print("%s,", cell_directions_str[cell].c_str());
      }
      out.print("\n");
    }
    out.output("  Receiving routes (%u):\n", _receivingRoutes.size());
    for (const auto &route : _receivingRoutes) {
      out.output("    from cell %s to slots ",
                 cell_directions_str[route.first].c_str());
      for (const auto &slot : route.second) {
        out.print("%u,", slot);
      }
      out.print("\n");
    }
  }

  // Different supported opcodes
  enum Opcode {
    REP,
    REPX,
    FSM,
    SWB = 4,
    ROUTE,
  };

  void handleRep(uint32_t instr);
  void handleRepx(uint32_t instr);
  void handleFsm(uint32_t instr);
  void handleSwb(uint32_t instr);
  void handleRoute(uint32_t instr);

  // Map input ports to output ports ([source] = target)
  vector<map<uint32_t, uint32_t>> connection_maps;
  vector<map<uint32_t, uint32_t>> next_connection_maps;
  vector<map<uint32_t, vector<uint32_t>>> sending_routes_maps;
  vector<map<uint32_t, vector<uint32_t>>> next_sending_routes_maps;
  vector<map<uint32_t, vector<uint32_t>>> receiving_routes_maps;
  vector<map<uint32_t, vector<uint32_t>>> next_receiving_routes_maps;

  // Slot links
  vector<Link *> slot_links;

  // Cell links
  vector<Link *> cell_links;

  // Cell directions
  enum CellDirection { NW, N, NE, W, C, E, SW, S, SE };
  string cell_directions_str[9] = {"NW", "N",  "NE", "W", "C",
                                   "E",  "SW", "S",  "SE"};

  // Handlers
  void handleSlotEventWithID(Event *event, uint32_t id);
  void handleCellEventWithID(Event *event, uint32_t id);

  // Events handlers list
  vector<function<void()>> eventsHandlers;

  uint32_t currentFsmOption_swb = 0;
  uint32_t currentFsmOption_route = 0;
  uint32_t currentEventNumber = 0;
  uint32_t pendingFSMInstr = 0;
  uint64_t activeCycle = 0;
};

#endif // _SWITCHBOX_H
