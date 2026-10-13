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

DRRA_AGU &DRRA_AGU::addTransition(uint64_t delay) {
  checkNotBuilt("transitions");

  // agu_controller.sv drops a TRANS once an outer repetition is configured
  if (outer_configured) {
    if (vesylaDebug())
      std::cout << "Ignoring transition after an outer repetition\n";
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
  if (isEmpty())
    return -1;

  return timing_state->getAddressForCycle(cycle);
}

uint64_t DRRA_AGU::getLastScheduledCycle() {
  if (!timing_state)
    throw std::runtime_error(
        "No timing state built to get last scheduled cycle");

  return timing_state->getLastScheduledCycle();
}
