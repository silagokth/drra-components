#pragma once

#include "drra_agu.h"
#include "drra_output.h"

#include <cstdint>
#include <string>
#include <vector>

// AGUArray -- the SST twin of lib/common/rtl/agu_array/rtl/src/agu_array.sv.
//
// Owns every AGU of a resource and everything between the resource's
// instruction port and its datapath: the configuration EVT / REP / TRANS
// build up, the activation state, and the generated address. Like the RTL
// module it takes plain instruction fields rather than the ISA-generated
// evt_t / rep_t / trans_t structs, so it stays ISA-agnostic and is compiled
// once for every resource.
//
// Clocked on the resource subcycle contract (see DRRAResource::clockTick):
//
//   sub 0   activate()             ACT strobes latched by the resource
//   sub 1   evt() rep() trans()    whatever arrived at the instruction port
//   sub 2   update()               the single clocked point: every running AGU
//                                  advances one cycle and drives addr,
//                                  addr_valid and done
//   sub 3+  addr() / addrValid()   stable for the rest of the cycle
//
// One EVT opens one *lane* -- an event plus the REP nest wrapped around it,
// the same word agu_cfg_if.sv uses for the first dimension of ir_configs. A
// TRANS concatenates lane k to lane k+1 (mt_configs[k]) and switches every
// later REP to an outer repetition (or_configs), exactly as agu_controller.sv
// does with use_or_reg.
class AGUArray {
public:
  AGUArray() = default;
  AGUArray(const AGUArray &) = delete;
  AGUArray &operator=(const AGUArray &) = delete;

  void init(uint32_t num_agus, DRRAOutput *output);

  // --- configuration, subcycle 1 --------------------------------------
  // `agu` is the relative port index, i.e. slot_pos * PORTS_PER_SLOT + port.

  // Opens a lane. The lane's event carries no behaviour -- what a resource
  // does with an address lives in its port-action table and is driven by
  // addr_valid -- but it is still what the following REP and TRANS attach to.
  void evt(uint32_t agu, uint64_t init_addr);

  // A base REP (ext = 0) carries the low half of iter / step / delay and a
  // REPX (ext = 1) folds the high half into the repetition it follows, so the
  // shift widths are the resource's own ISA segment widths.
  void rep(uint32_t agu, bool ext, uint32_t iter, uint32_t step, uint32_t delay,
           uint32_t iter_bits, uint32_t step_bits, uint32_t delay_bits);

  void trans(uint32_t agu, uint32_t delay);

  // --- activation, subcycle 0 -----------------------------------------
  void activate(uint32_t agu);

  // --- the clocked point, subcycle 2 ----------------------------------
  void update();

  // --- output ports ---------------------------------------------------
  uint32_t size() const { return static_cast<uint32_t>(ports.size()); }
  bool active(uint32_t agu) const { return ports.at(agu).active; }
  bool addrValid(uint32_t agu) const { return ports.at(agu).addr_valid; }
  int64_t addr(uint32_t agu) const { return ports.at(agu).addr; }
  bool done(uint32_t agu) const { return ports.at(agu).done; }
  int64_t activeCycle(uint32_t agu) const { return ports.at(agu).cycle; }
  bool anyActive() const;

  // Debug only.
  std::string expressionString(uint32_t agu);

private:
  struct Port {
    DRRA_AGU agu;
    bool active = false;
    bool addr_valid = false;
    bool done = false;
    int64_t addr = -1;
    // Cycles since activation. -1 until this port's first update(), so that
    // the first update of a freshly activated AGU lands on cycle 0.
    int64_t cycle = -1;
    // Lanes opened since the last reset. Only used to keep event names unique
    // within one AGU, which TimingState requires.
    uint32_t lanes = 0;
  };

  std::vector<Port> ports;
  DRRAOutput *out = nullptr;
};
