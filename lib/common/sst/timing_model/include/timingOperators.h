#pragma once
#include "timingExpression.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

class TimingOperator : public TimingExpression {
public:
  virtual ~TimingOperator() {}
};

class TransitionOperator : public TimingOperator {
private:
  uint64_t delay;
  std::string nextEventName;
  std::function<void()> handler;
  std::shared_ptr<TimingExpression> from;
  std::shared_ptr<TimingExpression> to;

public:
  TransitionOperator(uint64_t delay, std::string nextEventName,
                     std::function<void()> handler,
                     std::shared_ptr<TimingExpression> from,
                     std::shared_ptr<TimingExpression> to);

  uint64_t getDelay() const;
  std::string getNextEventName() const;
  std::function<void()> getHandler() const;
  std::shared_ptr<TimingExpression> getFrom() const;
  std::shared_ptr<TimingExpression> getTo() const;

  uint64_t scheduleEvents(TimingState &state,
                          uint64_t startCycle) const override;
  std::string toString() const override;
  uint64_t lastEventId() const override;
  std::string lastEventName() const override;

  // TimingState toTimingState() const;
};

class RepetitionOperator : public TimingOperator {
private:
  uint64_t iterations;
  uint64_t delay;
  uint64_t level;
  uint64_t step;
  std::shared_ptr<TimingExpression> expression;

public:
  RepetitionOperator(uint64_t iterations, uint64_t delay, uint64_t level,
                     uint64_t step,
                     std::shared_ptr<TimingExpression> expression);

  uint64_t getIterations() const;
  uint64_t getDelay() const;
  uint64_t getLevel() const;
  uint64_t getStep() const;
  void setIterations(uint64_t iterations) { this->iterations = iterations; }
  void setDelay(uint64_t delay) { this->delay = delay; }
  void setStep(uint64_t step) { this->step = step; }
  std::shared_ptr<TimingExpression> getExpression() const;

  uint64_t scheduleEvents(TimingState &state,
                          uint64_t startCycle) const override;
  std::string toString() const override;
  uint64_t lastEventId() const override;
  std::string lastEventName() const override;
};
