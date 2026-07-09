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
    : addr_segs(other.addr_segs), addr_outerDims(other.addr_outerDims),
      addr_last_cycle(other.addr_last_cycle), expression(other.expression),
      eventCounter(other.eventCounter),
      lastScheduledCycle(other.lastScheduledCycle),
      eventNames(other.eventNames), operator_queue(other.operator_queue),
      eventInitialAddresses(other.eventInitialAddresses),
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
    addr_segs = other.addr_segs;
    addr_outerDims = other.addr_outerDims;
    addr_last_cycle = other.addr_last_cycle;
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

  // Empty operator queue: either nothing was ever added (error) or the state
  // holds a single event created via createFromEvent() whose event lives in
  // `expression` rather than the queue. Schedule that lone event at cycle 0.
  if (operator_queue.empty()) {
    if (!expression)
      throw std::runtime_error("Cannot build timing state without any events");
    EventSeg seg;
    seg.base_addr = eventInitialAddresses.empty() ? 0 : eventInitialAddresses[0];
    seg.event = std::dynamic_pointer_cast<const TimingEvent>(expression);
    addr_segs.clear();
    if (seg.event)
      addr_segs.push_back(seg);
    addr_outerDims.clear();
    addr_last_cycle = 0;
    lastScheduledCycle = 0;
    return *this;
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

  // Assemble the final expression (for toString/debug and transition chaining)
  // and, in the same pass, build the compact lazy address model.
  //
  //   * An event starts a new segment.
  //   * A repetition before any transition is an inner rep of the current
  //     segment; after a transition it is a global/outer rep applied to every
  //     segment.
  //   * A transition places the next event's segment after the accumulated
  //     concatenation span (plus the transition delay).
  //
  // For a single rep level l with (iter, delay, step) applied on top of a
  // structure already spanning `span` cycles:
  //     period      = span + delay + 1        (stride between iterations)
  //     new span    = (iter - 1) * period + span
  // The address contribution is index_l * step, summed across levels. This is
  // exactly the recurrence the old per-cycle enumeration produced, but stored
  // in O(#levels) instead of O(#cycles).
  std::deque<std::shared_ptr<TimingExpression>> expressions;

  std::vector<EventSeg> segs;
  std::vector<Dim> outerDims;
  int64_t current_event_id = -1;
  bool transitions_started = false;
  uint64_t concat_span = 0; // span of the accumulated concatenation
  uint64_t trans_count = 0;

  // Raw TimingState API (createFromEvent + addRepetition/addTransition) leaves
  // the base event in `expression` instead of pushing it onto the queue, so the
  // queue can start with a repetition or transition. Seed the first segment
  // from that base event before processing the operators.
  if (!std::dynamic_pointer_cast<TimingEvent>(operator_queue.front()) &&
      expression) {
    expressions.push_back(expression);
    current_event_id = 0;
    EventSeg seg;
    seg.base_addr = eventInitialAddresses.empty() ? 0 : eventInitialAddresses[0];
    seg.event = std::dynamic_pointer_cast<const TimingEvent>(expression);
    segs.push_back(seg);
  }

  for (auto &op : operator_queue) {
    if (auto event = std::dynamic_pointer_cast<TimingEvent>(op)) {
      expressions.push_back(std::static_pointer_cast<TimingExpression>(event));

      current_event_id++;
      EventSeg seg;
      seg.base_addr =
          (current_event_id < (int64_t)eventInitialAddresses.size())
              ? eventInitialAddresses[current_event_id]
              : 0;
      seg.event = std::static_pointer_cast<const TimingEvent>(event);
      segs.push_back(seg);
    } else if (auto transition =
                   std::dynamic_pointer_cast<TransitionOperator>(op)) {
      if (std::getenv("VESYLA_DEBUG"))
        std::cout << "Building transition operator (delay: "
                  << transition->getDelay() << ")" << std::endl;
      std::shared_ptr<TimingExpression> from_expr = expressions.front();
      expressions.pop_front();
      // The destination event is either already queued (DRRA_AGU pushes both
      // events) or must be synthesized from the transition's next-event name
      // (raw string addTransition, which carries only the name).
      bool synthesized_to = expressions.empty();
      std::shared_ptr<TimingExpression> to_expr;
      if (!synthesized_to) {
        to_expr = expressions.front();
        expressions.pop_front();
      } else {
        auto toEvent = std::make_shared<TimingEvent>(
            transition->getNextEventName(), eventCounter++);
        toEvent->setHandler(transition->getHandler());
        to_expr = std::static_pointer_cast<TimingExpression>(toEvent);
      }
      std::shared_ptr<TransitionOperator> transition_op =
          std::make_shared<TransitionOperator>(
              transition->getDelay(), transition->getNextEventName(),
              transition->getHandler(), from_expr, to_expr);
      // Place the new expression back to the front of the queue
      expressions.push_front(
          std::static_pointer_cast<TimingExpression>(transition_op));

      // Concatenate the next event's segment after the accumulated span.
      if (!transitions_started) {
        transitions_started = true;
        concat_span = segs[0].inner_span;
      }
      // The destination segment is pre-created for queued events; synthesize it
      // here for the string-API path.
      if (trans_count + 1 >= segs.size()) {
        EventSeg seg;
        seg.base_addr = (trans_count + 1 < eventInitialAddresses.size())
                            ? eventInitialAddresses[trans_count + 1]
                            : 0;
        seg.event = std::dynamic_pointer_cast<const TimingEvent>(to_expr);
        segs.push_back(seg);
      }
      EventSeg &toSeg = segs[trans_count + 1];
      toSeg.base_cycle = concat_span + transition->getDelay() + 1;
      concat_span = toSeg.base_cycle + toSeg.inner_span;
      trans_count++;
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

      uint64_t iter = repetition->getIterations();
      uint64_t delay = repetition->getDelay();
      uint64_t step = repetition->getStep();
      if (!transitions_started) {
        // Inner repetition of the current segment.
        EventSeg &s = segs[current_event_id];
        uint64_t period = s.inner_span + delay + 1;
        s.innerDims.push_back({period, iter, step});
        s.inner_span = (iter - 1) * period + s.inner_span;
      } else {
        // Global/outer repetition applied to the whole concatenation.
        uint64_t period = concat_span + delay + 1;
        outerDims.push_back({period, iter, step});
        concat_span = (iter - 1) * period + concat_span;
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

  if (!transitions_started) {
    // No transitions: only the first event's lattice is exposed, matching the
    // legacy single-event address generation (extra events, if any, were
    // discarded there too).
    if (segs.size() > 1)
      segs.resize(1);
    concat_span = segs.empty() ? 0 : segs[0].inner_span;
  }

  addr_segs = std::move(segs);
  addr_outerDims = std::move(outerDims);
  addr_last_cycle = concat_span;
  lastScheduledCycle = concat_span;

  if (std::getenv("VESYLA_DEBUG"))
    std::cout << "Build complete, last scheduled cycle: " << lastScheduledCycle
              << std::endl;

  return *this;
}

std::shared_ptr<TimingExpression> TimingState::getExpression() const {
  return expression;
}

void TimingState::scheduleEvent(std::shared_ptr<const TimingEvent> event,
                                uint64_t cycle) {
  // No-op: the lazy address model derives events on demand in
  // getEventsForCycle. Kept because TimingEvent::scheduleEvents still calls it.
  (void)event;
  (void)cycle;
}

int64_t TimingState::decomposeDims(uint64_t offset,
                                   const std::vector<Dim> &dims) {
  uint64_t addr = 0;
  // Outermost (largest period) first: each level's period exceeds the span of
  // all inner levels, so the quotient is the unique index at that level.
  for (size_t k = dims.size(); k-- > 0;) {
    const Dim &d = dims[k];
    uint64_t idx = d.period ? offset / d.period : 0;
    if (idx >= d.iter)
      return -1; // beyond this level's iteration count
    addr += idx * d.step;
    offset -= idx * d.period;
  }
  if (offset != 0)
    return -1; // not on the lattice (gap cycle)
  return static_cast<int64_t>(addr);
}

std::set<std::shared_ptr<const TimingEvent>>
TimingState::getEventsForCycle(uint64_t cycle) {
  std::set<std::shared_ptr<const TimingEvent>> result;
  uint64_t off = cycle;
  // Strip outer repetition dimensions (they do not change which event fires).
  for (size_t k = addr_outerDims.size(); k-- > 0;) {
    const Dim &d = addr_outerDims[k];
    uint64_t idx = off / d.period;
    if (idx >= d.iter)
      return result;
    off -= idx * d.period;
  }
  for (const auto &s : addr_segs) {
    if (off < s.base_cycle)
      break; // fell into a gap before this segment
    if (off <= s.base_cycle + s.inner_span) {
      if (decomposeDims(off - s.base_cycle, s.innerDims) >= 0 && s.event)
        result.insert(s.event);
      return result;
    }
  }
  return result;
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
  uint64_t off = cycle;
  uint64_t outer_addr = 0;
  // Decompose the outer repetition dimensions (outermost first).
  for (size_t k = addr_outerDims.size(); k-- > 0;) {
    const Dim &d = addr_outerDims[k];
    uint64_t idx = off / d.period;
    if (idx >= d.iter)
      return -1; // beyond the outermost repetition -> gap
    outer_addr += idx * d.step;
    off -= idx * d.period;
  }
  // Locate the segment whose inner lattice contains the remaining offset.
  for (const auto &s : addr_segs) {
    if (off < s.base_cycle)
      break; // gap before this segment
    if (off <= s.base_cycle + s.inner_span) {
      int64_t inner = decomposeDims(off - s.base_cycle, s.innerDims);
      if (inner < 0)
        return -1; // gap inside the segment's inner lattice
      return static_cast<int64_t>(s.base_addr + static_cast<uint64_t>(inner) +
                                  outer_addr);
    }
  }
  return -1;
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
