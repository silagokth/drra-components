#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

class TimingExpression;
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
};

class TimingState {
private:
  std::shared_ptr<TimingExpression> expression;
  uint64_t eventCounter = 0;
  uint64_t lastScheduledCycle = 0;
  std::map<uint64_t, uint64_t> generatedAddresses;
  std::vector<uint64_t> levels_current_iteration;
  std::vector<uint64_t> levels_total_iterations;
  std::vector<uint64_t> levels_step;

  std::vector<std::shared_ptr<TimingExpression>> operator_queue;
  std::vector<uint64_t> eventInitialAddresses;

  TimingState &buildRepetition(uint64_t iterations, uint64_t delay);
  TimingState &buildRepetition(uint64_t iterations, uint64_t delay,
                               uint64_t level, uint64_t step);
  TimingState &buildTransition(uint64_t delay, const std::string &nextEventName,
                               std::function<void()> handler);
  TimingState &buildTransition(uint64_t delay, const std::string &nextEventName,
                               uint8_t priority, std::function<void()> handler);

public:
  TimingState();
  ~TimingState() = default;
  TimingState(const TimingState &other);
  TimingState(std::shared_ptr<TimingExpression> expression);
  TimingState(const TimingState &fromState, const TimingState &toState,
              std::shared_ptr<TransitionOperator> transition);
  TimingState(std::shared_ptr<TransitionOperator> transition);

  // Operator overloads for chaining
  TimingState &operator=(const TimingState &other);

  void moveTransitionsToEnd();

  uint64_t currentCycle = 0;
  std::map<uint64_t, std::set<std::shared_ptr<const TimingEvent>>>
      scheduledEvents;
  std::vector<std::string> eventNames;

  uint64_t getLastScheduledCycle() const;
  void updateLastScheduledCycle(uint64_t cycle);
  static TimingState createFromEvent(const std::string &name);

  void printOperatorQueue() const;

  TimingState &addEvent(
      const std::string &name = "", std::function<void()> handler = [] {},
      uint8_t priority = 5);
  TimingState &addRepetition(uint64_t iterations, uint64_t delay);
  TimingState &addRepetition(uint64_t iterations, uint64_t delay,
                             uint64_t level, uint64_t step);
  TimingState &adjustRepetition(uint64_t iterations, uint64_t delay,
                                uint64_t level, uint64_t step);
  TimingState &addTransition(std::shared_ptr<TransitionOperator> transition);
  TimingState &addTransition(
      uint64_t delay = 0, const std::string &nextEventName = "",
      std::function<void()> handler = [] {}, uint8_t priority = 5);

  void incrementLevels(void);
  TimingState &build();
  std::shared_ptr<TimingExpression> getExpression() const;
  void scheduleEvent(std::shared_ptr<const TimingEvent> event, uint64_t cycle);
  std::set<std::shared_ptr<const TimingEvent>>
  getEventsForCycle(uint64_t cycle);
  bool findEventByName(const std::string &name);
  void addEventName(const std::string &name);
  std::string toString() const;

  RepetitionOperator getRepetitionOperatorFromLevel(uint64_t level) const;
  int64_t getAddressForCycle(uint64_t cycle);
  void setEventInitialAddresses(const std::vector<uint64_t> &addresses);
  void copyLevelData(const TimingState &other);
  const std::vector<uint64_t> &getLevelsStep() const;
  const std::vector<uint64_t> &getLevelsTotalIterations() const;
};
