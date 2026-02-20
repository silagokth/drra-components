#include "drra_agu.h"
#include "svdpi.h"
#include "timingModel.h"
#include <deque>
#include <iostream>
#include <map>
#include <memory>
#include <vector>

static std::deque<int> expected_addresses;
static std::deque<int> expected_cycles;
std::string timing_expression = "N/A";

// Export with C linkage so the DPI loader can find the exact names
extern "C" {

// Build pattern called from SV: import "DPI-C" context function void
// cpp_build_pattern(...)
void cpp_build_pattern(const svOpenArrayHandle type_h,
                       const svOpenArrayHandle iter_h,
                       const svOpenArrayHandle delay_h,
                       const svOpenArrayHandle step_h, bool debug = false) {
  expected_addresses.clear();

  DRRA_AGU agu;

  std::shared_ptr<TransitionOperator> prev_trans;
  int rep_level = 0;
  int trans_level = 0;
  int event_count = 0;
  int num_levels = svSize(type_h, 1);
  if (debug)
    std::cerr << "[DPI] Building states for " << num_levels << " levels"
              << std::endl;
  for (int i = 0; i < num_levels; i++) {
    int type = 0;
    int iter = 0;
    int delay = 0;
    int step = 0;
    int *type_ptr = (int *)svGetArrElemPtr1(type_h, i);
    int *iter_ptr = (int *)svGetArrElemPtr1(iter_h, i);
    int *delay_ptr = (int *)svGetArrElemPtr1(delay_h, i);
    int *step_ptr = (int *)svGetArrElemPtr1(step_h, i);
    if (type_ptr)
      type = *type_ptr;
    if (iter_ptr)
      iter = *iter_ptr;
    if (delay_ptr)
      delay = *delay_ptr;
    if (step_ptr)
      step = *step_ptr;

    if (debug)
      std::cerr << "[DPI] - LVL " << i;

    if (type == 0) { // REP
      if (iter > 0) {
        if (debug)
          std::cerr << " (repetition) iter=" << iter << " delay=" << delay
                    << " step=" << step << std::endl;
        agu.addRepetition(iter, delay, step);
      }
    } else if (type == 1) { // TRANS
      if (debug)
        std::cerr << " (transition) delay=" << delay << std::endl;
      agu.addTransition(delay);
    } else if (type == 2) { // Event
      if (debug)
        std::cerr << " (event " << event_count << ")" << std::endl;
      agu.addEvent("event_" + std::to_string(event_count), [] {});
      event_count++;
    }
  }

  agu.build();

  timing_expression = agu.getTimingExpressionString();
  if (debug)
    std::cerr << "[DPI] final state built: " << timing_expression << std::endl;

  // Pattern generation - need to track address counters per event
  uint64_t max_cycles = agu.getLastScheduledCycle() + 1;

  if (debug)
    std::cerr << "[DPI] final state built. Generating pattern..." << std::endl;
  if (debug)
    std::cerr << "[DPI] state built: max_cycles=" << max_cycles << std::endl;

  // Build a map of event names to their address generation state
  std::map<std::string, std::vector<uint64_t>> event_address_counters;
  std::map<std::string, std::vector<uint64_t>> event_total_iterations;
  std::map<std::string, std::vector<uint64_t>> event_steps;
  std::map<std::string, int>
      event_index_map; // Map event names to their index for counter mode

  for (uint64_t cycle = 0; cycle <= max_cycles; cycle++) {
    // Calculate address for this event using its own counters
    int address = agu.getAddressForCycle(cycle);
    if (address != -1) {
      if (debug)
        std::cerr << "[DPI] cycle " << cycle
                  << ": final address from state: " << address << std::endl;
      expected_addresses.push_back(address);
      expected_cycles.push_back(cycle);
    }
  }
  if (debug)
    std::cerr << "[DPI] pattern generated: " << expected_addresses.size()
              << " addresses\n"
              << std::endl;
}

// Pop one address
int cpp_pop_expected_address() {
  if (expected_addresses.empty())
    return -1;
  int v = expected_addresses.front();
  expected_addresses.pop_front();
  return v;
}

int cpp_pop_expected_cycle() {
  if (expected_cycles.empty())
    return -1;
  int v = expected_cycles.front();
  expected_cycles.pop_front();
  return v;
}

// Return queue size
int cpp_get_address_queue_size() {
  return static_cast<int>(expected_addresses.size());
}

int cpp_get_cycle_queue_size() {
  return static_cast<int>(expected_cycles.size());
}

const char *cpp_get_timing_expression() { return timing_expression.c_str(); }

} // extern "C"
