#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

class TimingState;
class TimingEvent;
class TimingOperator;
class TransitionOperator;
class RepetitionOperator;

class TimingExpression {
public:
  virtual ~TimingExpression() {}
  virtual uint64_t scheduleEvents(TimingState &state,
                                  uint64_t startCycle) const;
  virtual std::string toString() const = 0;
  virtual uint64_t lastEventId() const;
  virtual std::string lastEventName() const;
  std::shared_ptr<TimingExpression> parent_expression;
};

class TimingState {
private:
  std::shared_ptr<TimingExpression> expression;
  uint64_t eventCounter;
  uint64_t lastScheduledCycle;
  std::vector<uint64_t> levels_current_iteration;
  std::vector<uint64_t> levels_total_iterations;
  std::vector<uint64_t> levels_step;

  std::vector<std::shared_ptr<TimingOperator>> operator_queue;
  std::shared_ptr<TimingExpression> event0;

  TimingState &buildRepetition(uint64_t iterations, uint64_t delay);
  TimingState &buildRepetition(uint64_t iterations, uint64_t delay,
                               uint64_t level, uint64_t step);
  TimingState &buildTransition(uint64_t delay, const std::string &nextEventName,
                               std::function<void()> handler);
  TimingState &buildTransition(uint64_t delay, const std::string &nextEventName,
                               uint8_t priority, std::function<void()> handler);

public:
  TimingState();
  TimingState(std::shared_ptr<TimingExpression> expression);

  uint64_t currentCycle = 0;
  std::map<uint64_t, std::set<std::shared_ptr<const TimingEvent>>>
      scheduledEvents;
  std::vector<std::string> eventNames;

  uint64_t getLastScheduledCycle() const;
  void updateLastScheduledCycle(uint64_t cycle);
  static TimingState createFromEvent(std::shared_ptr<TimingEvent> event);
  static TimingState createFromEvent(const std::string &name);
  static TimingState
  createFromExpression(std::shared_ptr<TimingExpression> expression);
  TimingState &addEvent(std::shared_ptr<TimingEvent> event);
  TimingState &addEvent(const std::string &name, std::function<void()> handler);
  TimingState &addEvent(const std::string &name, uint8_t priority,
                        std::function<void()> handler);
  TimingState &addTransition(uint64_t delay, const std::string &nextEventName,
                             std::function<void()> handler);
  TimingState &addTransition(uint64_t delay, const std::string &nextEventName,
                             uint8_t priority, std::function<void()> handler);
  TimingState &addRepetition(uint64_t iterations, uint64_t delay);
  TimingState &addRepetition(uint64_t iterations, uint64_t delay,
                             uint64_t level, uint64_t step);
  TimingState &adjustRepetition(uint64_t iterations, uint64_t delay,
                                uint64_t level, uint64_t step);
  uint64_t getRepIncrementForCycle(uint64_t cycle);
  void incrementLevels(void);
  TimingState &build();
  std::shared_ptr<TimingExpression> getExpression() const;
  void scheduleEvent(std::shared_ptr<const TimingEvent> event, uint64_t cycle);
  std::set<std::shared_ptr<const TimingEvent>>
  getEventsForCycle(uint64_t cycle);
  bool findEventByName(const std::string &name);
  void addEventName(const std::string &name);
  std::string toString() const;

  const RepetitionOperator &
  getRepetitionOperatorFromLevel(uint64_t level) const;

  void print(std::ostream &os);
};
