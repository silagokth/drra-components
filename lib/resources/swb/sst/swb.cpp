#include "swb.h"
#include "dataEvent.h"
#include "swb_pkg.h"
#include "timingOperators.h"

using namespace SST;

Swb::Swb(SST::ComponentId_t id, SST::Params &params)
    : DRRAResource(id, params) {
  instructionHandlers = SWB_PKG::createInstructionHandlers(this);
  // Route/SWB config-table depth is the config-table depth NUM_CONFIGS
  // (num_configs), NOT the FSM/activation-port count. Each stored option holds a
  // full crossbar/route snapshot selected by the instruction's `option` field;
  // num_configs sets how many such options exist (mirrors the NUM_CONFIGS-sized
  // config tables in swb_pkg/controller/conf_manager). The 3-bit ISA `option`
  // field is only an upper bound (<=8); the usable count is num_configs.
  for (uint32_t i = 0; i < num_configs; i++) {
    connection_maps.push_back(std::map<uint32_t, uint32_t>());
    sending_routes_maps.push_back(std::map<uint32_t, std::set<uint32_t>>());
    receiving_routes_maps.push_back(std::map<uint32_t, std::set<uint32_t>>());
  }
  swb_opt_touched.assign(num_configs, 0);
  route_opt_touched.assign(num_configs, 0);

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

  // Epoch boundary: DRRAResource::clockTick retires a finished AGU at subcycle 9
  // (checkAGULifetime), so its port drops active->inactive here. That marks the
  // end of the current configuration epoch -- the SST analogue of agu_done in
  // conf_manager.sv.j2. Mark every option stale so the next (re)configuration of
  // an option replaces it instead of merging onto the config left from this
  // epoch. swb options are driven by REP_PORT_INTRACELL, route by INTERCELL.
  bool swb_port_active = isPortActive(SWB_PKG::REP_PORT_INTRACELL);
  if (prev_swb_port_active && !swb_port_active)
    swb_opt_touched.assign(swb_opt_touched.size(), 0);
  prev_swb_port_active = swb_port_active;

  bool route_port_active = isPortActive(SWB_PKG::REP_PORT_INTERCELL);
  if (prev_route_port_active && !route_port_active)
    route_opt_touched.assign(route_opt_touched.size(), 0);
  prev_route_port_active = route_port_active;

  if (currentCycle % 10 == 0) {
    if (!portsToActivate.empty()) {
      for (const auto &[sid, ports] : portsToActivate) {
        activatePortsForSlot(sid, ports);
      }
      portsToActivate.clear();
    }
    // Advance conf_manager's current_option register (1-cycle latch): the option
    // the AGU produces in cycle N selects the config used in cycle N+1. Routing
    // is deferred to the Route phase below.
    latchOption(SWB_PKG::REP_PORT_INTRACELL, currentFsmOption_swb,
                nextFsmOption_swb, "swb_config");
    latchOption(SWB_PKG::REP_PORT_INTERCELL, currentFsmOption_route,
                nextFsmOption_route, "route_config");
  }

  // Route phase (sub-5): evaluate the crossbar as a pure function of
  // (latched config, producer-wire snapshots, neighbor cell inputs). Runs after
  // producers have driven their outputs (Drive, sub-2) and before any consumer
  // samples (Sample, sub-7). One unconditional pass -> order-independent, an
  // unrouted consumer reads 0, and a config switch is handled automatically.
  // This is the SST analogue of the swb.sv.j2 always_comb blocks; it replaces
  // the old change-driven propagate + sub-0 replay + sub-4 zeroing.
  if (currentCycle % 10 == 5) {
    evaluate();
  }

  return result;
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

  // First configuration of this option in a new epoch replaces the whole option
  // (see swb_opt_touched). Clear the stale crossbar before adding this link so a
  // re-used option does not keep connections from a previous epoch.
  if (!swb_opt_touched[instr.option]) {
    connection_maps[instr.option].clear();
    swb_opt_touched[instr.option] = 1;
  }

  // Add the connection to the SWB map
  connection_maps[instr.option][instr.source] = instr.target;

  out.output("Adding connection from slot %u to slot %u "
             "in FSM %u\n",
             instr.source, instr.target, instr.option);
}

void Swb::handleROUTE(const SWB_PKG::ROUTEInstruction &instr) {
  out.output("route (slot=%d, option=%d, sr=%d, source=%d, target=%d)\n",
             instr.slot, instr.option, instr.sr, instr.source, instr.target);

  bool is_receive = instr.sr == SWB_PKG::ROUTE_SR::ROUTE_SR_RECEIVE;

  // First route configuration of this option in a new epoch replaces the whole
  // option (see route_opt_touched). Send and receive share one per-option flag,
  // so the first write of either kind wipes both the receive and send maps for
  // this option before the link below is applied -- a re-used option keeps no
  // routing from a previous epoch.
  if (!route_opt_touched[instr.option]) {
    receiving_routes_maps[instr.option].clear();
    sending_routes_maps[instr.option].clear();
    route_opt_touched[instr.option] = 1;
  }

  std::vector<uint32_t> targets;
  if (is_receive) {
    // Receive
    // source is cell (NW=0/N/NE/W/C/E/SW/S/SE)
    // target is slot number (1-hot encoded)

    // Convert 1-hot encoded target to slot number.
    // REPLACE (not accumulate) the target set for this (option, direction):
    // the RTL conf_manager assigns the whole receive_links[option][dir] bitmask
    // per instruction (conf_manager.sv.j2), so re-configuring an option in a
    // later epoch overwrites its previous routing instead of merging with it.
    auto &receive_targets = receiving_routes_maps[instr.option][instr.source];
    receive_targets.clear();
    for (uint32_t i = 0; i < SWB_PKG::SWB_INSTR_ROUTE_TARGET_BITWIDTH; i++) {
      if (instr.target & (1 << i)) {
        targets.push_back(i);
        receive_targets.insert(i);
      }
    }
  } else {
    // Send
    // source is slot number
    // target is cell (NW=0/N/NE/W/C/E/SW/S/SE) (1-hot encoded)

    // Convert 1-hot encoded target to cell number.
    // REPLACE (not accumulate) the direction set for this (option, source):
    // the RTL conf_manager assigns the whole send_links[option][source] bitmask
    // per instruction (conf_manager.sv.j2).
    // Only the low NUM_DIRS bits reach send_links in the RTL
    // (controller.sv.j2); bits above that are not directions and would index
    // cell_directions_str out of bounds.
    auto &send_targets = sending_routes_maps[instr.option][instr.source];
    send_targets.clear();
    for (uint32_t i = 0; i <= SE; i++) {
      if (instr.target & (1 << i)) {
        targets.push_back(i);
        send_targets.insert(i);
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
  out.output(
      "evt (slot=%d, option=%d, port=%s, init_addr_sd=%d, init_addr=%d)\n",
      instr.slot, instr.option,
      instr.port == SWB_PKG::REP_PORT_INTRACELL ? "intracell" : "intercell",
      instr.init_addr_sd, instr.init_addr);

  // add event to the timing model
  std::string event_name =
      "evt_" + std::to_string(instr.slot) + "_" +
      (instr.port == SWB_PKG::REP_PORT_INTRACELL ? "intracell" : "intercell");
  agus[instr.port].addEvent(
      event_name,
      [this, event_name] {
        out.output("Event %s triggered\n", event_name.c_str());
      },
      1, instr.init_addr);
}

void Swb::handleREP(const SWB_PKG::REPInstruction &instr) {
  out.output("rep (slot=%d, ext=%d, port=%s, iter=%d, step=%d, delay=%d)\n",
             instr.slot, instr.ext,
             instr.port == SWB_PKG::REP_PORT_INTRACELL ? "intracell"
                                                       : "intercell",
             instr.iter, instr.step, instr.delay);

  if (!instr.ext) {
    // base: add a new repetition (low half of iter/step/delay)
    agus[instr.port].addRepetition(instr.iter, instr.delay, instr.step);
  } else {
    // extension: fold the high bits into the last repetition
    auto repetition_op = agus[instr.port].getLastRepetitionOperator();
    uint32_t iter = instr.iter << SWB_PKG::SWB_INSTR_REP_ITER_BITWIDTH |
                    repetition_op.getIterations();
    uint32_t step = instr.step << SWB_PKG::SWB_INSTR_REP_STEP_BITWIDTH |
                    repetition_op.getStep();
    uint32_t delay = instr.delay << SWB_PKG::SWB_INSTR_REP_DELAY_BITWIDTH |
                     repetition_op.getDelay();
    agus[instr.port].adjustRepetition(iter, delay, step);
  }
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

// conf_manager's current_option register (conf_manager.sv.j2), as a
// commit-then-sample flop. `cur` (Q) is the applied option read by evaluate();
// `nxt` (D) is the AGU's next-state. Commit Q<=D applies last cycle's option,
// giving the RTL 1-cycle latch. Then sample D<=agu_address only while the AGU
// drives a valid one (isPortActive && opt>=0 == agu_enable/addr_valid); else D
// holds. After a commit Q==D, so once the AGU stops (agu_done) D stays equal to
// Q and the last option is preserved -- like `else current_option <= current_option`.
void Swb::latchOption(uint32_t port, uint32_t &cur, uint32_t &nxt,
                      const char *tag) {
  if (nxt != cur) {
    cur = nxt;
    out.output("SWB switched to %s #%u\n", tag, cur);
    logTraceEvent(tag, slot_id, true, 'X',
                  {{"option", static_cast<long long>(cur)}});
  }
  if (isPortActive(port)) {
    int64_t opt = agus[port].getAddressForCycle(getPortActiveCycle(port));
    if (opt >= 0)
      nxt = static_cast<uint32_t>(opt);
  }
}

// ---- Register/wire routing engine ---------------------------------------
// Derive the wire channel from the event's port type (robust whether or not a
// sender set the explicit channel tag).
static inline PortChannel channelOf(const DataEvent *e) {
  return (e->portType == DataEvent::PortType::WriteWide ||
          e->portType == DataEvent::PortType::ReadWide)
             ? PortChannel::BULK
             : PortChannel::WORD;
}

void Swb::handleSlotEventWithID(Event *event, uint32_t id) {
  DataEvent *dataEvent = dynamic_cast<DataEvent *>(event);
  if (!dataEvent)
    return;
  PortChannel ch = channelOf(dataEvent);
  // Snapshot only: the Route-phase evaluate() does all routing, so the order in
  // which producers deliver cannot affect the result.
  slot_out_snapshot[{id, ch}] = PortValue(dataEvent->payload, dataEvent->size);
  delete dataEvent;
}

void Swb::handleCellEventWithID(Event *event, uint32_t id) {
  DataEvent *dataEvent = dynamic_cast<DataEvent *>(event);
  if (!dataEvent)
    return;
  // Snapshot only; evaluate() routes cell inputs to receiving slots.
  cell_in_snapshot[id] = PortValue(dataEvent->payload, dataEvent->size);
  delete dataEvent;
}

void Swb::deliverToSlot(uint32_t target_slot, PortChannel ch,
                        const PortValue &value) {
  if (target_slot >= slot_links.size() || slot_links[target_slot] == nullptr) {
    out.output("deliverToSlot: target slot %u not linked; dropping\n",
               target_slot);
    return;
  }
  PortValue &last = slot_in_delivered[{target_slot, ch}];
  if (last == value)
    return; // change-driven: nothing to do if the target's value is unchanged
  last = value;
  DataEvent *ev =
      new DataEvent(ch == PortChannel::WORD ? DataEvent::PortType::WriteNarrow
                                            : DataEvent::PortType::WriteWide);
  ev->channel = ch;
  ev->source_slot = target_slot;
  ev->payload = value.data;
  ev->size = value.bits;
  out.output("Delivering to slot %u (%s, size=%zubits, data=%s)\n", target_slot,
             ch == PortChannel::WORD ? "word" : "bulk", value.bits,
             formatRawDataToWords(value.data).c_str());
  logTraceEvent(
      "swb_route_slot", slot_id, true, 'X',
      {{"target_slot", static_cast<long long>(target_slot)},
       {"channel", std::string(ch == PortChannel::WORD ? "word" : "bulk")},
       {"size", static_cast<long long>(value.bits)},
       {"data", formatRawDataToWords(value.data)}});
  slot_links[target_slot]->send(ev);
}

void Swb::deliverToCell(uint32_t dir, const PortValue &value) {
  if (dir >= cell_links.size() || cell_links[dir] == nullptr) {
    out.flush();
    out.fatal(CALL_INFO, -1, "Cell link %u is not linked\n", dir);
  }
  PortValue &last = cell_out_delivered[dir];
  if (last == value)
    return;
  last = value;
  DataEvent *ev = new DataEvent(DataEvent::PortType::WriteWide);
  ev->channel = PortChannel::BULK;
  ev->payload = value.data;
  ev->size = value.bits;
  out.output("Delivering to cell %s (size=%zubits, data=%s)\n",
             cell_directions_str[dir].c_str(), value.bits,
             formatRawDataToWords(value.data).c_str());
  logTraceEvent("swb_route_cell", slot_id, true, 'X',
                {{"direction", cell_directions_str[dir]},
                 {"size", static_cast<long long>(value.bits)},
                 {"data", formatRawDataToWords(value.data)}});
  cell_links[dir]->send(ev);
}

static PortValue zeroLike(const PortValue &v) {
  PortValue z;
  z.bits = v.bits;
  z.data.assign(v.data.size(), 0);
  return z;
}

void Swb::evaluate() {
  // Route phase: full combinational evaluation of the crossbar from the held
  // producer-wire and cell-input snapshots under the current config -- the SST
  // analogue of the three always_comb blocks in swb.sv.j2. Runs once per cycle,
  // unconditionally, so the result is a pure function of (config, snapshots):
  // order-independent, config switches handled for free, and every wire that
  // has no driver under the current config is driven to 0 (the '0 default).
  // deliverTo* stays change-suppressed purely as a send-side optimisation.

  // ---- BULK send side: cell-output wires + intracell local channel ----------
  std::map<uint32_t, PortValue> cell_out_target; // direction -> value
  bool local_driven = false;
  for (auto &kv : sending_routes_maps[currentFsmOption_route]) {
    auto sit = slot_out_snapshot.find({kv.first, PortChannel::BULK});
    if (sit == slot_out_snapshot.end())
      continue; // source holds no value -> its wires stay 0
    for (uint32_t dir : kv.second) {
      if (dir == CellDirection::C) {
        local_channel_snapshot = sit->second;
        local_driven = true;
      } else {
        cell_out_target[dir] = sit->second;
      }
    }
  }
  if (!local_driven)
    local_channel_snapshot = PortValue();

  // Drive each linked cell output to its value, or 0 if it lost its driver.
  for (uint32_t dir = 0; dir < cell_links.size(); dir++) {
    if (dir == CellDirection::C || cell_links[dir] == nullptr)
      continue;
    auto cit = cell_out_target.find(dir);
    if (cit != cell_out_target.end()) {
      deliverToCell(dir, cit->second);
    } else {
      auto dit = cell_out_delivered.find(dir);
      if (dit != cell_out_delivered.end() && !dit->second.isZero())
        deliverToCell(dir, zeroLike(dit->second));
    }
  }

  // ---- Assemble the intended input value for every consumer slot ------------
  std::map<uint32_t, PortValue> bulk_target; // BULK: from receiving routes
  for (auto &kv : receiving_routes_maps[currentFsmOption_route]) {
    uint32_t dir = kv.first;
    const PortValue *wire = nullptr;
    if (dir == CellDirection::C) {
      if (local_driven)
        wire = &local_channel_snapshot;
    } else {
      auto cin = cell_in_snapshot.find(dir);
      if (cin != cell_in_snapshot.end())
        wire = &cin->second;
    }
    if (!wire)
      continue; // undriven source wire -> targets fall back to 0 below
    for (uint32_t t : kv.second)
      bulk_target[t] = *wire;
  }

  std::map<uint32_t, PortValue> word_target; // WORD: intracell crossbar
  for (auto &kv : connection_maps[currentFsmOption_swb]) {
    auto sit = slot_out_snapshot.find({kv.first, PortChannel::WORD});
    if (sit != slot_out_snapshot.end())
      word_target[kv.second] = sit->second;
  }

  // ---- Deliver computed slot inputs and zero the ones that lost a source ----
  auto flush = [&](const std::map<uint32_t, PortValue> &targets,
                   PortChannel ch) {
    std::vector<uint32_t> to_zero;
    for (auto &kv : slot_in_delivered)
      if (kv.first.second == ch && !targets.count(kv.first.first) &&
          !kv.second.isZero())
        to_zero.push_back(kv.first.first);
    for (auto &kv : targets)
      deliverToSlot(kv.first, ch, kv.second);
    for (uint32_t t : to_zero)
      deliverToSlot(t, ch, zeroLike(slot_in_delivered[{t, ch}]));
  };
  flush(bulk_target, PortChannel::BULK);
  flush(word_target, PortChannel::WORD);
}

void Swb::handleActivation(uint32_t slot_id, uint32_t ports) {
  portsToActivate[slot_id] = ports;
}
