#include "swb.h"
#include "dataEvent.h"
#include "swb_pkg.h"
#include "timingOperators.h"

using namespace SST;

Swb::Swb(SST::ComponentId_t id, SST::Params &params)
    : DRRAResource(id, params) {
  instructionHandlers = SWB_PKG::createInstructionHandlers(this);
  for (uint32_t i = 0; i < num_fsms; i++) {
    connection_maps.push_back(std::map<uint32_t, uint32_t>());
    sending_routes_maps.push_back(std::map<uint32_t, std::vector<uint32_t>>());
    receiving_routes_maps.push_back(
        std::map<uint32_t, std::vector<uint32_t>>());
    next_connection_maps.push_back(std::map<uint32_t, uint32_t>());
    next_sending_routes_maps.push_back(
        std::map<uint32_t, std::vector<uint32_t>>());
    next_receiving_routes_maps.push_back(
        std::map<uint32_t, std::vector<uint32_t>>());
  }

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

bool Swb::clockTick(SST::Cycle_t currentCycle) {
  bool result = DRRAResource::clockTick(currentCycle);

  if (currentCycle % 10 == 5) {
    if (isPortActive(SWB_PKG::REP_PORT_INTRACELL)) {
      int64_t agu_address =
          agus[SWB_PKG::REP_PORT_INTRACELL].getAddressForCycle(
              getPortActiveCycle(SWB_PKG::REP_PORT_INTRACELL));
      if (agu_address >= 0 && agu_address != currentFsmOption_swb) {
        currentFsmOption_swb = agu_address;
        out.output("SWB switched to SWB configuration #%u\n",
                   currentFsmOption_swb);
      }
    }
    if (isPortActive(SWB_PKG::REP_PORT_INTERCELL)) {
      out.output("Checking AGU for intercell port at cycle %lu\n",
                 currentCycle / 10);
      int64_t agu_address =
          agus[SWB_PKG::REP_PORT_INTERCELL].getAddressForCycle(
              getPortActiveCycle(SWB_PKG::REP_PORT_INTERCELL));
      out.output("AGU address for intercell port: %ld\n", agu_address);
      // print agu expression
      out.output("AGU expression: %s\n", agus[SWB_PKG::REP_PORT_INTERCELL]
                                             .getTimingExpressionString()
                                             .c_str());
      if (agu_address >= 0 && agu_address != currentFsmOption_route) {
        currentFsmOption_route = agu_address;
        out.output("SWB switched to ROUTE configuration #%u\n",
                   currentFsmOption_route);
      }
    }
  }

  return result;
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

  // Add the connection to the SWB map
  next_connection_maps[instr.option][instr.source] = instr.target;

  out.output("Adding connection from slot %u to slot %u "
             "in FSM %u\n",
             instr.source, instr.target, instr.option);
}

void Swb::handleROUTE(const SWB_PKG::ROUTEInstruction &instr) {
  out.output("route (slot=%d, option=%d, sr=%d, source=%d, target=%d)\n",
             instr.slot, instr.option, instr.sr, instr.source, instr.target);

  bool is_receive = instr.sr == SWB_PKG::ROUTE_SR::ROUTE_SR_RECEIVE;
  std::vector<uint32_t> targets;
  if (is_receive) {
    // Receive
    // source is cell (NW=0/N/NE/W/C/E/SW/S/SE)
    // target is slot number (1-hot encoded)

    // Convert 1-hot encoded target to slot number
    for (uint32_t i = 0; i < 16; i++) {
      if (instr.target & (1 << i)) {
        targets.push_back(i);
        next_receiving_routes_maps[instr.option][instr.source].push_back(i);
      }
    }
  } else {
    // Send
    // source is slot number
    // target is cell (NW=0/N/NE/W/C/E/SW/S/SE) (1-hot encoded)

    // Convert 1-hot encoded target to cell number
    for (uint32_t i = 0; i < 16; i++) {
      if (instr.target & (1 << i)) {
        targets.push_back(i);
        next_sending_routes_maps[instr.option][instr.source].push_back(i);
      }
    }
  }

  out.output("Adding %s route from %s to [",
             is_receive ? "receiving" : "sending",
             is_receive ? cell_directions_str[instr.source].c_str()
                        : std::to_string(instr.source).c_str());
  for (size_t i = 0; i < targets.size(); ++i) {
    if (is_receive) {
      out.print("%u", targets[i]);
    } else {
      out.print("%s", cell_directions_str[targets[i]].c_str());
    }
    if (i < targets.size() - 1) {
      out.print(", ");
    }
  }
  out.print("] in configuration slot %u\n", instr.option);
}

void Swb::handleEVT(const SWB_PKG::EVTInstruction &instr) {
  out.output("evt (slot=%d, port=%s)\n", instr.slot,
             instr.port == SWB_PKG::REP_PORT_INTRACELL ? "intracell"
                                                       : "intercell");

  // add event to the timing model
  std::string event_name =
      "evt_" + std::to_string(instr.slot) + "_" +
      (instr.port == SWB_PKG::REP_PORT_INTRACELL ? "intracell" : "intercell");
  agus[instr.port].addEvent(
      event_name,
      [this, event_name] {
        out.output("Event %s triggered\n", event_name.c_str());
      },
      1);
}

void Swb::handleREP(const SWB_PKG::REPInstruction &instr) {
  out.output("rep (slot=%d, port=%s, iter=%d, step=%d, delay=%d)\n", instr.slot,
             instr.port == SWB_PKG::REP_PORT_INTRACELL ? "intracell"
                                                       : "intercell",
             instr.iter, instr.step, instr.delay);

  // add repetition to the timing model
  agus[instr.port].addRepetition(instr.iter, instr.delay, instr.step);
}

void Swb::handleREPX(const SWB_PKG::REPXInstruction &instr) {
  out.output(
      "repx (slot=%d, port=%s, iter=%d, step=%d, delay=%d)\n", instr.slot,
      instr.port == SWB_PKG::REP_PORT_INTRACELL ? "intracell" : "intercell",
      instr.iter, instr.step, instr.delay);

  auto repx = instr;
  auto repetition_op = agus[instr.port].getLastRepetitionOperator();
  uint32_t iter = repx.iter << SWB_PKG::SWB_INSTR_REP_ITER_BITWIDTH |
                  repetition_op.getIterations();
  uint32_t step = repx.step << SWB_PKG::SWB_INSTR_REP_STEP_BITWIDTH |
                  repetition_op.getStep();
  uint32_t delay = repx.delay << SWB_PKG::SWB_INSTR_REP_DELAY_BITWIDTH |
                   repetition_op.getDelay();

  agus[instr.port].adjustRepetition(iter, delay, step);
}

void Swb::handleTRANS(const SWB_PKG::TRANSInstruction &instr) {
  out.output("trans (slot=%d, port=%d, delay=%d)\n", instr.slot, instr.port,
             instr.delay);

  agus[instr.port].addTransition(instr.delay);
}

void Swb::switchToNextOption_swb() {
  currentFsmOption_swb++;
  out.output("Switching to FSM port %u\n", currentFsmOption_swb);
}

void Swb::resetOption_swb() {
  currentFsmOption_swb = 0;
  out.output("Reset FSM to 0\n");
}

void Swb::switchToNextOption_route() {
  currentFsmOption_route++;
  out.output("Switching to FSM port %u\n", currentFsmOption_route);
}

void Swb::resetOption_route() {
  currentFsmOption_route = 0;
  out.output("Reset FSM to 0\n");
}

void Swb::handleSlotEventWithID(Event *event, uint32_t id) {
  DataEvent *dataEvent = dynamic_cast<DataEvent *>(event);
  if (dataEvent) {
    // Verify if the slot is mapped to another slot
    if (connection_maps[currentFsmOption_swb].count(id)) {
      uint32_t target = connection_maps[currentFsmOption_swb][id];
      out.output("Forwarding data from slot %u to slot %u\n", id, target);
      slot_links[target]->send(dataEvent);
    } else if (sending_routes_maps[currentFsmOption_route].count(id)) {
      for (auto target : sending_routes_maps[currentFsmOption_route][id]) {
        if (target != CellDirection::C) {
          if (cell_links[target] == nullptr) {
            out.flush();
            out.fatal(CALL_INFO, -1, "Cell link %u is not linked\n", id);
          }
          out.output("Forwarding data to adjacent cell (direction: %s)\n",
                     cell_directions_str[target].c_str());
          DataEvent *dataEventCopy = dataEvent->clone();
          cell_links[target]->send(dataEventCopy);
        } else {
          handleCellEventWithID(event, CellDirection::C);
        }
      }
    } else {
      if (dataEvent->portType == DataEvent::PortType::WriteWide) {
        // check if Dir::C is in receiving_routes_maps
        if (receiving_routes_maps[currentFsmOption_route].count(
                CellDirection::C)) {
          out.output("Forwarding data to self (direction: C)\n");
          handleCellEventWithID(event, CellDirection::C);
          return;
        }
      }
      out.output("Slot %u is not linked. Ignoring sent data.\n", id);
      out.output("Current SWB FSM option: %u\n", currentFsmOption_swb);
      out.output("Current ROUTE FSM option: %u\n", currentFsmOption_route);
      // Print sending routes map
      out.output("Sending routes map (size: %d):\n",
                 next_sending_routes_maps[currentFsmOption_route].size());
      for (int s = 0; s < next_sending_routes_maps.size(); s++) {
        out.output("  FSM option %d:\n", s);
        for (auto const &pair : next_sending_routes_maps[s]) {
          out.output("    Slot %u -> [", pair.first);
          for (size_t i = 0; i < pair.second.size(); ++i) {
            out.print("%s", cell_directions_str[pair.second[i]].c_str());
            if (i < pair.second.size() - 1) {
              out.print(", ");
            }
          }
          out.print("]\n");
        }
      }
    }
  }
}

void Swb::handleCellEventWithID(Event *event, uint32_t id) {
  DataEvent *dataEvent = dynamic_cast<DataEvent *>(event);
  // Verify if the slot is mapped to another slot
  if (id != CellDirection::C) {
    out.output("Received data from adjacent cell (direction: %s)\n",
               cell_directions_str[id].c_str());
    if (receiving_routes_maps[currentFsmOption_route].count(id)) {
      if (receiving_routes_maps[currentFsmOption_route][id].size() > 1) {
        out.output("Broadcasting data from cell %s to slots ",
                   cell_directions_str[id].c_str());
      } else {
        out.output("Forwarding from cell %s data to slot ",
                   cell_directions_str[id].c_str());
      }
      for (int i = 0;
           i < receiving_routes_maps[currentFsmOption_route][id].size(); i++) {
        uint32_t target = receiving_routes_maps[currentFsmOption_route][id][i];
        out.print("%u", target);
        if (i < receiving_routes_maps[currentFsmOption_route][id].size() - 1) {
          out.print(", ");
        }
        DataEvent *dataEventCopy = dataEvent->clone();
        slot_links[target]->send(dataEventCopy);
      }
      out.print("\n");
    } else {
      out.fatal(CALL_INFO, -1, "Cell %u is not linked\n", id);
    }
  } else {
    out.output("Received data from self (direction: C)\n");
    auto routes = receiving_routes_maps[currentFsmOption_route][id];
    if (routes.size() > 1)
      out.output("Broadcasting data to slots ");
    else
      out.output("Forwarding data to slot ");
    for (int i = 0; i < routes.size(); i++) {
      uint32_t target = routes[i];
      if (target < num_slots) {
        if (i < routes.size() - 1) {
          out.print("%u, ", target);
        } else {
          out.print("%u", target);
        }
        DataEvent *dataEventCopy = dataEvent->clone();
        slot_links[target]->send(dataEventCopy);
      } else {
        out.fatal(CALL_INFO, -1, "Invalid target slot %u\n", target);
      }
    }
    out.print("\n");
  }
}

void Swb::handleActivation(uint32_t slot_id, uint32_t ports) {
  activatePortsForSlot(slot_id, ports);
  uint32_t relative_ports = ports << (slot_id * 4);
  if (ports & 0b1) {
    connection_maps = next_connection_maps;
    next_connection_maps.clear();
    for (uint32_t i = 0; i < num_fsms; i++) {
      next_connection_maps.push_back(std::map<uint32_t, uint32_t>());
    }
  }
  if (ports & 0b10) {
    sending_routes_maps = next_sending_routes_maps;
    receiving_routes_maps = next_receiving_routes_maps;

    next_sending_routes_maps.clear();
    next_receiving_routes_maps.clear();
    for (uint32_t i = 0; i < num_fsms; i++) {
      next_sending_routes_maps.push_back(
          std::map<uint32_t, std::vector<uint32_t>>());
      next_receiving_routes_maps.push_back(
          std::map<uint32_t, std::vector<uint32_t>>());
    }
  }
}
