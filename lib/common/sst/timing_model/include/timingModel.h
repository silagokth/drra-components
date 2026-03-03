#pragma once

#include "timingExpression.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

class TimingEvent : public TimingExpression,
                    public std::enable_shared_from_this<TimingEvent> {
private:
  std::string name;
  uint64_t eventNumber;
  std::function<void()> handler;
  uint8_t priority;

public:
  TimingEvent(const std::string &name, uint64_t eventNumber);
  TimingEvent(std::shared_ptr<const TimingEvent> other);

  uint64_t scheduleEvents(TimingState &state,
                          uint64_t startCycle) const override;
  std::string toString() const override;
  std::string getName() const;
  uint64_t getEventNumber() const;
  uint64_t lastEventId() const override;
  std::string lastEventName() const override;
  void execute() const;
  void setHandler(std::function<void()> handler);
  std::function<void()> getHandler() const;

  void setPriority(uint8_t priority);
  uint8_t getPriority() const;
};
