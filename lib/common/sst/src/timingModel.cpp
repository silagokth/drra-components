#include "timingModel.h"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <deque>
#include <iostream>
#include <memory>
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
  return this->addEvent(name, 5, handler);
}

TimingState &TimingState::addEvent(const std::string &name, uint8_t priority,
                                   std::function<void()> handler) {
  std::shared_ptr<TimingEvent> temp_event = nullptr;

  // Find if an event with the same name already exists in the operator queue
  for (auto &op : operator_queue) {
    if (auto event = std::dynamic_pointer_cast<TimingEvent>(op)) {
      if (event->getName() == name) {
        std::cout << "Merging event handlers for event: " << name << std::endl;
        temp_event = event;
        auto existing_handler = event->getHandler();
        std::function<void()> merged_handler;
        if (existing_handler && handler) {
          merged_handler = [existing_handler, handler]() {
            std::cout << "Executing merged handlers for event." << std::endl;
            existing_handler();
            handler();
          };
        } else if (existing_handler) {
          merged_handler = existing_handler;
        } else if (handler) {
          merged_handler = handler;
        }
        // merged_handler remains empty if both are empty
        temp_event->setHandler(merged_handler);

        temp_event->setPriority(priority);

        // Replace the old event in the operator queue
        op = std::make_shared<TimingEvent>(temp_event);
        break;
      }
    }
  }
  if (!temp_event) {
    temp_event = std::make_shared<TimingEvent>(name, eventCounter++);
    std::function<void()> merged_handler = [handler]() {
      std::cout << "Executing handler for event." << std::endl;
      if (handler) {
        handler();
      }
    };
    temp_event->setHandler(handler);
    temp_event->setPriority(priority);
    addEventName(name);
    this->operator_queue.push_back(std::make_shared<TimingEvent>(temp_event));
  }

  // Update the expression to point to the new/merged event
  expression = std::static_pointer_cast<TimingExpression>(temp_event);
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
  delay++; // delay is always incremented by 1

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
    throw std::runtime_error("Cannot add transition without an event");
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

  // Check that no other repetition operator with the same level exists
  for (auto &op : operator_queue) {
    if (auto repetition = std::dynamic_pointer_cast<RepetitionOperator>(op)) {
      if (repetition->getLevel() == level) {
        throw std::runtime_error("Repetition operator with level " +
                                 std::to_string(level) + " already exists");
      }
    }
  }

  this->operator_queue.push_back(std::make_shared<RepetitionOperator>(
      iterations, delay, level, step, expression));
  return *this;
}

TimingState &TimingState::adjustRepetition(uint64_t iterations, uint64_t delay,
                                           uint64_t level, uint64_t step) {
  // Find repetition operator with the same level
  for (auto &op : operator_queue) {
    if (auto repetition = std::dynamic_pointer_cast<RepetitionOperator>(op)) {
      if (repetition->getLevel() == level) {
        repetition->setIterations(iterations);
        repetition->setDelay(delay);
        repetition->setStep(step);
        return *this;
      }
    }
  }
  throw std::runtime_error("Repetition operator with level " +
                           std::to_string(level) + " not found");
}

uint64_t TimingState::getRepIncrementForCycle(uint64_t cycle) {
  return getAddressForCycle(cycle);
}

TimingState &TimingState::build() {
  if (!expression) {
    this->addEvent("event_0", [] {});
  }

  // Order operator_queue by putting all transition in beginning
  uint32_t transition_count = 0;
  uint32_t event_count = 0;
  for (uint32_t i = 0; i < operator_queue.size(); i++) {
    if (auto trans =
            std::dynamic_pointer_cast<TransitionOperator>(operator_queue[i])) {
      if (i != transition_count) {
        // std::swap(operator_queue[i], operator_queue[transition_count]);
      }
      std::cout << "Found transition operator (delay: " << trans->getDelay()
                << ")" << std::endl;
      transition_count++;
    } else if (auto event =
                   std::dynamic_pointer_cast<TimingEvent>(operator_queue[i])) {
      std::cout << "Found event: " << event->getName() << std::endl;
      std::cout << "id: " << event->toString() << std::endl;
      // std::cout << "Executing event handler for event: " << test->getName()
      //           << std::endl;
      // test->getHandler()(); // this works, it means it preserves the handler
      event_count++;
    } else if (auto rep = std::dynamic_pointer_cast<RepetitionOperator>(
                   operator_queue[i])) {
      std::cout << "Found repetition operator (level: " << rep->getLevel()
                << ", iterations: " << rep->getIterations()
                << ", delay: " << rep->getDelay()
                << ", step: " << rep->getStep() << ")" << std::endl;
    } else {
      throw std::runtime_error("Unknown operator type in operator queue");
    }
  }

  TimingState temp_state;
  uint32_t num_events = 0;
  uint32_t num_transitions = 0;
  if (std::dynamic_pointer_cast<TimingEvent>(operator_queue[0])) {
    std::cout << "First operator is an event" << std::endl;
    temp_state = TimingState(
        std::static_pointer_cast<TimingExpression>(operator_queue[0]));
  }

  // Add all operators to the expression
  std::deque<std::shared_ptr<TimingExpression>> expressions;
  for (auto &op : operator_queue) {
    if (auto event = std::dynamic_pointer_cast<TimingEvent>(op)) {
      expressions.push_back(std::static_pointer_cast<TimingExpression>(event));
    } else if (auto transition =
                   std::dynamic_pointer_cast<TransitionOperator>(op)) {
      std::shared_ptr<TimingExpression> from_expr = expressions.front();
      expressions.pop_front();
      std::shared_ptr<TimingExpression> to_expr = expressions.front();
      expressions.pop_front();
      std::shared_ptr<TransitionOperator> transition_op =
          std::make_shared<TransitionOperator>(
              transition->getDelay(), transition->getNextEventName(),
              transition->getHandler(), from_expr, to_expr);
      // Place the new expression back to the front of the queue
      expressions.push_front(
          std::static_pointer_cast<TimingExpression>(transition_op));
    } else if (auto repetition =
                   std::dynamic_pointer_cast<RepetitionOperator>(op)) {

      levels_current_iteration.push_back(0);
      levels_total_iterations.push_back(repetition->getIterations());
      levels_step.push_back(repetition->getStep());

      std::shared_ptr<TimingExpression> expr = expressions.back();
      expressions.pop_back();
      std::shared_ptr<RepetitionOperator> repetition_op =
          std::make_shared<RepetitionOperator>(
              repetition->getIterations(), repetition->getDelay() + 1,
              repetition->getLevel(), repetition->getStep(), expr);
      expressions.push_back(
          std::static_pointer_cast<TimingExpression>(repetition_op));
    } else {
      throw std::runtime_error("Unknown operator type");
    }

    this->expression = expressions.back();
  }

  // Schedule events
  updateLastScheduledCycle(
      this->expression->scheduleEvents(*this, lastScheduledCycle));

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

RepetitionOperator
TimingState::getRepetitionOperatorFromLevel(uint64_t level) const {
  // Find repetition operator with the same level in the operator queue
  for (auto &op : operator_queue) {
    if (auto repetition = std::dynamic_pointer_cast<RepetitionOperator>(op)) {
      printf("Checking repetition operator with level %lu\n",
             repetition->getLevel());
      if (repetition->getLevel() == level) {
        return *repetition;
      }
    }
  }
  // print the queue
  printf("Operator queue:\n");
  for (auto &op : operator_queue) {
    if (auto repetition = std::dynamic_pointer_cast<RepetitionOperator>(op)) {
      printf(" - Repetition operator with level %lu\n", repetition->getLevel());
    }
  }
  throw std::runtime_error("Repetition operator with level " +
                           std::to_string(level) + " not found");
}

uint64_t TimingState::getAddressForCycle(uint64_t cycle) {
  uint64_t address = 0;
  for (uint64_t i = 0; i < levels_current_iteration.size(); i++) {
    address += levels_step[i] * levels_current_iteration[i];
  }
  return address;
}

void TimingState::copyLevelData(const TimingState &other) {
  this->levels_current_iteration = other.levels_current_iteration;
  this->levels_total_iterations = other.levels_total_iterations;
  this->levels_step = other.levels_step;
}

const std::vector<uint64_t> &TimingState::getLevelsStep() const {
  return levels_step;
}

const std::vector<uint64_t> &TimingState::getLevelsTotalIterations() const {
  return levels_total_iterations;
}

// TimingEvent implementations
TimingEvent::TimingEvent(const std::string &name, uint64_t eventNumber)
    : name(name), eventNumber(eventNumber) {}

TimingEvent::TimingEvent(std::shared_ptr<const TimingEvent> other)
    : name(other->name), eventNumber(other->eventNumber),
      handler(other->handler), priority(other->priority) {}

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
