#include "swb.h"
#include "dataEvent.h"
#include "swb_pkg.h"
#include "timingOperators.h"

using namespace SST;

Swb::Swb(SST::ComponentId_t id, SST::Params &params)
    : DRRAResource(id, params) {
  instructionHandlers = SWB_PKG::createInstructionHandlers(this);
  conf.init(num_fsms, &out);

  // Slot ports
  for (uint32_t i = 0; i < num_slots; i++) {
    slot_links.push_back(nullptr);
  }
  std::string linkName;
  std::vector<uint32_t> connected_links;
  for (uint32_t swb_slot_conn_id = 0; swb_slot_conn_id < num_slots;
       swb_slot_conn_id++) {
    linkName = "slot_port" + std::to_string(swb_slot_conn_id);
    if (isPortConnected(linkName)) {
      if (swb_slot_conn_id == slot_id) {
        out.fatal(CALL_INFO, -1,
                  "SWB cannot be connected to itself (slot %d)\n", slot_id);
      }

      Link *link = configureLink(
          linkName, "0ns",
          new Event::Handler2<Swb, &Swb::handleSlotEventWithID, uint32_t>(
              this, swb_slot_conn_id));
      sst_assert(link, CALL_INFO, -1, "Failed to configure link %s\n",
                 linkName.c_str());

      slot_links[swb_slot_conn_id] = link;
      connected_links.push_back(swb_slot_conn_id);
    }
  }
  out.output("Connected %lu slot links (", connected_links.size());
  for (auto link : connected_links) {
    out.print("%u,", link);
  }
  out.print(")\n");

  // Cell ports
  std::vector<uint32_t> totalConnections;
  for (uint8_t i = 0; i < 9; i++) {
    cell_links.push_back(nullptr);
  }
  for (uint8_t dir = CellDirection::NW; dir <= CellDirection::SE; dir++) {
    linkName = "cell_port" + std::to_string(dir);
    if (isPortConnected(linkName)) {
      Link *link = configureLink(
          linkName, "0ns",
          new Event::Handler2<Swb, &Swb::handleCellEventWithID, uint32_t>(this,
                                                                          dir));

      sst_assert(link, CALL_INFO, -1, "Failed to configure link %s\n",
                 linkName.c_str());

      cell_links[dir] = link;
      totalConnections.push_back(dir);
    }
  }
  out.output("Connected %u cell links (", totalConnections.size());
  for (auto link : totalConnections) {
    out.print("%s,", cell_directions_str[link].c_str());
  }
  out.print(")\n");
}

// The live SWB / ROUTE configuration follows the AGU address: one option per
// address, picked up mid-cycle so that data forwarded later in the cycle uses
// it. Same shape as the RTL, where the config bank index is the AGU output.
void Swb::logic(uint32_t subcycle) {
  DRRAResource::logic(subcycle);

  if (subcycle != 5)
    return;

  if (isPortAddressValid(SWB_PKG::REP_PORT_INTRACELL)) {
    int64_t agu_address = getPortAddress(SWB_PKG::REP_PORT_INTRACELL);
    if (agu_address != currentFsmOption_swb) {
      currentFsmOption_swb = agu_address;
      out.output("SWB switched to SWB configuration #%u\n",
                 currentFsmOption_swb);
    }
  }

  if (isPortAddressValid(SWB_PKG::REP_PORT_INTERCELL)) {
    int64_t agu_address = getPortAddress(SWB_PKG::REP_PORT_INTERCELL);
    if (agu_address != currentFsmOption_route) {
      currentFsmOption_route = agu_address;
      out.output("SWB switched to ROUTE configuration #%u\n",
                 currentFsmOption_route);
    }
  }
}

void Swb::handleCONF(const SWB_PKG::CONFInstruction &instr) {
  // Decode the variant selector and forward to the existing handler, rebuilding
  // the instruction with the variant's own segment layout.
  switch (instr.variant) {
  case SWB_PKG::OPCODE_SWB: {
    auto defs = SWB_PKG::getSwbSegmentDefs();
    Instruction in(instr.raw, instr.format, defs);
    handleSWB(SWB_PKG::SWBInstruction(in));
    break;
  }
  case SWB_PKG::OPCODE_ROUTE: {
    auto defs = SWB_PKG::getRouteSegmentDefs();
    Instruction in(instr.raw, instr.format, defs);
    handleROUTE(SWB_PKG::ROUTEInstruction(in));
    break;
  }
  default:
    out.fatal(CALL_INFO, -1, "Invalid CONF variant: %u\n", instr.variant);
  }
}

void Swb::handleSWB(const SWB_PKG::SWBInstruction &instr) {
  out.output("swb (slot=%d, option=%d, channel=%d, source=%d, target=%d)\n",
             instr.slot, instr.option, instr.channel, instr.source,
             instr.target);

  if (instr.channel != instr.target) {
    out.fatal(CALL_INFO, -1,
              "Invalid channel\nSWB implemented as a "
              "crossbar\n");
  }

  conf.writeCrossbar(instr.option, instr.source, instr.target);
}

void Swb::handleROUTE(const SWB_PKG::ROUTEInstruction &instr) {
  out.output("route (slot=%d, option=%d, sr=%d, source=%d, target=%d)\n",
             instr.slot, instr.option, instr.sr, instr.source, instr.target);

  bool is_receive = instr.sr == SWB_PKG::ROUTE_SR::ROUTE_SR_RECEIVE;
  conf.writeRoute(instr.option, is_receive, instr.source, instr.target);
}

void Swb::handleSlotEventWithID(Event *event, uint32_t id) {
  DataEvent *dataEvent = dynamic_cast<DataEvent *>(event);
  if (dataEvent) {
    // Verify if the slot is mapped to another slot
    if (conf.hasCrossbar(currentFsmOption_swb, id)) {
      uint32_t target = conf.crossbarTarget(currentFsmOption_swb, id);
      out.output("Forwarding data from slot %u to slot %u (size=%dbits, "
                 "data=%s)\n",
                 id, target, dataEvent->size,
                 formatRawDataToWords(dataEvent->payload).c_str());
      slot_links[target]->send(dataEvent);
    } else if (conf.hasSendRoute(currentFsmOption_route, id)) {
      for (auto target : conf.sendTargets(currentFsmOption_route, id)) {
        if (target != CellDirection::C) {
          if (cell_links[target] == nullptr) {
            out.flush();
            out.fatal(CALL_INFO, -1, "Cell link %u is not linked\n", id);
          }
          out.output("Forwarding data to adjacent cell (direction: %s, "
                     "size=%dbits, data=%s)\n",
                     cell_directions_str[target].c_str(), dataEvent->size,
                     formatRawDataToWords(dataEvent->payload).c_str());
          DataEvent *dataEventCopy = dataEvent->clone();
          cell_links[target]->send(dataEventCopy);
        } else {
          handleCellEventWithID(event, CellDirection::C);
        }
      }
    } else {
      if (dataEvent->portType == DataEvent::PortType::WriteWide) {
        // check if Dir::C is in the receive routes
        if (conf.hasRecvRoute(currentFsmOption_route, CellDirection::C)) {
          out.output("Forwarding data to self (direction: C, size=%dbits, "
                     "data=%s)\n",
                     dataEvent->size,
                     formatRawDataToWords(dataEvent->payload).c_str());
          handleCellEventWithID(event, CellDirection::C);
          return;
        }
      }
      out.output("Slot %u is not linked. Ignoring sent data.\n", id);
      out.output("Current SWB FSM option: %u\n", currentFsmOption_swb);
      out.output("Current ROUTE FSM option: %u\n", currentFsmOption_route);
      conf.dumpSendRoutes(currentFsmOption_route);
    }
  }
}

void Swb::handleCellEventWithID(Event *event, uint32_t id) {
  DataEvent *dataEvent = dynamic_cast<DataEvent *>(event);
  // Verify if the slot is mapped to another slot
  if (id != CellDirection::C) {
    out.output("Received data from adjacent cell (direction: %s)\n",
               cell_directions_str[id].c_str());
    if (conf.hasRecvRoute(currentFsmOption_route, id)) {
      if (conf.recvTargets(currentFsmOption_route, id).size() > 1) {
        out.output("Broadcasting data from cell %s to slots ",
                   cell_directions_str[id].c_str());
      } else {
        out.output("Forwarding from cell %s data to slot ",
                   cell_directions_str[id].c_str());
      }
      bool first = true;
      for (auto target : conf.recvTargets(currentFsmOption_route, id)) {
        if (!first)
          out.print(", ");
        out.print("%u", target);
        first = false;
        DataEvent *dataEventCopy = dataEvent->clone();
        slot_links[target]->send(dataEventCopy);
      }
      out.print("\n");
    } else {
      out.fatal(CALL_INFO, -1, "Cell %u is not linked\n", id);
    }
  } else {
    out.output("Received data from self (direction: C)\n");
    const auto &routes = conf.recvTargets(currentFsmOption_route, id);
    if (routes.size() > 1)
      out.output("Broadcasting data to slots ");
    else
      out.output("Forwarding data to slot ");
    bool first = true;
    for (auto target : routes) {
      if (target < num_slots) {
        if (!first)
          out.print(", ");
        out.print("%u", target);
        first = false;
        DataEvent *dataEventCopy = dataEvent->clone();
        slot_links[target]->send(dataEventCopy);
      } else {
        out.fatal(CALL_INFO, -1, "Invalid target slot %u\n", target);
      }
    }
    out.print("\n");
  }
}
