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
                       const svOpenArrayHandle delay_h,
                       const svOpenArrayHandle step_h, int num_levels) {
  expected_addresses.clear();

  // Your existing code for building the TimingState...
  TimingState state = TimingState();

  std::vector<TimingState> states;
  std::shared_ptr<TransitionOperator> prev_trans;
  int rep_level = 0;
  int trans_level = 0;
  int event_count = 0;
  std::cerr << "[DPI] Building states for " << num_levels << " levels"
            << std::endl;
  for (int i = 0; i < num_levels; ++i) {
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

    std::cerr << "[DPI] - LVL " << i;

    if (type == 0) {
      if (iter > 0) {
        std::cerr << " (repetition) iter=" << iter << " delay=" << delay
                  << " step=" << step << std::endl;
        TimingState &current_state = states[states.size() - 1];
        TimingState new_state = TimingState(current_state.getExpression());

        new_state.copyLevelData(current_state);
        new_state.addRepetition(iter, delay, rep_level, step);
        new_state.build();
        states[states.size() - 1] = new_state;
        rep_level++;
      }
    } else if (type == 1) {
      std::cerr << " (transition) delay=" << delay << std::endl;
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
      std::cerr << " (event) delay=" << delay << std::endl;
      // state.addEvent("Event_" + std::to_string(event_count), [] {});
      TimingState new_state =
          TimingState::createFromEvent("Event_" + std::to_string(event_count));
      // new_state.build();
      states.push_back(new_state);
      event_count++;
      rep_level = 0;
    }
  }

  std::cerr << "[DPI] total states: " << states.size() << std::endl;
  TimingState final_state;
  if (trans_level > 0 && states.size() > 1) {
    final_state = TimingState(prev_trans);
    for (const auto &state : states) {
      if (!state.getLevelsStep().empty()) {
        final_state.copyLevelData(state);
        break;
      }
    }
  } else {
    final_state = TimingState(states[states.size() - 1].getExpression());
  }
  final_state.copyLevelData(states[states.size() - 1]);
  final_state.build();
  std::cerr << "[DPI] final state built: "
            << final_state.getExpression()->toString() << std::endl;

  // Pattern generation - need to track address counters per event
  uint64_t max_cycles = final_state.getLastScheduledCycle() + 1;

  std::cerr << "[DPI] final state built. Generating pattern..." << std::endl;
  std::cerr << "[DPI] state built: max_cycles=" << max_cycles << std::endl;

  // Build a map of event names to their address generation state
  std::map<std::string, std::vector<uint64_t>> event_address_counters;
  std::map<std::string, std::vector<uint64_t>> event_total_iterations;
  std::map<std::string, std::vector<uint64_t>> event_steps;
  std::map<std::string, int>
      event_index_map; // Map event names to their index for counter mode

  // Initialize counters for each event by copying from states
  int event_idx = 0;
  for (const auto &state : states) {
    if (state.getExpression()) {
      std::string event_name = state.getExpression()->lastEventName();
      if (!event_name.empty()) {
        event_index_map[event_name] = event_idx++;

        if (!state.getLevelsStep().empty()) {
          // Event has repetitions configured
          event_address_counters[event_name] =
              std::vector<uint64_t>(state.getLevelsStep().size(), 0);
          event_total_iterations[event_name] = state.getLevelsTotalIterations();
          event_steps[event_name] = state.getLevelsStep();
          std::cerr << "[DPI] Initialized counters for event: " << event_name
                    << " with " << state.getLevelsStep().size() << " levels"
                    << std::endl;
        } else {
          // Event has no repetitions - use counter mode (will use event index
          // as address)
          std::cerr << "[DPI] Event " << event_name
                    << " has no repetitions, will use counter mode (index="
                    << event_index_map[event_name] << ")" << std::endl;
        }
      }
    }
  }

  for (uint64_t cycle = 0; cycle <= max_cycles; cycle++) {
    auto events = final_state.getEventsForCycle(cycle);
    std::cerr << "[DPI] cycle " << cycle << ": events found: " << events.size()
              << std::endl;
    if (events.empty())
      continue;

    std::string current_event_name = "";
    for (const auto &event : events) {
      current_event_name = event->getName();
      std::cerr << "[DPI]   event: " << current_event_name << std::endl;
      event->execute();
    }

    // Calculate address for this event using its own counters
    int address = 0;
    if (event_address_counters.find(current_event_name) !=
        event_address_counters.end()) {
      // Event has repetitions configured - use repetition-based addressing
      auto &counters = event_address_counters[current_event_name];
      auto &steps = event_steps[current_event_name];
      auto &iterations = event_total_iterations[current_event_name];

      // Calculate address from current counter values
      for (size_t i = 0; i < counters.size(); i++) {
        address += steps[i] * counters[i];
      }

      std::cerr << "[DPI] cycle " << cycle << ": event " << current_event_name
                << ", address " << address << " (repetition mode)" << std::endl;

      // Increment counters for this event
      if (counters.size() > 0) {
        uint64_t current_level = 0;
        do {
          counters[current_level]++;
          if (counters[current_level] >= iterations[current_level]) {
            counters[current_level] = 0;
          } else {
            break;
          }
        } while (++current_level < counters.size());
      }
    } else if (event_index_map.find(current_event_name) !=
               event_index_map.end()) {
      // Event has no repetitions - use counter mode (event index as address)
      address = event_index_map[current_event_name];
      std::cerr << "[DPI] cycle " << cycle << ": event " << current_event_name
                << ", address " << address << " (counter mode)" << std::endl;
    } else {
      std::cerr << "[DPI] cycle " << cycle << ": no mapping for event "
                << current_event_name << ", using address 0" << std::endl;
    }

    expected_addresses.push_back(address);
    expected_cycles.push_back(cycle);
  }
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

} // extern "C"
