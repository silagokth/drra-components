#include "conf_manager.h"

#include <sst/core/output.h>

const std::string cell_directions_str[9] = {"NW", "N",  "NE", "W", "C",
                                            "E",  "SW", "S",  "SE"};

const std::set<uint32_t> SwbConfManager::no_targets;

void SwbConfManager::init(uint32_t options, DRRAOutput *output) {
  out = output;
  crossbar.assign(options, {});
  send_routes.assign(options, {});
  recv_routes.assign(options, {});
}

void SwbConfManager::checkOption(uint32_t option, const char *what) const {
  if (option >= crossbar.size()) {
    out->fatal(CALL_INFO, -1,
               "SWB configuration option %u out of range (%s, %lu banks)\n",
               option, what, crossbar.size());
  }
}

void SwbConfManager::writeCrossbar(uint32_t option, uint32_t source,
                                   uint32_t target) {
  checkOption(option, "crossbar write");
  crossbar[option][source] = target;

  out->output("Adding connection from slot %u to slot %u "
              "in FSM %u\n",
              source, target, option);
}

void SwbConfManager::writeRoute(uint32_t option, bool receive, uint32_t source,
                                uint32_t target_onehot) {
  checkOption(option, "route write");

  // Receive: source is a cell direction, the one-hot targets are slots.
  // Send:    source is a slot, the one-hot targets are cell directions.
  auto &targets_set =
      receive ? recv_routes[option][source] : send_routes[option][source];

  std::vector<uint32_t> targets;
  for (uint32_t i = 0; i < 16; i++) {
    if (target_onehot & (1 << i)) {
      targets.push_back(i);
      targets_set.insert(i);
    }
  }

  out->output("Adding %s route from %s to [", receive ? "receiving" : "sending",
              receive ? cell_directions_str[source].c_str()
                      : std::to_string(source).c_str());
  for (size_t i = 0; i < targets.size(); ++i) {
    if (receive) {
      out->print("%u", targets[i]);
    } else {
      out->print("%s", cell_directions_str[targets[i]].c_str());
    }
    if (i < targets.size() - 1) {
      out->print(", ");
    }
  }
  out->print("] in configuration slot %u\n", option);
}

bool SwbConfManager::hasCrossbar(uint32_t option, uint32_t source) const {
  checkOption(option, "crossbar read");
  return crossbar[option].count(source) > 0;
}

uint32_t SwbConfManager::crossbarTarget(uint32_t option,
                                        uint32_t source) const {
  checkOption(option, "crossbar read");
  return crossbar[option].at(source);
}

bool SwbConfManager::hasSendRoute(uint32_t option, uint32_t source) const {
  checkOption(option, "send route read");
  return send_routes[option].count(source) > 0;
}

const std::set<uint32_t> &SwbConfManager::sendTargets(uint32_t option,
                                                      uint32_t source) const {
  checkOption(option, "send route read");
  auto it = send_routes[option].find(source);
  return it == send_routes[option].end() ? no_targets : it->second;
}

bool SwbConfManager::hasRecvRoute(uint32_t option, uint32_t source) const {
  checkOption(option, "receive route read");
  return recv_routes[option].count(source) > 0;
}

const std::set<uint32_t> &SwbConfManager::recvTargets(uint32_t option,
                                                      uint32_t source) const {
  checkOption(option, "receive route read");
  auto it = recv_routes[option].find(source);
  return it == recv_routes[option].end() ? no_targets : it->second;
}

void SwbConfManager::dumpSendRoutes(uint32_t option) const {
  checkOption(option, "send route dump");
  out->output("Sending routes map (size: %d):\n", send_routes[option].size());
  for (size_t s = 0; s < send_routes.size(); s++) {
    out->output("  FSM option %d:\n", (int)s);
    for (auto const &pair : send_routes[s]) {
      out->output("    Slot %u -> [", pair.first);
      bool first = true;
      for (auto target : pair.second) {
        if (!first)
          out->print(", ");
        out->print("%s", cell_directions_str[target].c_str());
        first = false;
      }
      out->print("]\n");
    }
  }
}
