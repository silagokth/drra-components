#include "timingModel.h"
#include <cstdint>
#include <functional>
#include <string>

// TimingEvent implementations
TimingEvent::TimingEvent(const std::string &name, uint64_t eventNumber)
    : name(name), eventNumber(eventNumber) {}

TimingEvent::TimingEvent(const std::string &name, uint64_t eventNumber,
                         std::function<void()> handler, uint8_t priority)
    : name(name), eventNumber(eventNumber), handler(handler),
      priority(priority) {}

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
