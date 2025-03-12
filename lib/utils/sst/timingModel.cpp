#include "timingModel.h"
#include <algorithm>
#include <cstdint>
#include <stdexcept>

// TimingExpression implementations
uint64_t TimingExpression::scheduleEvents(TimingState &state,
                                          uint64_t startCycle) const {
  return 0;
}

uint64_t TimingExpression::lastEventId() const { return 0; }

std::string TimingExpression::lastEventName() const { return ""; }

// TimingState implementations
TimingState::TimingState() : eventCounter(0), lastScheduledCycle(0) {}

TimingState::TimingState(std::shared_ptr<TimingExpression> expression)
    : expression(expression), eventCounter(expression->lastEventId() + 1),
      lastScheduledCycle(0) {
  addEventName(expression->lastEventName());
}

uint64_t TimingState::getLastScheduledCycle() const {
  return lastScheduledCycle;
}

void TimingState::updateLastScheduledCycle(uint64_t cycle) {
  lastScheduledCycle = std::max(lastScheduledCycle, cycle);
}

TimingState TimingState::createFromEvent(const std::string &name) {
  uint64_t eventCounter = 0;
  auto event = std::make_shared<TimingEvent>(name, eventCounter++);
  return TimingState(std::static_pointer_cast<TimingExpression>(event));
}

TimingState &TimingState::addEvent(const std::string &name,
                                   std::function<void()> handler) {
  auto event = std::make_shared<TimingEvent>(name, eventCounter++);
  event->setHandler(handler);
  event->setPriority(5);
  addEventName(name);
  expression = std::static_pointer_cast<TimingExpression>(event);
  return *this;
}

TimingState &TimingState::addEvent(const std::string &name, uint8_t priority,
                                   std::function<void()> handler) {
  auto event = std::make_shared<TimingEvent>(name, eventCounter++);
  event->setHandler(handler);
  event->setPriority(priority);
  addEventName(name);
  expression = std::static_pointer_cast<TimingExpression>(event);
  return *this;
}

TimingState &TimingState::buildTransition(uint64_t delay,
                                          const std::string &nextEventName,
                                          std::function<void()> handler) {
  return buildTransition(delay, nextEventName, 5, handler);
}

TimingState &TimingState::buildTransition(uint64_t delay,
                                          const std::string &nextEventName,
                                          uint8_t priority,
                                          std::function<void()> handler) {
  if (!expression) {
    throw std::runtime_error("Cannot add transition without an event");
  }
  delay++; // delay is always incremented by 1
  auto nextEvent = std::make_shared<TimingEvent>(nextEventName, eventCounter++);
  nextEvent->setHandler(handler);
  nextEvent->setPriority(priority);
  auto transition = std::make_shared<TransitionOperator>(
      delay, nextEventName, handler, expression, nextEvent);
  expression = std::static_pointer_cast<TimingExpression>(transition);
  return *this;
}

TimingState &TimingState::buildRepetition(uint64_t iterations, uint64_t delay) {
  return buildRepetition(iterations, delay, 0, 1);
}

TimingState &TimingState::buildRepetition(uint64_t iterations, uint64_t delay,
                                          uint64_t level, uint64_t step) {
  if (!expression) {
    throw std::runtime_error("Cannot add repetition without an event");
  }
  delay++;      // delay is always incremented by 1
  iterations++; // iterations is always incremented by 1

  levels_current_iteration.push_back(0);
  levels_total_iterations.push_back(iterations);
  levels_step.push_back(step);

  auto repetition = std::make_shared<RepetitionOperator>(
      iterations, delay, level, step, expression);
  expression = std::static_pointer_cast<TimingExpression>(repetition);
  return *this;
}

TimingState &TimingState::addTransition(uint64_t delay,
                                        const std::string &nextEventName,
                                        std::function<void()> handler) {
  return addTransition(delay, nextEventName, 5, handler);
}

TimingState &TimingState::addTransition(uint64_t delay,
                                        const std::string &nextEventName,
                                        uint8_t priority,
                                        std::function<void()> handler) {
  if (!expression) {
    throw std::runtime_error("Cannot add repetition without an event");
  }
  this->operator_queue.push_back(std::make_shared<TransitionOperator>(
      delay, nextEventName, handler, expression, expression));
  return *this;
}

TimingState &TimingState::addRepetition(uint64_t iterations, uint64_t delay) {
  return addRepetition(iterations, delay, 0, 0);
}

TimingState &TimingState::addRepetition(uint64_t iterations, uint64_t delay,
                                        uint64_t level, uint64_t step) {
  if (!expression) {
    throw std::runtime_error("Cannot add repetition without an event");
  }
  this->operator_queue.push_back(std::make_shared<RepetitionOperator>(
      iterations, delay, level, step, expression));
  return *this;
}

uint64_t TimingState::getRepIncrementForCycle(uint64_t cycle) {
  uint64_t increment = 0;
  for (uint64_t i = 0; i < levels_current_iteration.size(); i++) {
    increment += levels_step[i] * levels_current_iteration[i];
  }
  return increment;
}

TimingState &TimingState::build() {
  if (!expression) {
    throw std::runtime_error("Cannot build timing state without events");
  }
  auto expression = this->expression;

  // Order operator_queue by putting all transition in beginning
  uint32_t transition_count = 0;
  for (uint32_t i = 0; i < operator_queue.size(); i++) {
    if (std::dynamic_pointer_cast<TransitionOperator>(operator_queue[i])) {
      if (i != transition_count) {
        std::swap(operator_queue[i], operator_queue[transition_count]);
      }
      transition_count++;
    }
  }

  // Order the left repetition operators by level
  for (uint32_t i = transition_count; i < operator_queue.size(); i++) {
    for (uint32_t j = i + 1; j < operator_queue.size(); j++) {
      if (std::dynamic_pointer_cast<RepetitionOperator>(operator_queue[i]) &&
          std::dynamic_pointer_cast<RepetitionOperator>(operator_queue[j])) {
        if (std::dynamic_pointer_cast<RepetitionOperator>(operator_queue[i])
                ->getLevel() >
            std::dynamic_pointer_cast<RepetitionOperator>(operator_queue[j])
                ->getLevel()) {
          std::swap(operator_queue[i], operator_queue[j]);
        }
      }
    }
  }

  // Add all operators to the expression
  for (auto &op : operator_queue) {
    if (auto transition = std::dynamic_pointer_cast<TransitionOperator>(op)) {
      this->buildTransition(transition->getDelay(),
                            transition->getNextEventName(),
                            transition->getHandler());
    } else if (auto repetition =
                   std::dynamic_pointer_cast<RepetitionOperator>(op)) {
      this->buildRepetition(repetition->getIterations(), repetition->getDelay(),
                            repetition->getLevel(), repetition->getStep());
    } else {
      throw std::runtime_error("Unknown operator type");
    }

    // Update the timing expression
    expression = this->expression;
  }

  // Schedule events
  updateLastScheduledCycle(
      expression->scheduleEvents(*this, lastScheduledCycle));

  return *this;
}

std::shared_ptr<TimingExpression> TimingState::getExpression() const {
  return expression;
}

void TimingState::scheduleEvent(std::shared_ptr<const TimingEvent> event,
                                uint64_t cycle) {
  scheduledEvents[cycle].insert(event);
}

std::set<std::shared_ptr<const TimingEvent>>
TimingState::getEventsForCycle(uint64_t cycle) {
  auto eventsIterator = scheduledEvents.find(cycle);
  if (eventsIterator != scheduledEvents.end()) {
    return eventsIterator->second;
  }
  return std::set<std::shared_ptr<const TimingEvent>>();
}

bool TimingState::findEventByName(const std::string &name) {
  return std::find(eventNames.begin(), eventNames.end(), name) !=
         eventNames.end();
}

void TimingState::incrementLevels() {
  if (levels_current_iteration.size() != 0) {
    uint64_t current_level = 0;
    do {
      levels_current_iteration[current_level]++;
      if (levels_current_iteration[current_level] >=
          levels_total_iterations[current_level]) {
        levels_current_iteration[current_level] = 0;
      } else {
        break;
      }
    } while (++current_level < levels_current_iteration.size());
  }
}

void TimingState::addEventName(const std::string &name) {
  if (findEventByName(name)) {
    throw std::runtime_error("Event with name " + name + " already exists");
  } else {
    eventNames.push_back(name);
  }
}

std::string TimingState::toString() const { return expression->toString(); }

// TimingEvent implementations
TimingEvent::TimingEvent(const std::string &name, uint64_t eventNumber)
    : name(name), eventNumber(eventNumber) {}

uint64_t TimingEvent::scheduleEvents(TimingState &state,
                                     uint64_t startCycle) const {
  state.scheduleEvent(shared_from_this(), startCycle);
  state.updateLastScheduledCycle(startCycle);
  return startCycle;
}

std::string TimingEvent::toString() const {
  return "e" + std::to_string(eventNumber);
}

std::string TimingEvent::getName() const { return name; }

uint64_t TimingEvent::getEventNumber() const { return eventNumber; }

uint64_t TimingEvent::lastEventId() const { return eventNumber; }

std::string TimingEvent::lastEventName() const { return name; }

void TimingEvent::execute() const {
  if (handler) {
    handler();
  }
}

void TimingEvent::setHandler(std::function<void()> handler) {
  this->handler = handler;
}

std::function<void()> TimingEvent::getHandler() const { return handler; }

void TimingEvent::setPriority(uint8_t priority) { this->priority = priority; }

uint8_t TimingEvent::getPriority() const { return priority; }

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
  uint64_t toCycle = to->scheduleEvents(state, fromCycle + delay);
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
      lastCycle += delay;
    }
  }
  return lastCycle;
}

std::string RepetitionOperator::toString() const {
  return "R<" + std::to_string(iterations) + "," + std::to_string(delay) +
         ">(" + expression->toString() + ")";
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
