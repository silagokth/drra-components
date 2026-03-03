#include "timingExpression.h"
#include "timingModel.h"
#include "timingOperators.h"

class DRRA_AGU {
private:
  std::unique_ptr<TimingState> timing_state = nullptr;

  std::vector<TimingState> lanes;
  size_t current_lane_index = 0;
  size_t current_trans_index = 0;
  uint64_t current_rep_level = 0;
  uint64_t initial_address = 0;
  TimingState *getCurrentLane();
  TimingState *getLaneAtIndex(size_t index);
  void printLaneExpressions() const;

public:
  DRRA_AGU() {};

  DRRA_AGU(const DRRA_AGU &) = delete;
  DRRA_AGU &operator=(const DRRA_AGU &) = delete;

  DRRA_AGU(DRRA_AGU &&) = default;
  DRRA_AGU &operator=(DRRA_AGU &&) = default;

  DRRA_AGU &addEvent(const std::string &name, std::function<void()> handler,
                     uint8_t priority = 5);

  DRRA_AGU &addRepetition(uint64_t iterations = 1, uint64_t delay = 0,
                          uint64_t step = 1);
  RepetitionOperator getLastRepetitionOperator();
  DRRA_AGU &adjustRepetition(uint64_t iterations = 1, uint64_t delay = 0,
                             uint64_t step = 1);

  DRRA_AGU &addTransition(uint64_t delay = 0);

  bool isEmpty() {
    if (!timing_state)
      if (lanes.empty())
        return true;
    return false;
  }

  DRRA_AGU &build();

  std::set<std::shared_ptr<const TimingEvent>>
  getEventsForCycle(uint64_t cycle);

  DRRA_AGU &reset();

  void setInitialAddress(uint64_t address) { initial_address = address; }

  int64_t getAddressForCycle(uint64_t cycle);
  uint64_t getLastScheduledCycle();

  std::string getTimingExpressionString() {
    if (timing_state) {
      return timing_state->getExpression()->toString();
    } else {
      return "No timing state built";
    }
  }
};
