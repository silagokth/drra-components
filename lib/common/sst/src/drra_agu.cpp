#include "drra_agu.h"
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

DRRA_AGU &DRRA_AGU::addEvent(const std::string &name,
                             std::function<void()> handler, uint8_t priority) {
  if (timing_state) {
    if (std::getenv("VESYLA_DEBUG"))
      std::cout << "Timing state exists, expression: "
                << timing_state->getExpression()->toString() << "\n";
    throw std::runtime_error("Cannot add events after transitions");
  }

  // Create new lane if index is equal to size
  if (std::getenv("VESYLA_DEBUG"))
    std::cout << "Adding event '" << name << "' to lane index "
              << current_lane_index << "\n";
  if (std::getenv("VESYLA_DEBUG"))
    std::cout << "Current lanes size: " << lanes.size() << "\n";
  if (current_lane_index == lanes.size())
    lanes.emplace_back();

  if (auto lane = getLaneAtIndex(current_lane_index)) {
    auto expr = lane->getExpression();
    lane->addEvent(name, handler, priority);
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
  if (lanes.empty() && !timing_state) {
    throw std::runtime_error(
        "No lanes or timing state available to add repetition to");
  }

  if (timing_state) {
    // Outer repetition: wrap the entire timing state
    timing_state->addRepetition(iterations, delay, current_rep_level, step);
  } else {
    if (std::getenv("VESYLA_DEBUG"))
      std::cout << "Adding repetition to lane index " << current_lane_index - 1
                << " (current_rep_level: " << current_rep_level << ")\n";
    size_t index = current_lane_index - 1;
    if (auto lane = getLaneAtIndex(index)) {
      // Inner repetition: wrap the current lane
      lane->addRepetition(iterations, delay, current_rep_level, step);
    } else
      throw std::runtime_error("Lane index " + std::to_string(index) +
                               " out of range");

    current_rep_level++;
    printLaneExpressions();
  }

  return *this;
}

void DRRA_AGU::printLaneExpressions() const {
  if (std::getenv("VESYLA_DEBUG"))
    std::cout << "AGU " << this << " lane expressions:\n";
  for (size_t i = 0; i < lanes.size(); ++i) {
    auto expr = lanes[i].getExpression();
    if (std::getenv("VESYLA_DEBUG"))
      std::cout << " - " << i << "(" << static_cast<const void *>(&lanes[i])
                << "): " << (expr ? expr->toString() : "null") << "\n";
  }
}

RepetitionOperator DRRA_AGU::getLastRepetitionOperator() {
  if (lanes.empty() && !timing_state) {
    throw std::runtime_error(
        "No lanes or timing state available to get repetition operator from");
  }

  if (timing_state) {
    // Outer repetition: get from the entire timing state
    return timing_state->getRepetitionOperatorFromLevel(current_rep_level - 1);
  } else {
    size_t index = lanes.size() - 1; // Default to last lane
    if (auto lane = getLaneAtIndex(index)) {
      // Inner repetition: get from the current lane
      return lane->getRepetitionOperatorFromLevel(current_rep_level - 1);
    } else
      throw std::runtime_error("Lane index " + std::to_string(index) +
                               " out of range");
  }
}

DRRA_AGU &DRRA_AGU::adjustRepetition(uint64_t iterations, uint64_t delay,
                                     uint64_t step) {
  if (lanes.empty() && !timing_state) {
    throw std::runtime_error(
        "No lanes or timing state available to adjust repetition for");
  }

  if (timing_state) {
    // Outer repetition: adjust the entire timing state
    timing_state->adjustRepetition(iterations, delay, current_rep_level - 1,
                                   step);
  } else {
    size_t index = current_lane_index - 1; // Default to current lane
    if (auto lane = getLaneAtIndex(index)) {
      // Inner repetition: adjust the current lane
      lane->adjustRepetition(iterations, delay, current_rep_level - 1, step);
    } else
      throw std::runtime_error("Lane index " + std::to_string(index) +
                               " out of range");
  }

  return *this;
}

DRRA_AGU &DRRA_AGU::addTransition(uint64_t delay) {
  if (current_trans_index + 1 >= lanes.size())
    throw std::runtime_error("Transition target lane out of range");

  auto &from_state = (current_trans_index == 0 || !timing_state)
                         ? lanes[current_trans_index]
                         : *timing_state;
  auto from_expr = (current_trans_index == 0 || !timing_state)
                       ? lanes[current_trans_index].getExpression()
                       : timing_state->getExpression();
  if (std::getenv("VESYLA_DEBUG"))
    std::cout << "from_expr for transition: "
              << (from_expr ? from_expr->toString() : "null") << "\n";

  auto &to_state = lanes[current_trans_index + 1];
  to_state.build(); // Ensure target lane is built before using its expression
  auto to_expr = to_state.getExpression();
  if (std::getenv("VESYLA_DEBUG"))
    std::cout << "to_expr for transition: "
              << (to_expr ? to_expr->toString() : "null") << "\n";

  std::shared_ptr<TransitionOperator> transition =
      std::make_shared<TransitionOperator>(
          delay, to_expr->lastEventName(), [] {}, from_expr, to_expr);
  if (std::getenv("VESYLA_DEBUG"))
    std::cout << "Adding transition with delay " << delay << " from lane "
              << current_trans_index << " to lane " << current_trans_index + 1
              << "\n";

  assert(from_expr && "from_expr is null");
  assert(to_expr && "to_expr is null");
  if (std::getenv("VESYLA_DEBUG"))
    std::cout << "transition: " << transition->toString() << "\n";
  TimingState temp_timing_state(from_state, to_state, transition);
  if (std::getenv("VESYLA_DEBUG"))
    std::cout << "temp_timing_state expression: "
              << temp_timing_state.getExpression()->toString() << "\n";
  timing_state = std::make_unique<TimingState>(temp_timing_state);
  timing_state->moveTransitionsToEnd();

  if (std::getenv("VESYLA_DEBUG"))
    std::cout << "Timing state expression after adding transition: "
              << timing_state->getExpression()->toString() << "\n";
  current_trans_index++;
  current_rep_level = 0;

  timing_state->printOperatorQueue();

  return *this;
}

DRRA_AGU &DRRA_AGU::build() {
  if (!timing_state) {
    if (std::getenv("VESYLA_DEBUG"))
      std::cout << "No transitions added, building timing state from lanes\n";
    if (lanes.empty()) {
      throw std::runtime_error("No lanes to build timing state from");
    }
    // Use the first lane as the initial timing state if no transitions were
    // added
    if (std::getenv("VESYLA_DEBUG"))
      std::cout << "Using lane 0 as initial timing state\n";
    if (std::getenv("VESYLA_DEBUG"))
      std::cout << "Lane 0 expression: " << lanes[0].getExpression()->toString()
                << "\n";
    timing_state = std::make_unique<TimingState>(lanes[0]);
  }

  if (std::getenv("VESYLA_DEBUG"))
    std::cout << "Timing state expression: "
              << timing_state->getExpression()->toString() << "\n";
  timing_state->build();
  if (std::getenv("VESYLA_DEBUG"))
    std::cout << "Timing state built successfully\n";
  return *this;
}

DRRA_AGU &DRRA_AGU::reset() {
  if (std::getenv("VESYLA_DEBUG"))
    std::cout << "Resetting AGU\n";
  if (std::getenv("VESYLA_DEBUG"))
    std::cout << "Timing state before reset: "
              << (timing_state ? timing_state->getExpression()->toString()
                               : "none")
              << "\n";
  lanes.clear();
  timing_state = nullptr;
  current_lane_index = 0;
  current_trans_index = 0;
  current_rep_level = 0;
  if (std::getenv("VESYLA_DEBUG"))
    std::cout << "Timing state after reset: "
              << (timing_state ? timing_state->getExpression()->toString()
                               : "none")
              << "\n";
  if (std::getenv("VESYLA_DEBUG"))
    std::cout << "AGU reset successfully\n";
  return *this;
}

int64_t DRRA_AGU::getAddressForCycle(uint64_t cycle) {
  if (isEmpty())
    return -1;

  int64_t address = timing_state->getAddressForCycle(cycle);
  if (address != -1)
    address += initial_address;

  return address;
}

uint64_t DRRA_AGU::getLastScheduledCycle() {
  if (!timing_state)
    throw std::runtime_error(
        "No timing state built to get last scheduled cycle");

  return timing_state->getLastScheduledCycle();
}
