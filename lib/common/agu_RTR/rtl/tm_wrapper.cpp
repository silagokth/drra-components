// wrapper.cpp (or tm_wrapper.cpp) -- compile this into timingModel.so
#include "svdpi.h"
#include "timingModel.h"
#include <deque>
#include <iostream>
#include <map>
#include <memory>
#include <vector>

static std::deque<int> expected_addresses;
static std::deque<int> expected_cycles;

// Export with C linkage so the DPI loader can find the exact names
extern "C" {

// Build pattern called from SV: import "DPI-C" context function void
// cpp_build_pattern(...)
void cpp_build_pattern(const svOpenArrayHandle type_h,
                       const svOpenArrayHandle iter_h,
                       const svOpenArrayHandle delay_h, int num_levels) {
  // Basic sanity print for debugging (remove later)
  std::cerr << "[DPI] cpp_build_pattern(): num_levels=" << num_levels
            << std::endl;

  expected_addresses.clear();

  // Your existing code for building the TimingState...
  TimingState state = TimingState();

  std::vector<TimingState> states;
  std::shared_ptr<TransitionOperator> prev_trans;
  int rep_level = 0;
  int trans_level = 0;
  int event_count = 0;
  std::map<std::string, int> event_max_iter;
  std::cerr << "[DPI] Building states..." << std::endl;
  for (int i = 0; i < num_levels; ++i) {
    int type = 0;
    int iter = 0;
    int delay = 0;
    int *type_ptr = (int *)svGetArrElemPtr1(type_h, i);
    int *iter_ptr = (int *)svGetArrElemPtr1(iter_h, i);
    int *delay_ptr = (int *)svGetArrElemPtr1(delay_h, i);
    if (type_ptr)
      type = *type_ptr;
    if (iter_ptr)
      iter = *iter_ptr;
    if (delay_ptr)
      delay = *delay_ptr;

    std::cerr << "[DPI] - " << i << ": type=" << type << " iter=" << iter
              << " delay=" << delay << std::endl;

    if (type == 0) {
      if (iter > 0) {
        std::cerr << "[DPI] " << i << " repetition iter=" << iter
                  << " delay=" << delay << std::endl;
        if (rep_level == 0) {
          event_max_iter["Event_" + std::to_string(event_count - 1)] = iter;
          std::cerr << "[DPI]   event max iter set: Event_" << (event_count - 1)
                    << " -> " << iter << std::endl;
        }
        TimingState new_state =
            TimingState(states[states.size() - 1].getExpression());
        new_state.addRepetition(iter, delay, rep_level, 0);
        new_state.build();
        states[states.size() - 1] = new_state;
        rep_level++;
      }
    } else if (type == 1) {
      std::cerr << "[DPI] " << i << " transition delay=" << delay << std::endl;
      if (trans_level == 0) {
        TransitionOperator trans(
            delay, "name", [] {}, states[0].getExpression(),
            states[1].getExpression());
        prev_trans = std::make_shared<TransitionOperator>(trans);
      } else {
        TransitionOperator trans(
            delay, "", [] {}, prev_trans,
            states[trans_level + 1].getExpression());
        prev_trans = std::make_shared<TransitionOperator>(trans);
      }
      trans_level++;
    } else if (type == 2) {
      std::cerr << "[DPI] " << i << " event delay=" << delay << std::endl;
      // state.addEvent("Event_" + std::to_string(event_count), [] {});
      TimingState new_state =
          TimingState::createFromEvent("Event_" + std::to_string(event_count));
      new_state.build();
      states.push_back(new_state);
      event_count++;
      rep_level = 0;
    }
  }

  std::cerr << "[DPI] total states: " << states.size() << std::endl;
  TimingState final_state;
  if (trans_level > 0) {
    final_state = TimingState(prev_trans);
  } else {
    final_state = TimingState(states[states.size() - 1].getExpression());
  }
  final_state.build();
  std::cerr << "[DPI] final state built: " << final_state.toString()
            << std::endl;

  std::cerr << "[DPI] final state built. Generating pattern..." << std::endl;

  int address = -1;
  int limit = 1;
  uint64_t max_cycles = final_state.getLastScheduledCycle() + 1;
  std::cerr << "[DPI] state built: max_cycles=" << max_cycles << std::endl;

  for (uint64_t cycle = 0; cycle <= max_cycles; cycle++) {
    auto events = final_state.getEventsForCycle(cycle);
    if (!events.empty()) {
      std::cerr << "[DPI] cycle " << cycle
                << ": events found: " << events.size() << std::endl;
      if (address == -1)
        address = 0;
      else
        address = (address == limit - 1) ? 0 : address + 1;
      for (const auto &event : events) {
        limit = event_max_iter[event->getName()];
        std::cerr << "[DPI]   event: " << event->getName() << " limit=" << limit
                  << std::endl;
      }
      std::cerr << "[DPI] cycle " << cycle << ": address " << address
                << std::endl;
      expected_addresses.push_back(address);
      expected_cycles.push_back(static_cast<int>(cycle));
    }
  }
  std::cerr << "[DPI] pattern generated: " << expected_addresses.size()
            << " addresses" << std::endl;
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

} // extern "C"
