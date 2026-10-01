#include "agu_array.h"

#include <string>

void AGUArray::init(uint32_t num_agus, DRRAOutput *output) {
  out = output;
  ports.clear();
  ports.resize(num_agus);
}

void AGUArray::evt(uint32_t agu, uint64_t init_addr) {
  Port &p = ports.at(agu);

  // setInitialAddress must precede addEvent: DRRA_AGU captures the initial
  // address per lane at the moment the lane is opened. This is the single
  // init-address path -- resources no longer keep one of their own.
  p.agu.setInitialAddress(init_addr);
  p.agu.addEvent("lane_" + std::to_string(agu) + "_" + std::to_string(p.lanes),
                 [] {}, 1);
  p.lanes++;
}

void AGUArray::rep(uint32_t agu, bool ext, uint32_t iter, uint32_t step,
                   uint32_t delay, uint32_t iter_bits, uint32_t step_bits,
                   uint32_t delay_bits) {
  Port &p = ports.at(agu);
  if (!ext) {
    // Base REP: a new repetition carrying the low half of each field.
    p.agu.addRepetition(iter, delay, step);
  } else {
    // REPX: fold the high bits into the repetition this one follows.
    RepetitionOperator repetition_op = p.agu.getLastRepetitionOperator();
    p.agu.adjustRepetition((iter << iter_bits) | repetition_op.getIterations(),
                           (delay << delay_bits) | repetition_op.getDelay(),
                           (step << step_bits) | repetition_op.getStep());
  }
}

void AGUArray::trans(uint32_t agu, uint32_t delay) {
  ports.at(agu).agu.addTransition(delay);
}

void AGUArray::activate(uint32_t agu) {
  Port &p = ports.at(agu);

  if (p.agu.isEmpty()) {
    // An RTL AGU with no configuration still produces one address cycle.
    // Model it with a single lane so update() can retire the port instead of
    // leaving it active across epochs.
    p.agu.addEvent("default_" + std::to_string(agu), [] {}, 1);
    p.lanes++;
  }
  p.agu.build();

  p.active = true;
  p.done = false;
  p.addr_valid = false;
  p.addr = -1;
  p.cycle = -1;

  if (out)
    out->output("AGU %u activated\n", agu);
}

void AGUArray::update() {
  for (uint32_t i = 0; i < ports.size(); i++) {
    Port &p = ports[i];
    p.done = false;

    if (!p.active) {
      p.addr_valid = false;
      p.addr = -1;
      continue;
    }

    p.cycle++;
    const uint64_t last_cycle = p.agu.getLastScheduledCycle();

    if (static_cast<uint64_t>(p.cycle) > last_cycle) {
      // Retired one cycle after the last scheduled address, so a resource
      // acting late in the cycle still sees the port active on that last
      // cycle.
      p.active = false;
      p.done = true;
      p.addr_valid = false;
      p.addr = -1;
      p.cycle = -1;
      p.lanes = 0;
      p.agu.reset();
      if (out)
        out->output("AGU %u done (ran for %lu cycles)\n", i, last_cycle + 1);
      continue;
    }

    p.addr = p.agu.getAddressForCycle(static_cast<uint64_t>(p.cycle));
    p.addr_valid = p.addr >= 0;

    if (out)
      out->output("AGU %u cycle %ld/%lu: addr %ld (valid %d)\n", i,
                  static_cast<long>(p.cycle), last_cycle,
                  static_cast<long>(p.addr), static_cast<int>(p.addr_valid));
  }
}

bool AGUArray::anyActive() const {
  for (const Port &p : ports) {
    if (p.active)
      return true;
  }
  return false;
}

std::string AGUArray::expressionString(uint32_t agu) {
  return ports.at(agu).agu.getTimingExpressionString();
}
