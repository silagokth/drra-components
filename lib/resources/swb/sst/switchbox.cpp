#include "switchbox.h"

#include <bitset>

#include "dataEvent.h"

Switchbox::Switchbox(ComponentId_t id, Params &params)
    : DRRAResource(id, params) {
  for (uint32_t i = 0; i < num_fsms; i++) {
    connection_maps.push_back(map<uint32_t, uint32_t>());
    sending_routes_maps.push_back(map<uint32_t, vector<uint32_t>>());
    receiving_routes_maps.push_back(map<uint32_t, vector<uint32_t>>());
    next_connection_maps.push_back(map<uint32_t, uint32_t>());
    next_sending_routes_maps.push_back(map<uint32_t, vector<uint32_t>>());
    next_receiving_routes_maps.push_back(map<uint32_t, vector<uint32_t>>());
  }

  // Set FSM 0 as the default FSM
  currentFsmOption_swb = 0;
  currentFsmOption_route = 0;

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
          new Event::Handler2<Switchbox, &Switchbox::handleSlotEventWithID,
                              uint32_t>(this, swb_slot_conn_id));
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
          new Event::Handler2<Switchbox, &Switchbox::handleCellEventWithID,
                              uint32_t>(this, dir));

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

void Switchbox::handleSlotEventWithID(Event *event, uint32_t id) {
  DataEvent *dataEvent = dynamic_cast<DataEvent *>(event);
  if (dataEvent) {
    // if (!active_ports[dataEvent->portType]) {
    //   out.fatal(CALL_INFO, -1, "Received data while inactive\n", slot_id);
    // }

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
    }
  }
}

void Switchbox::handleCellEventWithID(Event *event, uint32_t id) {
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

void Switchbox::decodeInstr(uint32_t instr) {
  // Decode instruction
  uint32_t opcode = getInstrOpcode(instr);
  switch (opcode) {
  case REP: // repetition instruction
    handleRep(instr);
    break;
  case REPX: // repetition instruction
    handleRepx(instr);
    break;
  case FSM: // transition instruction
    // pendingFSMInstr = instr; // save FSM instruction
    handleFsm(instr);
    break;
  case SWB: // event instruction
    // Switch FSM port if FSM instruction is pending
    // if (pendingFSMInstr) {
    //   handleFsm(pendingFSMInstr);
    // }
    // Add handler to the current FSM port
    // fsmHandlers["fsm_" + std::to_string(currentFsmOption)].push_back(
    //     [this, instr] { handleSwb(instr); });
    handleSwb(instr);
    break;
  case ROUTE: // event instruction
    // Add handler to the current FSM port
    // fsmHandlers["fsm_" + std::to_string(currentFsmOption)].push_back(
    //     [this, instr] { handleRoute(instr); });
    handleRoute(instr);
    break;
  default:
    out.fatal(CALL_INFO, -1, "Invalid opcode\n");
    break;
  }
}

void Switchbox::handleActivation(uint32_t slot_id, uint32_t ports) {
  uint32_t relative_ports = ports << (slot_id * 4);
  if (ports & 0b1) {
    connection_maps = next_connection_maps;
    next_connection_maps.clear();
    for (uint32_t i = 0; i < num_fsms; i++) {
      next_connection_maps.push_back(map<uint32_t, uint32_t>());
    }
  }
  if (ports & 0b100) {
    sending_routes_maps = next_sending_routes_maps;
    receiving_routes_maps = next_receiving_routes_maps;

    next_sending_routes_maps.clear();
    next_receiving_routes_maps.clear();
    for (uint32_t i = 0; i < num_fsms; i++) {
      next_sending_routes_maps.push_back(map<uint32_t, vector<uint32_t>>());
      next_receiving_routes_maps.push_back(map<uint32_t, vector<uint32_t>>());
    }
  }
}

void Switchbox::handleRep(uint32_t instr) {
  // Instruction fields
  uint32_t port = getInstrField(instr, 2, 22);
  uint32_t level = getInstrField(instr, 4, 18);
  uint32_t iter = getInstrField(instr, 6, 12);
  uint32_t step = getInstrField(instr, 6, 6);
  uint32_t delay = getInstrField(instr, 6, 0);

  // For now, we only support increasing repetition levels (and no skipping)
  if (level != port_last_rep_level[port] + 1) {
    out.fatal(CALL_INFO, -1, "Invalid repetition level (last=%u, curr=%u)\n",
              port_last_rep_level[port], level);
  } else {
    port_last_rep_level[port] = level;
  }

  // add repetition to the timing model
  try {
    next_timing_states[port].addRepetition(iter, delay, level, step);
  } catch (const std::exception &e) {
    out.fatal(CALL_INFO, -1, "Failed to add repetition: %s\n", e.what());
  }
}

void Switchbox::handleRepx(uint32_t instr) {
  // Instruction fields
  uint32_t slot = getInstrSlot(instr);
  uint32_t port = getInstrField(instr, 2, 22);
  uint32_t level = getInstrField(instr, 4, 18);
  uint32_t iter_msb = getInstrField(instr, 6, 12);
  uint32_t step_msb = getInstrField(instr, 6, 6);
  uint32_t delay_msb = getInstrField(instr, 6, 0);

  out.output("repx (slot=%d, port=%d, level=%d, iter_msb=%d, step_msb=%d, "
             "delay_msb=%d)\n",
             slot, port, level, iter_msb, step_msb, delay_msb);

  uint32_t port_num = 0;
  auto it = std::find(slot_ids.begin(), slot_ids.end(), slot);
  if (it != slot_ids.end()) {
    port_num = std::distance(slot_ids.begin(), it);
  } else {
    out.fatal(CALL_INFO, -1, "Slot ID not found\n");
  }
  port_num = port_num * 4 + port;

  auto repetition_op =
      next_timing_states[port_num].getRepetitionOperatorFromLevel(level);
  uint32_t iter = iter_msb << 6 | repetition_op.getIterations();
  uint32_t step = step_msb << 6 | repetition_op.getStep();
  uint32_t delay = delay_msb << 6 | repetition_op.getDelay();
  try {
    next_timing_states[port_num].adjustRepetition(iter, delay, level, step);
  } catch (const std::exception &e) {
    out.fatal(CALL_INFO, -1, "REPX failed: %s\n", e.what());
  }
}

void Switchbox::handleFsm(uint32_t instr) {
  // Instruction fields
  uint32_t slot = getInstrSlot(instr);
  uint32_t port = getInstrField(instr, 2, 22);
  uint32_t delay_0 = getInstrField(instr, 7, 15);
  uint32_t delay_1 = getInstrField(instr, 7, 8);
  uint32_t delay_2 = getInstrField(instr, 7, 1);

  out.output("fsm (slot %u, port %u, delay_0 %u, delay_1 %u, delay_2 %u)\n",
             slot, port, delay_0, delay_1, delay_2);

  vector<uint32_t> delays = {delay_0, delay_1, delay_2};
  if (port == 0) { // inner cell communication
    if (next_connection_maps[0].size() == 0) {
      out.fatal(CALL_INFO, -1, "No inner cell connections in FSM 0\n");
    }
    next_timing_states[port].addEvent("fsm_option_slot_reset_" +
                                          std::to_string(currentEventNumber),
                                      5, [this] { resetOption_swb(); });
    currentEventNumber++;
    out.output("Adding FSM reset event\n");

    for (uint32_t i = 1; i < num_fsms; i++) {
      out.output("Size of connection_maps[%u]: %lu\n", i,
                 next_connection_maps[i].size());
      if (next_connection_maps[i].size() == 0) {
        break;
      }
      next_timing_states[port].addTransition(
          delays[i - 1],
          "fsm_option_slot_switch_" + std::to_string(currentEventNumber), 5,
          [this] { switchToNextOption_swb(); });
      currentEventNumber++;
      out.output("Adding FSM transition from FSM %u to FSM %u\n", i - 1, i);
    }
  } else if (port == 2) // outer cell communication
  {
    if (next_receiving_routes_maps[0].size() == 0 &&
        (next_sending_routes_maps[0].size() == 0)) {
      out.fatal(CALL_INFO, -1, "No outer cell connections in FSM 0\n");
    }
    next_timing_states[port].addEvent("fsm_option_slot_reset_" +
                                          std::to_string(currentEventNumber),
                                      5, [this] { resetOption_route(); });
    currentEventNumber++;
    out.output("Adding FSM reset event\n");

    for (uint32_t i = 1; i < num_fsms; i++) {
      if ((next_receiving_routes_maps[i].size() == 0) &&
          (next_sending_routes_maps[i].size() == 0)) {
        break;
      }
      // TODO: verify the timing of this
      next_timing_states[port].addTransition(
          delays[i - 1],
          "fsm_option_route_switch_" + std::to_string(currentEventNumber), 5,
          [this] { switchToNextOption_route(); });
      currentEventNumber++;
      out.output("Adding FSM transition from FSM %u to FSM %u\n", i - 1, i);
    }
  } else
    out.fatal(CALL_INFO, -1, "Invalid SWB port (can only be 0 or 2)\n");
}

void Switchbox::handleSwb(uint32_t instr) {
  // Instruction fields
  uint32_t option = getInstrField(instr, 2, 22);
  uint32_t channel = getInstrField(instr, 4, 18);
  uint32_t source = getInstrField(instr, 4, 14);
  uint32_t target = getInstrField(instr, 4, 10);

  if (channel != target) {
    out.fatal(CALL_INFO, -1,
              "Invalid channel\nSWB implemented as a "
              "crossbar\n");
  }

  // Add the connection to the SWB map
  next_connection_maps[option][source] = target;

  out.output("Adding connection from slot %u to slot %u "
             "in FSM %u\n",
             source, target, option);
}

void Switchbox::handleRoute(uint32_t instr) {
  // Instruction fields
  uint32_t option = getInstrField(instr, 2, 22);
  bool is_receive = getInstrField(instr, 1, 21) == 1;
  uint32_t source = getInstrField(instr, 4, 17);
  uint32_t target = getInstrField(instr, 16, 1);
  vector<uint32_t> targets;

  out.output("route (slot=%u, option=%u, sr=%u, source=%u, target=%s)\n",
             getInstrSlot(instr), option, is_receive, source,
             bitset<16>(target).to_string().c_str());

  if (is_receive) {
    // Receive
    // source is cell (NW=0/N/NE/W/C/E/SW/S/SE)
    // target is slot number (1-hot encoded)

    // Convert 1-hot encoded target to slot number
    for (uint32_t i = 0; i < 16; i++) {
      if (target & (1 << i)) {
        targets.push_back(i);
        next_receiving_routes_maps[option][source].push_back(i);
      }
    }
  } else {
    // Send
    // source is slot number
    // target is cell (NW=0/N/NE/W/C/E/SW/S/SE) (1-hot encoded)

    // Convert 1-hot encoded target to cell number
    for (uint32_t i = 0; i < 16; i++) {
      if (target & (1 << i)) {
        targets.push_back(i);
        next_sending_routes_maps[option][source].push_back(i);
      }
    }
  }

  out.output("Adding %s route from %s to [",
             is_receive ? "receiving" : "sending",
             is_receive ? cell_directions_str[source].c_str()
                        : to_string(source).c_str());
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
  out.print("] in FSM %u\n", option);
}
