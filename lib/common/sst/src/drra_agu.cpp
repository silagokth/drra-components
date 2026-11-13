#include "drra_agu.h"
#include "vesylaDebug.h"
#include <cassert>
#include <iostream>
#include <memory>
#include <stdexcept>

TimingState *DRRA_AGU::getCurrentLane() {
  return getLaneAtIndex(current_lane_index);
}

TimingState *DRRA_AGU::getLaneAtIndex(size_t index) {
  if (index < lanes.size()) {
    return &lanes[index];
  } else
    throw std::runtime_error("Lane index " + std::to_string(index) +
                             " out of range");
}

// The configuration is only read when the AGU is activated; changing it while
// the built timing state runs is not modelled.
void DRRA_AGU::checkNotBuilt(const std::string &what) const {
  if (table_built)
    throw std::runtime_error("Cannot add " + what + " to a running AGU");
  if (timing_state) {
    if (vesylaDebug())
      std::cout << "Timing state exists, expression: "
                << timing_state->getExpression()->toString() << "\n";
    throw std::runtime_error("Cannot add " + what + " to a running AGU");
  }
}

DRRA_AGU &DRRA_AGU::addEvent(const std::string &name,
                             std::function<void()> handler, uint8_t priority) {
  checkNotBuilt("events");

  // Create new lane if index is equal to size
  if (vesylaDebug())
    std::cout << "Adding event '" << name << "' to lane index "
              << current_lane_index << "\n";
  if (vesylaDebug())
    std::cout << "Current lanes size: " << lanes.size() << "\n";
  if (current_lane_index == lanes.size())
    lanes.emplace_back();

  if (auto lane = getLaneAtIndex(current_lane_index)) {
    lane->addEvent(name, handler, priority);
    lane_initial_addresses.push_back(initial_address);
    group_lo.push_back(current_lane_index);
    last_rep_range = false;
    // An EVT restarts the level count but, as in agu_controller.sv, leaves
    // use_or_reg alone: after a TRANS its REPs are still outer repetitions.
    current_rep_level = 0;
    current_lane_index++;
  } else {
    throw std::runtime_error(
        "Lane index " + std::to_string(current_lane_index) + " out of range");
  }

  printLaneExpressions();
  return *this;
}
std::set<std::shared_ptr<const TimingEvent>>
DRRA_AGU::getEventsForCycle(uint64_t cycle) {
  return timing_state ? timing_state->getEventsForCycle(cycle)
                      : std::set<std::shared_ptr<const TimingEvent>>();
}

DRRA_AGU &DRRA_AGU::addRepetition(uint64_t iterations, uint64_t delay,
                                  uint64_t step) {
  checkNotBuilt("repetitions");
  if (lanes.empty()) {
    throw std::runtime_error("No lanes available to add repetition to");
  }

  // A joined group: a range level over its lanes.
  if (!group_lo.empty() && group_lo.back() + 1 < current_lane_index) {
    RangeRepetition rep;
    rep.iterations = iterations;
    rep.delay = delay;
    rep.step = step;
    rep.lo = group_lo.back();
    rep.hi = current_lane_index - 1;
    range_reps.push_back(rep);
    last_rep_range = true;
    return *this;
  }
  last_rep_range = false;

  if (use_outer) {
    // Outer repetition: recorded at its level and wrapped around the whole
    // lane chain by build()
    if (vesylaDebug())
      std::cout << "Adding outer repetition at level " << current_rep_level
                << "\n";
    if (current_rep_level >= outer_reps.size())
      outer_reps.resize(current_rep_level + 1);
    outer_reps[current_rep_level] = {iterations, delay, step, true};
    outer_configured = true;
  } else {
    if (vesylaDebug())
      std::cout << "Adding repetition to lane index " << current_lane_index - 1
                << " (current_rep_level: " << current_rep_level << ")\n";
    size_t index = current_lane_index - 1;
    if (auto lane = getLaneAtIndex(index)) {
      // Inner repetition: wrap the current lane
      lane->addRepetition(iterations, delay, current_rep_level, step);
    } else
      throw std::runtime_error("Lane index " + std::to_string(index) +
                               " out of range");

    printLaneExpressions();
  }

  // Every base repetition advances the level, outer ones included, as the RTL
  // agu_controller does: a REPX lands on the level before the current one, so
  // an outer repetition that did not advance it leaves the REPX nothing to
  // find.
  current_rep_level++;

  return *this;
}

void DRRA_AGU::printLaneExpressions() const {
  if (vesylaDebug())
    std::cout << "AGU " << this << " lane expressions:\n";
  for (size_t i = 0; i < lanes.size(); ++i) {
    auto expr = lanes[i].getExpression();
    if (vesylaDebug())
      std::cout << " - " << i << "(" << static_cast<const void *>(&lanes[i])
                << "): " << (expr ? expr->toString() : "null") << "\n";
  }
}

RepetitionOperator DRRA_AGU::getLastRepetitionOperator() {
  if (lanes.empty()) {
    throw std::runtime_error(
        "No lanes available to get repetition operator from");
  }

  if (last_rep_range) {
    const RangeRepetition &rep = range_reps.back();
    return RepetitionOperator(rep.iterations, rep.delay, 0, rep.step, nullptr);
  }
  uint64_t level = current_rep_level == 0 ? 0 : current_rep_level - 1;
  if (use_outer) {
    // Outer repetition: the one recorded at the previous level
    if (level >= outer_reps.size() || !outer_reps[level].is_configured)
      throw std::runtime_error("No outer repetition at level " +
                               std::to_string(level));
    const OuterRepetition &rep = outer_reps[level];
    return RepetitionOperator(rep.iterations, rep.delay, level, rep.step,
                              nullptr);
  } else {
    size_t index = current_lane_index - 1;
    if (auto lane = getLaneAtIndex(index)) {
      // Inner repetition: get from the current lane
      return lane->getRepetitionOperatorFromLevel(level);
    } else
      throw std::runtime_error("Lane index " + std::to_string(index) +
                               " out of range");
  }
}

DRRA_AGU &DRRA_AGU::adjustRepetition(uint64_t iterations, uint64_t delay,
                                     uint64_t step) {
  checkNotBuilt("repetitions");
  if (lanes.empty()) {
    throw std::runtime_error("No lanes available to adjust repetition for");
  }
  if (last_rep_range) {
    RangeRepetition &rep = range_reps.back();
    rep.iterations = iterations;
    rep.delay = delay;
    rep.step = step;
    return *this;
  }

  uint64_t level = current_rep_level == 0 ? 0 : current_rep_level - 1;
  if (use_outer) {
    // Outer repetition: adjust the one recorded at the previous level
    if (level >= outer_reps.size())
      outer_reps.resize(level + 1);
    outer_reps[level] = {iterations, delay, step, true};
    outer_configured = true;
  } else {
    size_t index = current_lane_index - 1; // Default to current lane
    if (auto lane = getLaneAtIndex(index)) {
      // Inner repetition: adjust the current lane
      lane->adjustRepetition(iterations, delay, level, step);
    } else
      throw std::runtime_error("Lane index " + std::to_string(index) +
                               " out of range");
  }

  return *this;
}

DRRA_AGU &DRRA_AGU::setLaneStep(size_t lane, bool ext, uint64_t step,
                                uint32_t step_bits) {
  checkNotBuilt("repetitions");
  if (last_rep_range) {
    // At a range level, the lane is counted from its lo.
    RangeRepetition &rep = range_reps.back();
    lane += rep.lo;
    if (!ext) {
      rep.lane_steps[lane] = step;
    } else {
      uint64_t low = rep.lane_steps.count(lane) ? rep.lane_steps[lane] : rep.step;
      rep.lane_steps[lane] = (step << step_bits) | (low & ((1ULL << step_bits) - 1));
    }
    return *this;
  }
  uint64_t level = current_rep_level == 0 ? 0 : current_rep_level - 1;
  if (!use_outer || level >= outer_reps.size() ||
      !outer_reps[level].is_configured)
    throw std::runtime_error("Lane step without an outer repetition");
  auto &steps = outer_reps[level].lane_steps;
  if (!ext) {
    steps[lane] = step;
  } else {
    uint64_t low = steps.count(lane) ? steps[lane] : outer_reps[level].step;
    steps[lane] = (step << step_bits) | (low & ((1ULL << step_bits) - 1));
  }
  return *this;
}

DRRA_AGU &DRRA_AGU::addTransition(uint64_t delay) {
  checkNotBuilt("transitions");

  // agu_controller.sv drops a TRANS once an outer repetition is configured
  if (outer_configured) {
    if (vesylaDebug())
      std::cout << "Ignoring transition after an outer repetition\n";
    return *this;
  }

  // Join the top two groups: the transition into the top group's first lane.
  if (group_lo.size() >= 2) {
    size_t boundary = group_lo.back() - 1;
    group_lo.pop_back();
    if (transition_delays.size() <= boundary)
      transition_delays.resize(boundary + 1, 0);
    transition_delays[boundary] = delay;
    if (vesylaDebug())
      std::cout << "Adding transition " << boundary << " with delay " << delay
                << "\n";
    return *this;
  }

  if (vesylaDebug())
    std::cout << "Adding transition " << transition_delays.size()
              << " with delay " << delay << "\n";
  transition_delays.push_back(delay);
  use_outer = true;
  current_rep_level = 0;

  return *this;
}

DRRA_AGU &DRRA_AGU::build() {
  if (lanes.empty()) {
    throw std::runtime_error("No lanes to build timing state from");
  }

  // Range levels over every lane are the outer repetitions of before.
  bool all_full = true;
  for (const RangeRepetition &rep : range_reps)
    all_full = all_full && rep.lo == 0 && rep.hi + 1 == lanes.size();
  if (!range_reps.empty() && !all_full) {
    buildTable();
    return *this;
  }
  std::vector<OuterRepetition> outer;
  for (const RangeRepetition &rep : range_reps) {
    OuterRepetition o;
    o.iterations = rep.iterations;
    o.delay = rep.delay;
    o.step = rep.step;
    o.is_configured = true;
    o.lane_steps = rep.lane_steps;
    outer.push_back(o);
  }
  outer.insert(outer.end(), outer_reps.begin(), outer_reps.end());
  outer_reps = outer;

  if (vesylaDebug())
    std::cout << "Using lane 0 as initial timing state\n";
  if (vesylaDebug())
    std::cout << "Lane 0 expression: " << lanes[0].getExpression()->toString()
              << "\n";
  timing_state = std::make_unique<TimingState>(lanes[0]);

  // Chain lane k to lane k+1 through mt_configs[k]. Every opened lane runs,
  // as mt_ir.sv runs up to the last configured lane: one with no TRANS follows
  // on with delay 0, the value an unconfigured mt_config holds. A TRANS with
  // no lane after it has nothing to step into and is dropped.
  for (size_t k = 0; k + 1 < lanes.size(); k++) {
    uint64_t delay = k < transition_delays.size() ? transition_delays[k] : 0;

    auto from_expr = timing_state->getExpression();
    if (vesylaDebug())
      std::cout << "from_expr for transition: "
                << (from_expr ? from_expr->toString() : "null") << "\n";

    auto &to_state = lanes[k + 1];
    to_state.build(); // Ensure target lane is built before using its expression
    auto to_expr = to_state.getExpression();
    if (vesylaDebug())
      std::cout << "to_expr for transition: "
                << (to_expr ? to_expr->toString() : "null") << "\n";

    assert(from_expr && "from_expr is null");
    assert(to_expr && "to_expr is null");
    std::shared_ptr<TransitionOperator> transition =
        std::make_shared<TransitionOperator>(
            delay, to_expr->lastEventName(), [] {}, from_expr, to_expr);
    if (vesylaDebug())
      std::cout << "Adding transition with delay " << delay << " from lane "
                << k << " to lane " << k + 1 << ": " << transition->toString()
                << "\n";

    TimingState temp_timing_state(*timing_state, to_state, transition);
    timing_state = std::make_unique<TimingState>(temp_timing_state);
    timing_state->moveTransitionsToEnd();
    timing_state->printOperatorQueue();
  }

  // Outer repetitions wrap the whole lane chain, as or_configs do in
  // or_mt_ir.sv
  for (size_t level = 0; level < outer_reps.size(); level++) {
    const OuterRepetition &rep = outer_reps[level];
    if (!rep.is_configured)
      continue;
    timing_state->addRepetition(rep.iterations, rep.delay, level, rep.step);
  }

  if (vesylaDebug())
    std::cout << "Timing state expression: "
              << timing_state->getExpression()->toString() << "\n";
  timing_state->setEventInitialAddresses(lane_initial_addresses);
  timing_state->build();
  // Per-lane steps of the outer levels, in the order they were added above.
  size_t dim = 0;
  for (const OuterRepetition &rep : outer_reps) {
    if (!rep.is_configured)
      continue;
    if (!rep.lane_steps.empty()) {
      std::vector<uint64_t> steps(lanes.size(), rep.step);
      for (auto [lane, step] : rep.lane_steps)
        if (lane < steps.size())
          steps[lane] = step;
      timing_state->setOuterLaneSteps(dim, steps);
    }
    dim++;
  }
  if (vesylaDebug())
    std::cout << "Timing state built successfully\n";
  return *this;
}

DRRA_AGU &DRRA_AGU::reset() {
  if (vesylaDebug())
    std::cout << "Resetting AGU\n";
  if (vesylaDebug())
    std::cout << "Timing state before reset: "
              << (timing_state ? timing_state->getExpression()->toString()
                               : "none")
              << "\n";
  lanes.clear();
  lane_initial_addresses.clear();
  transition_delays.clear();
  outer_reps.clear();
  range_reps.clear();
  group_lo.clear();
  last_rep_range = false;
  table_built = false;
  addr_table.clear();
  table_last = 0;
  use_outer = false;
  outer_configured = false;
  timing_state = nullptr;
  current_lane_index = 0;
  current_rep_level = 0;
  if (vesylaDebug())
    std::cout << "Timing state after reset: "
              << (timing_state ? timing_state->getExpression()->toString()
                               : "none")
              << "\n";
  if (vesylaDebug())
    std::cout << "AGU reset successfully\n";
  return *this;
}

int64_t DRRA_AGU::getAddressForCycle(uint64_t cycle) {
  if (table_built)
    return cycle < addr_table.size() ? addr_table[cycle] : -1;
  if (isEmpty())
    return -1;

  return timing_state->getAddressForCycle(cycle);
}

uint64_t DRRA_AGU::getLastScheduledCycle() {
  if (table_built)
    return table_last;
  if (!timing_state)
    throw std::runtime_error(
        "No timing state built to get last scheduled cycle");

  return timing_state->getLastScheduledCycle();
}

// The lane sequencer of mt_ir.sv / or_mt_ir.sv, cycle by cycle: each lane runs
// its own (IR) pattern; at the end of lane l the innermost unfinished range
// level with hi == l steps and the chain goes back to its lo after its delay,
// otherwise the next lane follows after its transition delay; outer levels
// repeat the whole chain.
void DRRA_AGU::buildTable() {
  const size_t L = lanes.size();
  std::vector<std::vector<std::pair<uint64_t, int64_t>>> pattern(L);
  std::vector<uint64_t> span(L, 0);
  for (size_t k = 0; k < L; k++) {
    TimingState lane(lanes[k]);
    lane.setEventInitialAddresses({0});
    lane.build();
    span[k] = lane.getLastScheduledCycle();
    for (uint64_t c = 0; c <= span[k]; c++) {
      int64_t a = lane.getAddressForCycle(c);
      if (a >= 0)
        pattern[k].push_back({c, a});
    }
  }
  auto range_step = [&](const RangeRepetition &r, size_t l) -> int64_t {
    auto it = r.lane_steps.find(l);
    if (it != r.lane_steps.end())
      return static_cast<int64_t>(it->second);
    return (l >= r.lo && l <= r.hi) ? static_cast<int64_t>(r.step) : 0;
  };
  auto outer_step = [&](const OuterRepetition &o, size_t l) -> int64_t {
    auto it = o.lane_steps.find(l);
    return static_cast<int64_t>(it != o.lane_steps.end() ? it->second : o.step);
  };

  std::map<uint64_t, int64_t> table;
  std::vector<uint64_t> outer_count(outer_reps.size(), 0);
  uint64_t t = 0;
  while (true) {
    // One run of the chain from lane 0.
    std::vector<uint64_t> count(range_reps.size(), 0);
    size_t l = 0;
    uint64_t end = t;
    while (true) {
      int64_t base = static_cast<int64_t>(lane_initial_addresses[l]);
      for (size_t r = 0; r < range_reps.size(); r++)
        base += static_cast<int64_t>(count[r]) * range_step(range_reps[r], l);
      for (size_t o = 0; o < outer_reps.size(); o++)
        if (outer_reps[o].is_configured)
          base += static_cast<int64_t>(outer_count[o]) * outer_step(outer_reps[o], l);
      for (const auto &[off, a] : pattern[l])
        table[t + off] = base + a;
      end = t + span[l];

      int loop = -1;
      for (size_t r = 0; r < range_reps.size(); r++) {
        if (range_reps[r].hi == l && count[r] + 1 < range_reps[r].iterations) {
          loop = static_cast<int>(r);
          break;
        }
      }
      if (loop >= 0) {
        for (size_t r = 0; r < static_cast<size_t>(loop); r++)
          if (range_reps[r].hi == l)
            count[r] = 0;
        count[loop]++;
        t = end + range_reps[loop].delay + 1;
        l = range_reps[loop].lo;
        continue;
      }
      for (size_t r = 0; r < range_reps.size(); r++)
        if (range_reps[r].hi == l)
          count[r] = 0;
      if (l + 1 >= L)
        break;
      uint64_t d = l < transition_delays.size() ? transition_delays[l] : 0;
      t = end + d + 1;
      l++;
    }
    // Outer levels, innermost first.
    int level = -1;
    for (size_t o = 0; o < outer_reps.size(); o++) {
      if (!outer_reps[o].is_configured)
        continue;
      if (outer_count[o] + 1 < outer_reps[o].iterations) {
        level = static_cast<int>(o);
        break;
      }
    }
    if (level < 0)
      break;
    for (size_t o = 0; o < static_cast<size_t>(level); o++)
      outer_count[o] = 0;
    outer_count[level]++;
    t = end + outer_reps[level].delay + 1;
  }

  table_last = table.empty() ? 0 : table.rbegin()->first;
  addr_table.assign(table_last + 1, -1);
  for (const auto &[c, a] : table)
    addr_table[c] = a;
  table_built = true;
}
