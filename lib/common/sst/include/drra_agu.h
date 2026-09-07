#include "timingExpression.h"
#include "timingModel.h"
#include "timingOperators.h"

class DRRA_AGU {
private:
  std::unique_ptr<TimingState> timing_state = nullptr;

  std::vector<TimingState> lanes;
  std::vector<uint64_t> lane_initial_addresses;
  size_t current_lane_index = 0;
  size_t current_trans_index = 0;
  uint64_t current_rep_level = 0;
  uint64_t initial_address = 0;
  // Address offset terms, one per loop dimension: evt sets term 0, evts
  // appends the rest. Empty = no offset.
  std::vector<uint64_t> term_strides;
  std::vector<uint64_t> term_loop_vars;
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

  // Extend the initial address with the evtx high bits. addEvent already
  // snapshotted initial_address into a lane, so patch that lane too: evtx
  // configures the same lane as the preceding evt.
  void setInitialAddressHigh(uint64_t high_bits, uint32_t low_width) {
    uint64_t low_mask = (low_width >= 64) ? ~0ULL : ((1ULL << low_width) - 1);
    if (!lane_initial_addresses.empty()) {
      uint64_t &a = lane_initial_addresses.back();
      a = (a & low_mask) | (high_bits << low_width);
    }
    initial_address = (initial_address & low_mask) | (high_bits << low_width);
  }

  // Rebuilt by the resource on every activation: `stride` is static config
  // (evt/evts), `loop_var` the counter broadcast for that term's loop level.
  // Lets a resource replay its pattern shifted without redoing address setup.
  void clearOffsetTerms() {
    term_strides.clear();
    term_loop_vars.clear();
  }
  void addOffsetTerm(uint64_t stride, uint64_t loop_var) {
    term_strides.push_back(stride);
    term_loop_vars.push_back(loop_var);
  }

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
