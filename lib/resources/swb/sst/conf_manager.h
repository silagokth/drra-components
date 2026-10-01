#ifndef _SWB_CONF_MANAGER_H
#define _SWB_CONF_MANAGER_H

#include "drra_output.h"

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

// Cell directions. Shared vocabulary between the configuration store and the
// forwarding path, so it lives here rather than inside either one.
enum CellDirection { NW, N, NE, W, C, E, SW, S, SE };
extern const std::string cell_directions_str[9];

// SwbConfManager -- the SWB's configuration store, the SST counterpart of
// swb/rtl/conf_manager.sv.j2.
//
// Deliberately NOT the common configuration register file: one SWB CONF writes
// one *link*, and the tables are two-dimensional (option x link). A register
// file whose write granularity is a whole register cannot express that without
// a masked write, a read-modify-write, or redefining a register as a single
// link -- all three were considered and rejected on the RTL side, which is why
// the RTL conversion was reverted and swb keeps its own module. The SST model
// follows the same split.
//
// Holds `options` banks of three tables:
//   * crossbar    source slot           -> one target slot
//   * send route  source slot           -> set of cell directions
//   * recv route  source cell direction -> set of target slots
//
// Which bank is live is not decided here: the resource reads that off its AGU
// and passes it in.
class SwbConfManager {
public:
  void init(uint32_t options, DRRAOutput *output);

  // --- write path: one CONF writes one link ---------------------------
  void writeCrossbar(uint32_t option, uint32_t source, uint32_t target);

  // `target_onehot` is one-hot: cell directions when sending, target slots
  // when receiving.
  void writeRoute(uint32_t option, bool receive, uint32_t source,
                  uint32_t target_onehot);

  // --- read path, at the option the resource selected -----------------
  bool hasCrossbar(uint32_t option, uint32_t source) const;
  uint32_t crossbarTarget(uint32_t option, uint32_t source) const;

  bool hasSendRoute(uint32_t option, uint32_t source) const;
  const std::set<uint32_t> &sendTargets(uint32_t option, uint32_t source) const;

  bool hasRecvRoute(uint32_t option, uint32_t source) const;
  const std::set<uint32_t> &recvTargets(uint32_t option, uint32_t source) const;

  uint32_t options() const { return static_cast<uint32_t>(crossbar.size()); }

  // Debug only: every bank's send routes, plus the size of the live one.
  void dumpSendRoutes(uint32_t option) const;

private:
  void checkOption(uint32_t option, const char *what) const;

  std::vector<std::map<uint32_t, uint32_t>> crossbar;
  std::vector<std::map<uint32_t, std::set<uint32_t>>> send_routes;
  std::vector<std::map<uint32_t, std::set<uint32_t>>> recv_routes;

  static const std::set<uint32_t> no_targets;

  DRRAOutput *out = nullptr;
};

#endif // _SWB_CONF_MANAGER_H
