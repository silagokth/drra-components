#include "timingExpression.h"
#include "timingModel.h"
#include "timingOperators.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <deque>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <sys/types.h>

// TimingExpression implementations
uint64_t TimingExpression::scheduleEvents(TimingState &state,
                                          uint64_t startCycle) const {
  return 0;
}

uint64_t TimingExpression::lastEventId() const { return 0; }

std::string TimingExpression::lastEventName() const { return ""; }

// TimingState implementations
TimingState::TimingState() : eventCounter(0), lastScheduledCycle(0) {}

TimingState::TimingState(const TimingState &other)
    : expression(other.expression), eventCounter(other.eventCounter),
      lastScheduledCycle(other.lastScheduledCycle),
      eventNames(other.eventNames), operator_queue(other.operator_queue),
      eventInitialAddresses(other.eventInitialAddresses),
      scheduledEvents(other.scheduledEvents),
      generatedAddresses(other.generatedAddresses),
      levels_current_iteration(other.levels_current_iteration),
      levels_total_iterations(other.levels_total_iterations),
      levels_step(other.levels_step) {}

TimingState::TimingState(std::shared_ptr<TimingExpression> expression)
    : expression(expression), eventCounter(expression->lastEventId() + 1),
      lastScheduledCycle(0) {
  addEventName(expression->lastEventName());
}

TimingState::TimingState(const TimingState &fromState,
                         const TimingState &toState,
                         std::shared_ptr<TransitionOperator> transition) {
  // Concatenate operator queues
  operator_queue = fromState.operator_queue;
  operator_queue.insert(operator_queue.end(), toState.operator_queue.begin(),
                        toState.operator_queue.end());
  // Add the transition
  this->addTransition(transition);

  // Merge event names (avoid duplicates)
  eventNames = fromState.eventNames;
  for (const auto &name : toState.eventNames) {
    if (std::find(eventNames.begin(), eventNames.end(), name) ==
        eventNames.end()) {
      eventNames.push_back(name);
    }
  }

  // Set eventCounter to max of both + 1
  eventCounter = std::max(fromState.eventCounter, toState.eventCounter) + 1;

  // Set expression to the transition
  expression = std::static_pointer_cast<TimingExpression>(transition);
}

TimingState::TimingState(std::shared_ptr<TransitionOperator> transition)
    : eventCounter(0), lastScheduledCycle(0) {
  // Extract "from" event
  auto fromEvent = transition->getFrom();
  if (std::getenv("VESYLA_DEBUG"))
    std::cout << "Extracting 'from' event from transition operator..."
              << std::endl;
  if (std::getenv("VESYLA_DEBUG"))
    std::cout << "fromEvent: " << transition->getFrom()->toString()
              << std::endl;
  if (!fromEvent) {
    throw std::runtime_error(
        "TransitionOperator 'from' is not a simple TimingEvent");
  }

  // Extract "to" event
  if (std::getenv("VESYLA_DEBUG"))
    std::cout << "Extracting 'to' event from transition operator..."
              << std::endl;
  auto toEvent = transition->getTo();
  if (!toEvent) {
    throw std::runtime_error(
        "TransitionOperator 'to' is not a simple TimingEvent");
  }

  // Populate operator_queue with events then transition
  // eventCounter = fromEvent->lastEventId() + 1;
  eventNames.push_back(fromEvent->lastEventName());
  operator_queue.push_back(
      std::static_pointer_cast<TimingExpression>(transition->getFrom()));

  // eventCounter = std::max(eventCounter, toEvent->getEventNumber()) + 1;
  eventNames.push_back(toEvent->lastEventName());
  operator_queue.push_back(
      std::static_pointer_cast<TimingExpression>(transition->getTo()));

  operator_queue.push_back(
      std::static_pointer_cast<TimingExpression>(transition));

  expression = std::static_pointer_cast<TimingExpression>(fromEvent);
}

TimingState &TimingState::operator=(const TimingState &other) {
  if (this != &other) {
    expression = other.expression;
    eventCounter = other.eventCounter;
    lastScheduledCycle = other.lastScheduledCycle;
    eventNames = other.eventNames;
    operator_queue = other.operator_queue;
    eventInitialAddresses = other.eventInitialAddresses;
    scheduledEvents = other.scheduledEvents;
    generatedAddresses = other.generatedAddresses;
    levels_current_iteration = other.levels_current_iteration;
    levels_total_iterations = other.levels_total_iterations;
    levels_step = other.levels_step;
  }
  return *this;
}

void TimingState::moveTransitionsToEnd() {
  std::vector<std::shared_ptr<TimingExpression>> nonTransitions;
  std::vector<std::shared_ptr<TimingExpression>> transitions;

  for (const auto &op : operator_queue) {
    if (std::dynamic_pointer_cast<TransitionOperator>(op)) {
      transitions.push_back(op);
    } else {
      nonTransitions.push_back(op);
    }
  }
  operator_queue.clear();
  operator_queue.insert(operator_queue.end(), nonTransitions.begin(),
                        nonTransitions.end());
  operator_queue.insert(operator_queue.end(), transitions.begin(),
                        transitions.end());
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
                                   std::function<void()> handler,
                                   uint8_t priority) {
  std::shared_ptr<TimingEvent> temp_event = nullptr;

  // Find if an event with the same name already exists in the operator queue
  for (auto &op : operator_queue) {
    if (auto event = std::dynamic_pointer_cast<TimingEvent>(op)) {
      if (event->getName() == name) {
        if (std::getenv("VESYLA_DEBUG"))
          std::cout << "Merging event handlers for event: " << name
                    << std::endl;
        temp_event = event;
        auto existing_handler = event->getHandler();
        std::function<void()> merged_handler;
        if (existing_handler && handler) {
          merged_handler = [existing_handler, handler]() {
            if (std::getenv("VESYLA_DEBUG"))
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
      if (std::getenv("VESYLA_DEBUG"))
        std::cout << "Executing handler for event." << std::endl;
      if (handler) {
        handler();
      }
    };
    temp_event->setHandler(handler);
    temp_event->setPriority(priority);
    eventNames.push_back(name);
    this->operator_queue.push_back(std::make_shared<TimingEvent>(temp_event));
  }

  // Print the current operator queue for debugging
  if (std::getenv("VESYLA_DEBUG"))
    std::cout << "Current operator queue after adding event '" << name
              << "':" << std::endl;
  printOperatorQueue();

  // Update the expression to point to the new/merged event
  expression = std::static_pointer_cast<TimingExpression>(temp_event);
  return *this;
}

void TimingState::printOperatorQueue() const {
  if (std::getenv("VESYLA_DEBUG"))
    std::cout << "Operator queue (" << operator_queue.size()
              << " operators):" << std::endl;
  for (const auto &op : operator_queue) {
    if (auto event = std::dynamic_pointer_cast<TimingEvent>(op)) {
      if (std::getenv("VESYLA_DEBUG"))
        std::cout << "  - Event: " << event->getName()
                  << " (priority: " << (int)event->getPriority() << ")"
                  << std::endl;
    } else if (auto transition =
                   std::dynamic_pointer_cast<TransitionOperator>(op)) {
      if (std::getenv("VESYLA_DEBUG"))
        std::cout << "  - Transition (delay: " << transition->getDelay()
                  << ", next: " << transition->getNextEventName() << ")"
                  << std::endl;
    } else if (auto repetition =
                   std::dynamic_pointer_cast<RepetitionOperator>(op)) {
      if (std::getenv("VESYLA_DEBUG"))
        std::cout << "  - Repetition (level: " << repetition->getLevel()
                  << ", iterations: " << repetition->getIterations()
                  << ", delay: " << repetition->getDelay()
                  << ", step: " << repetition->getStep() << ")" << std::endl;
    } else {
      if (std::getenv("VESYLA_DEBUG"))
        std::cout << "  - Unknown operator" << std::endl;
    }
  }
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
  // delay++; // delay is always incremented by 1

  auto repetition = std::make_shared<RepetitionOperator>(
      iterations, delay, level, step, expression);
  expression = std::static_pointer_cast<TimingExpression>(repetition);
  return *this;
}

TimingState &TimingState::addTransition(uint64_t delay,
                                        const std::string &nextEventName,
                                        std::function<void()> handler,
                                        uint8_t priority) {
  if (!expression) {
    throw std::runtime_error("Cannot add transition without an event");
  }
  this->operator_queue.push_back(std::make_shared<TransitionOperator>(
      delay, nextEventName, handler, expression, expression));
  return *this;
}

TimingState &
TimingState::addTransition(std::shared_ptr<TransitionOperator> transition) {
  if (std::getenv("VESYLA_DEBUG"))
    std::cout << "Adding transition to operator queue (delay: "
              << transition->getDelay()
              << ", next: " << transition->getNextEventName() << ")"
              << std::endl;
  printOperatorQueue();
  this->operator_queue.push_back(transition);
  if (std::getenv("VESYLA_DEBUG"))
    std::cout << "Current operator queue after adding transition:" << std::endl;
  printOperatorQueue();
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

  if (std::getenv("VESYLA_DEBUG"))
    std::cout << "Current operator queue after adding repetition (level: "
              << level << ", iterations: " << iterations << ", delay: " << delay
              << ", step: " << step << "):" << std::endl;
  printOperatorQueue();
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

TimingState &TimingState::build() {
  if (std::getenv("VESYLA_DEBUG"))
    std::cout << "Building timing state with " << operator_queue.size()
              << " operators." << std::endl;
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
      if (std::getenv("VESYLA_DEBUG"))
        std::cout << "Found transition operator (delay: " << trans->getDelay()
                  << ")" << std::endl;
      transition_count++;
    } else if (auto event =
                   std::dynamic_pointer_cast<TimingEvent>(operator_queue[i])) {
      if (std::getenv("VESYLA_DEBUG"))
        std::cout << "Found event: " << event->getName() << std::endl;
      if (std::getenv("VESYLA_DEBUG"))
        std::cout << "id: " << event->toString() << std::endl;
      // if(std::getenv("VESYLA_DEBUG")) std::cout << "Executing event handler
      // for event: " << test->getName()
      //           << std::endl;
      // test->getHandler()(); // this works, it means it preserves the
      // handler
      event_count++;
    } else if (auto rep = std::dynamic_pointer_cast<RepetitionOperator>(
                   operator_queue[i])) {
      if (std::getenv("VESYLA_DEBUG"))
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
    if (std::getenv("VESYLA_DEBUG"))
      std::cout << "First operator is an event" << std::endl;
    temp_state = TimingState(
        std::static_pointer_cast<TimingExpression>(operator_queue[0]));
  }

  // Add all operators to the expression
  std::deque<std::shared_ptr<TimingExpression>> expressions;
  std::map<uint64_t, uint64_t> addresses;
  std::map<uint64_t, std::map<uint64_t, uint64_t>> events_addresses;
  std::map<uint64_t, uint64_t> events_last_cycle;
  std::map<uint64_t, uint64_t> events_last_address;
  int64_t current_event_id = -1;
  uint64_t current_transition_id = 0;
  for (auto &op : operator_queue) {
    if (auto event = std::dynamic_pointer_cast<TimingEvent>(op)) {
      expressions.push_back(std::static_pointer_cast<TimingExpression>(event));
      // First address for the event uses per-event initial address if available
      current_event_id++;
      events_addresses[current_event_id] = std::map<uint64_t, uint64_t>();
      uint64_t init_addr =
          (current_event_id < (int64_t)eventInitialAddresses.size())
              ? eventInitialAddresses[current_event_id]
              : 0;
      events_addresses[current_event_id][0] = init_addr;
      events_last_cycle[current_event_id] = 0;
      events_last_address[current_event_id] = init_addr;
    } else if (auto transition =
                   std::dynamic_pointer_cast<TransitionOperator>(op)) {
      if (std::getenv("VESYLA_DEBUG"))
        std::cout << "Building transition operator (delay: "
                  << transition->getDelay() << ")" << std::endl;
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

      // fromEvent addresses
      if (std::getenv("VESYLA_DEBUG"))
        std::cout << "Addresses fromEvent:" << std::endl;
      std::map<uint64_t, uint64_t> fromEventAddresses;
      if (current_transition_id == 0) {
        fromEventAddresses = events_addresses[current_transition_id];
      } else {
        fromEventAddresses = addresses;
      }
      for (const auto &addr_pair : fromEventAddresses) {
        if (std::getenv("VESYLA_DEBUG"))
          std::cout << " Cycle " << addr_pair.first << " -> Address "
                    << addr_pair.second << std::endl;
      }

      // toEvent addresses
      if (std::getenv("VESYLA_DEBUG"))
        std::cout << "Addresses toEvent:" << std::endl;
      std::map<uint64_t, uint64_t> toEventAddresses =
          events_addresses[current_transition_id + 1];
      for (const auto &addr_pair : toEventAddresses) {
        if (std::getenv("VESYLA_DEBUG"))
          std::cout << " Cycle " << addr_pair.first << " -> Address "
                    << addr_pair.second << std::endl;
      }

      addresses = fromEventAddresses;
      uint64_t last_cycle = addresses.rbegin()->first;
      if (std::getenv("VESYLA_DEBUG"))
        std::cout << "Last cycle fromEvent: " << last_cycle << std::endl;
      for (const auto &addr_pair : toEventAddresses) {
        uint64_t addr_cycle =
            addr_pair.first + last_cycle + transition->getDelay() + 1;
        uint64_t addr_value = addr_pair.second;
        addresses[addr_cycle] = addr_value;
      }

      current_transition_id++;
      if (std::getenv("VESYLA_DEBUG"))
        std::cout << "Addresses after transition:" << std::endl;
      for (const auto &addr_pair : addresses) {
        if (std::getenv("VESYLA_DEBUG"))
          std::cout << " Cycle " << addr_pair.first << " -> Address "
                    << addr_pair.second << std::endl;
      }
    } else if (auto repetition =
                   std::dynamic_pointer_cast<RepetitionOperator>(op)) {
      if (std::getenv("VESYLA_DEBUG"))
        std::cout << "Building repetition operator (level: "
                  << repetition->getLevel()
                  << ", iterations: " << repetition->getIterations()
                  << ", delay: " << repetition->getDelay()
                  << ", step: " << repetition->getStep() << ")" << std::endl;

      levels_current_iteration.push_back(0);
      levels_total_iterations.push_back(repetition->getIterations());
      levels_step.push_back(repetition->getStep());

      if (current_transition_id == 0) {
        auto events_addresses_copy = events_addresses[current_event_id];
        for (int it = 0; it < repetition->getIterations() - 1; it++) {
          uint64_t last_cycle =
              events_addresses[current_event_id].rbegin()->first;
          uint64_t current_cycle = last_cycle + repetition->getDelay() + 1;
          for (const auto &addr_pair : events_addresses_copy) {
            uint64_t addr_cycle = addr_pair.first + current_cycle;
            uint64_t addr_value =
                addr_pair.second + (it + 1) * repetition->getStep();
            events_addresses[current_event_id][addr_cycle] = addr_value;
          }
        }
        if (std::getenv("VESYLA_DEBUG"))
          std::cout << "Addresses event " << current_event_id
                    << " after repetition:" << std::endl;
        for (const auto &addr_pair : events_addresses[current_event_id]) {
          if (std::getenv("VESYLA_DEBUG"))
            std::cout << " Cycle " << addr_pair.first << " -> Address "
                      << addr_pair.second << std::endl;
        }
      } else {
        // Repeat the whole addresses
        auto addresses_copy = addresses;
        for (int it = 0; it < repetition->getIterations() - 1; it++) {
          uint64_t last_cycle = addresses.rbegin()->first;
          uint64_t current_cycle = last_cycle + repetition->getDelay() + 1;
          for (const auto &addr_pair : addresses_copy) {
            uint64_t addr_cycle = addr_pair.first + current_cycle;
            uint64_t addr_value =
                addr_pair.second + (it + 1) * repetition->getStep();
            addresses[addr_cycle] = addr_value;
          }
        }
        if (std::getenv("VESYLA_DEBUG"))
          std::cout << "Addresses after repetition:" << std::endl;
        for (const auto &addr_pair : addresses) {
          if (std::getenv("VESYLA_DEBUG"))
            std::cout << " Cycle " << addr_pair.first << " -> Address "
                      << addr_pair.second << std::endl;
        }
      }

      std::shared_ptr<TimingExpression> expr = expressions.back();
      expressions.pop_back();
      std::shared_ptr<RepetitionOperator> repetition_op =
          std::make_shared<RepetitionOperator>(
              repetition->getIterations(), repetition->getDelay(),
              repetition->getLevel(), repetition->getStep(), expr);
      expressions.push_back(
          std::static_pointer_cast<TimingExpression>(repetition_op));
    } else {
      throw std::runtime_error("Unknown operator type");
    }

    this->expression = expressions.back();
  }

  // If no transitions were added, generate addresses now
  if (current_transition_id == 0) {
    uint64_t cycle = addresses.empty() ? 0 : addresses.rbegin()->first + 1;
    for (const auto &addr_pair : events_addresses[current_transition_id]) {
      uint64_t addr_cycle = addr_pair.first + cycle;
      uint64_t addr_value = addr_pair.second;
      addresses[addr_cycle] = addr_value;
    }
  }

  // Schedule events
  if (std::getenv("VESYLA_DEBUG")) {
    // Print type of expression (TimingEvent, TransitionOperator,
    // RepetitionOperator)
    if (std::dynamic_pointer_cast<TimingEvent>(expression)) {
      std::cout << "Expression is a TimingEvent: "
                << std::dynamic_pointer_cast<TimingEvent>(expression)->getName()
                << std::endl;
    } else if (std::dynamic_pointer_cast<TransitionOperator>(expression)) {
      std::cout << "Expression is a TransitionOperator (next event: "
                << std::dynamic_pointer_cast<TransitionOperator>(expression)
                       ->getNextEventName()
                << ")" << std::endl;
    } else if (std::dynamic_pointer_cast<RepetitionOperator>(expression)) {
      std::cout << "Expression is a RepetitionOperator (level: "
                << std::dynamic_pointer_cast<RepetitionOperator>(expression)
                       ->getLevel()
                << ", iterations: "
                << std::dynamic_pointer_cast<RepetitionOperator>(expression)
                       ->getIterations()
                << ", delay: "
                << std::dynamic_pointer_cast<RepetitionOperator>(expression)
                       ->getDelay()
                << ", step: "
                << std::dynamic_pointer_cast<RepetitionOperator>(expression)
                       ->getStep()
                << ")" << std::endl;
    } else {
      std::cout << "Expression is an unknown type" << std::endl;
    }
    std::cout << "Scheduling events for expression: " << expression->toString()
              << std::endl;
  }
  updateLastScheduledCycle(
      this->expression->scheduleEvents(*this, lastScheduledCycle));
  lastScheduledCycle = addresses.rbegin()->first;

  if (std::getenv("VESYLA_DEBUG"))
    std::cout << "Addresses scheduled:" << std::endl;
  for (const auto &addr_pair : addresses) {
    if (std::getenv("VESYLA_DEBUG"))
      std::cout << " Cycle " << addr_pair.first << " -> Address "
                << addr_pair.second << std::endl;
  }
  if (std::getenv("VESYLA_DEBUG"))
    std::cout << "Build complete, last scheduled cycle: " << lastScheduledCycle
              << std::endl;
  generatedAddresses = addresses;

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
  if (std::getenv("VESYLA_DEBUG"))
    std::cout << "Getting events for cycle " << cycle << " in scheduledEvents ("
              << scheduledEvents.size() << " entries)" << std::endl;
  // print all cycles where these is an event scheduled for debugging
  if (std::getenv("VESYLA_DEBUG")) {
    std::cout << "Scheduled events cycles:" << std::endl;
    for (const auto &entry : scheduledEvents) {
      std::cout << " Cycle " << entry.first << " -> " << entry.second.size()
                << " events" << std::endl;
    }
  }
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

int64_t TimingState::getAddressForCycle(uint64_t cycle) {
  int64_t address = -1;
  auto it = generatedAddresses.find(cycle);
  if (it != generatedAddresses.end()) {
    address = it->second;
  }
  return address;
}

void TimingState::setEventInitialAddresses(
    const std::vector<uint64_t> &addresses) {
  eventInitialAddresses = addresses;
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
