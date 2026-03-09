#include "timingOperators.h"

// TransitionOperator implementations
TransitionOperator::TransitionOperator(uint64_t delay,
                                       std::string nextEventName,
                                       std::function<void()> handler,
                                       std::shared_ptr<TimingExpression> from,
                                       std::shared_ptr<TimingExpression> to)
    : delay(delay), nextEventName(nextEventName), handler(handler), from(from),
      to(to) {}

uint64_t TransitionOperator::scheduleEvents(TimingState &state,
                                            uint64_t startCycle) const {
  uint64_t fromCycle = from->scheduleEvents(state, startCycle);
  uint64_t toCycle = to->scheduleEvents(state, fromCycle + delay + 1);
  return toCycle;
}

std::string TransitionOperator::toString() const {
  return "T<" + std::to_string(delay) + ">(" + from->toString() + "," +
         to->toString() + ")";
}

uint64_t TransitionOperator::lastEventId() const { return to->lastEventId(); }

std::string TransitionOperator::lastEventName() const {
  return to->lastEventName();
}

uint64_t TransitionOperator::getDelay() const { return delay; }

std::string TransitionOperator::getNextEventName() const {
  return nextEventName;
}

std::function<void()> TransitionOperator::getHandler() const { return handler; }

std::shared_ptr<TimingExpression> TransitionOperator::getFrom() const {
  return from;
}

std::shared_ptr<TimingExpression> TransitionOperator::getTo() const {
  return to;
}

// RepetitionOperator implementations
RepetitionOperator::RepetitionOperator(
    uint64_t iterations, uint64_t delay, uint64_t level, uint64_t step,
    std::shared_ptr<TimingExpression> expression)
    : iterations(iterations), delay(delay), level(level), step(step),
      expression(expression) {}

uint64_t RepetitionOperator::scheduleEvents(TimingState &state,
                                            uint64_t startCycle) const {
  uint64_t lastCycle = startCycle;
  for (uint64_t i = 0; i < iterations; i++) {
    lastCycle = expression->scheduleEvents(state, lastCycle);
    if (i < iterations - 1) {
      lastCycle += delay + 1;
    }
  }
  return lastCycle;
}

std::string RepetitionOperator::toString() const {
  return "R<" + std::to_string(iterations) + "," + std::to_string(step) + "," +
         std::to_string(delay) + ">(" + expression->toString() + ")";
}

uint64_t RepetitionOperator::lastEventId() const {
  return expression->lastEventId();
}

std::string RepetitionOperator::lastEventName() const {
  return expression->lastEventName();
}

uint64_t RepetitionOperator::getIterations() const { return iterations; }

uint64_t RepetitionOperator::getDelay() const { return delay; }

uint64_t RepetitionOperator::getLevel() const { return level; }

uint64_t RepetitionOperator::getStep() const { return step; }

std::shared_ptr<TimingExpression> RepetitionOperator::getExpression() const {
  return expression;
}
