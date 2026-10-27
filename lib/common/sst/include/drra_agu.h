#pragma once

#include "timingExpression.h"
#include "timingModel.h"
#include "timingOperators.h"

// Configuration is recorded the way agu_controller.sv registers it and only
// assembled into a timing state by build(), so EVT and TRANS may arrive in any
// order: each EVT opens the next lane (ir_configs[lane]), each TRANS fills the
// next transition (mt_configs[k], lane k -> lane k+1) and switches every later
// REP to an outer repetition (or_configs[level]). As in mt_ir.sv every opened
// lane runs, a lane without a TRANS of its own following on with no delay.
class DRRA_AGU {
private:
  struct OuterRepetition {
    uint64_t iterations = 0;
    uint64_t delay = 0;
    uint64_t step = 0;
    bool is_configured = false;
  };

  std::unique_ptr<TimingState> timing_state = nullptr;

  std::vector<TimingState> lanes;
  std::vector<uint64_t> lane_initial_addresses;
  std::vector<uint64_t> transition_delays;     // mt_configs
  std::vector<OuterRepetition> outer_reps;     // or_configs, by level
  bool use_outer = false;                      // use_or_reg
  bool outer_configured = false;               // or_configured_reg
  size_t current_lane_index = 0;
  uint64_t current_rep_level = 0;
  uint64_t initial_address = 0;
  void checkNotBuilt(const std::string &what) const;
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

  bool isEmpty() { return !timing_state && lanes.empty(); }

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
