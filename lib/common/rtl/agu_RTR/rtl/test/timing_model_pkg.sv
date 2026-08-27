
package timing_model_pkg;

  import "DPI-C" context function int cpp_build_pattern(
    input int type_configs [],
    input int iter_configs [],
    input int delay_configs[],
    input int step_configs []
  );
  import "DPI-C" context function int cpp_pop_expected_address();
  import "DPI-C" context function int cpp_pop_expected_cycle();
  import "DPI-C" context function int cpp_get_address_queue_size();
  import "DPI-C" context function int cpp_get_cycle_queue_size();
  import "DPI-C" context function string cpp_get_timing_expression();

  class TimingModelHelper;
    // Configuration queues
    int type_q[$];
    int iter_q[$];
    int delay_q[$];
    int step_q[$];

    // Exprected results
    int addr_queue[$];
    int cycle_queue[$];
    int addr_count;
    string expression;

    // Verification state
    int verified_count;

    function new();
      clear();
    endfunction

    function void clear();
      type_q.delete();
      iter_q.delete();
      delay_q.delete();
      step_q.delete();
      addr_queue.delete();
      cycle_queue.delete();
      addr_count = 0;
      verified_count = 0;
      expression = "";
    endfunction

    function void add_event();
      type_q.push_back(2);
      iter_q.push_back(0);
      delay_q.push_back(0);
      step_q.push_back(0);
    endfunction

    function void add_repetition(int iter, int delay, int step);
      type_q.push_back(0);
      iter_q.push_back(iter);
      delay_q.push_back(delay);
      step_q.push_back(step);
    endfunction

    function void add_transition(int delay);
      type_q.push_back(1);
      iter_q.push_back(0);
      delay_q.push_back(delay);
      step_q.push_back(0);
    endfunction

    function int build();
      int type_arr[];
      int iter_arr[];
      int delay_arr[];
      int step_arr[];

      int max_cycles = 0;

      // Convert SV Queues to C++ Arrays for DPI
      type_arr  = new[type_q.size()];
      iter_arr  = new[iter_q.size()];
      delay_arr = new[delay_q.size()];
      step_arr  = new[step_q.size()];
      foreach (type_q[i]) type_arr[i] = type_q[i];
      foreach (iter_q[i]) iter_arr[i] = iter_q[i];
      foreach (delay_q[i]) delay_arr[i] = delay_q[i];
      foreach (step_q[i]) step_arr[i] = step_q[i];

      // Clear SV queues
      addr_queue.delete();
      cycle_queue.delete();

      // Call C++ timing model
      max_cycles = cpp_build_pattern(type_arr, iter_arr, delay_arr, step_arr);

      // Retrieve results
      addr_count = cpp_get_address_queue_size();
      for (int k = 0; k < addr_count; k++) begin
        addr_queue.push_back(cpp_pop_expected_address());
        cycle_queue.push_back(cpp_pop_expected_cycle());
      end

      expression = cpp_get_timing_expression();
      verified_count = 0;

      return max_cycles;
    endfunction

    function int verify(int cycle, int address, int valid);
      int expected_cycle, expected_address;

      // If not valid, verify that current cycle is not the next cycle_queue
      // value
      if (!valid) begin
        if (cycle_queue.size() == 0) return 1;  // No more expected cycles, so valid should be 0
        expected_cycle = cycle_queue[0];
        if (cycle === expected_cycle) return 0;  // Valid should be 1 at expected cycle
        return 1;  // Valid is correctly 0
      end

      if (addr_queue.size() == 0) return -1;  // No expected addresses

      expected_cycle   = cycle_queue.pop_front();
      expected_address = addr_queue.pop_front();
      verified_count++;

      if (cycle !== expected_cycle || address !== expected_address) return 0;  // Mismatch

      return 1;  // Match
    endfunction

    function int remaining();
      return addr_queue.size();
    endfunction
  endclass

endpackage
