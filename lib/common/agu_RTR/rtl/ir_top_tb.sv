`timescale 1ns / 1ps

module ir_top_tb
  import agu_RTR_pkg::*;
();

  import "DPI-C" context function void cpp_build_pattern(
    input int type_configs [],
    input int iter_configs [],
    input int delay_configs[],
    input int num_levels
  );
  import "DPI-C" context function int cpp_pop_expected_address();
  import "DPI-C" context function int cpp_pop_expected_cycle();
  import "DPI-C" context function int cpp_get_address_queue_size();
  import "DPI-C" context function int cpp_get_cycle_queue_size();

  // Parameters
  localparam int ADDRESS_WIDTH = 16;
  localparam int NUMBER_IR = 4;
  localparam int CLK_PERIOD = 10;  // 10ns = 100MHz

  // DUT signals
  logic                            clk;
  logic                            rst_n;
  logic                            enable;
  rep_config_t [    NUMBER_IR-1:0] rep_configs;
  logic        [ADDRESS_WIDTH-1:0] ir_addr;
  logic                            ir_valid;
  logic                            ir_done;
  int                              current_cycle;

  // Testbench variables
  int                              test_num = 0;
  int                              error_count = 0;
  int                              addr_count = 0;
  int                              expected_total_addrs;
  logic        [ADDRESS_WIDTH-1:0] addr_queue           [$];
  int                              cycle_queue          [$];
  logic        [ADDRESS_WIDTH-1:0] expected_addr;

  // ===== DUT Instantiation =====
  ir_top #(
      .ADDRESS_WIDTH(ADDRESS_WIDTH),
      .NUMBER_IR(NUMBER_IR),
      .DELAY_WIDTH  (REP_DELAY_WIDTH),
      .ITER_WIDTH   (REP_ITER_WIDTH),
      .STEP_WIDTH   (REP_STEP_WIDTH)
  ) dut (
      .clk(clk),
      .rst_n(rst_n),
      .enable(enable),
      .rep_configs(rep_configs),
      .ir_addr(ir_addr),
      .ir_valid(ir_valid),
      .ir_done(ir_done)
  );

  // ===== Clock Generation =====
  initial begin
    clk = 0;
    forever #(CLK_PERIOD / 2) clk = ~clk;
  end

  // ===== Helper Tasks =====

  // Build expected addresses for nested loops
  // ONLY innermost (IR[0]) addresses are output
  // Outer loops control how many TIMES the inner loop repeats
  task automatic build_expected_innermost_addresses();

    int type_arr[];
    int iter_arr[];
    int delay_arr[];
    int val;
    int cycle;
    int stack_counter = 0;

    type_arr = new[NUMBER_IR + 1];
    iter_arr = new[NUMBER_IR + 1];
    delay_arr = new[NUMBER_IR + 1];

    type_arr[0] = 2;  // Event
    iter_arr[0] = 0;
    delay_arr[0] = 0;
    stack_counter++;
    for (int i = 0; i < NUMBER_IR; i++) begin
      if (!rep_configs[i].is_configured) continue;
      // Default to 1 iteration if 0 (disabled), so the math works
      type_arr[stack_counter]  = 0;  // Not used in this testbench (always repeat)
      iter_arr[stack_counter]  = (rep_configs[i].iter == 0) ? 0 : rep_configs[i].iter;
      delay_arr[stack_counter] = rep_configs[i].delay;
      stack_counter++;
    end

    // Clear SV Queue
    addr_queue.delete();

    // Call C++ Timing Model
    // We assume the C++ model adds loops in order 0..N
    cpp_build_pattern(type_arr, iter_arr, delay_arr, stack_counter);
    stack_counter = 0;

    // Retrieve results back into SV Queue
    expected_total_addrs = cpp_get_address_queue_size();

    for (int k = 0; k < expected_total_addrs; k++) begin
      val   = cpp_pop_expected_address();
      cycle = cpp_pop_expected_cycle();
      addr_queue.push_back(val);
      cycle_queue.push_back(cycle);
    end
  endtask

  // Configure a specific IR level
  task automatic configure_ir(input int level, input int delay, input int iter, input int step);
    rep_configs[level].delay = delay;
    rep_configs[level].iter = iter;
    rep_configs[level].step = step;
    rep_configs[level].is_configured = 1;
  endtask

  // Initialize all IR levels to disabled
  task automatic init_configs();
    for (int i = 0; i < NUMBER_IR; i++) begin
      rep_configs[i].delay = 0;
      rep_configs[i].iter = 0;
      rep_configs[i].step = 0;
      rep_configs[i].is_configured = 0;
    end
  endtask

  // Wait for generation to complete
  task automatic wait_for_completion(input int timeout_cycles = 20000);
    int cycle_count = 0;

    while (!ir_done && cycle_count < timeout_cycles) begin
      @(posedge clk);
      cycle_count++;
    end

    if (cycle_count >= timeout_cycles) begin
      $display("  [ERROR] TIMEOUT: ir_done not asserted within %0d cycles", timeout_cycles);
      error_count++;
    end
  endtask

  // Check generated addresses
  task automatic check_address(input logic [ADDRESS_WIDTH-1:0] addr, input logic valid);
    if (valid) begin
      if (addr_queue.size() == 0) begin
        $display("  [ERROR] EXTRA ADDRESS: Got addr=%0d but no more expected", addr);
        error_count++;
      end else begin
        expected_addr = addr_queue.pop_front();
        if (addr !== expected_addr) begin
          $display("  [ERROR] ADDRESS MISMATCH at position %0d: Expected=%0d, Got=%0d", addr_count,
                   expected_addr, addr);
          error_count++;
        end else begin
          if (addr_count < 10 || addr_count >= expected_total_addrs - 5) begin
            $display("    Address[%0d]: %0d (PASS)", addr_count, addr);
          end else if (addr_count == 10) begin
            $display("    ... (addresses 10-%0d omitted for brevity)", expected_total_addrs - 6);
          end
        end
        addr_count++;
      end
    end
  endtask

  task automatic check_cycle(input int cycle, input logic valid);
    if (valid) begin
      int expected_cycle;
      expected_cycle = cycle_queue.pop_front();
      if (cycle !== expected_cycle) begin
        $display("  [ERROR] CYCLE MISMATCH at position %0d: Expected=%0d, Got=%0d", addr_count,
                 expected_cycle, cycle);
        error_count++;
      end else begin
        if (addr_count < 10 || addr_count >= expected_total_addrs - 5) begin
          $display("      Cycle[%0d]: %0d (PASS)", addr_count, cycle);
        end
      end
    end
  endtask

  // Print test header
  task automatic print_test_header(input string test_name);
    int outer_repetitions;
    test_num++;
    addr_count = 0;

    // Calculate outer loop repetitions
    outer_repetitions = 1;
    for (int i = 1; i < NUMBER_IR; i++) begin
      if (rep_configs[i].iter > 0) begin
        outer_repetitions *= rep_configs[i].iter;
      end
    end

    $display("");
    $display("========================================");
    $display("TEST %0d: %s", test_num, test_name);
    $display("========================================");
    for (int i = 0; i < NUMBER_IR; i++) begin
      $display("  IR[%0d]: delay=%0d, iter=%0d, step=%0d", i, rep_configs[i].delay,
               rep_configs[i].iter, rep_configs[i].step);
    end
    $display("  IR[0] will repeat: %0d times", outer_repetitions);
    $display("  Expected total addresses: %0d", expected_total_addrs);
    $display("  Expected pattern: IR[0] sequence repeated %0d times", outer_repetitions);
    $display("----------------------------------------");
  endtask

  // Verify final state
  task automatic verify_completion();
    if (addr_queue.size() != 0) begin
      $display("  [ERROR] MISSING ADDRESSES: %0d addresses not generated", addr_queue.size());
      error_count++;
    end else begin
      $display("  [PASS] All %0d addresses generated correctly", addr_count);
    end

    if (!ir_done) begin
      $display("  [ERROR] ir_done not asserted at completion");
      error_count++;
    end
  endtask

  // ===== Monitor Process =====
  always @(posedge clk) begin
    if (rst_n && enable) begin
      check_address(ir_addr, ir_valid);
      check_cycle(current_cycle, ir_valid);
      current_cycle++;
    end
  end

  // ===== Main Test Sequence =====
  initial begin
    // Initialize
    rst_n  = 0;
    enable = 0;
    init_configs();

    $display("");
    $display("+========================================+");
    $display("|   IR_TOP TESTBENCH                     |");
    $display("|   (Innermost Loop Output Only)         |");
    $display("+========================================+");

    // Reset
    rst_n = 0;
    @(posedge clk);
    rst_n = 1;
    repeat (2) @(posedge clk);

    // ========================================
    // TEST 1: Single IR level (no nesting)
    // ========================================
    init_configs();
    configure_ir(0, 0, 5, 1);  // IR[0]: 5 iterations, step=1
    build_expected_innermost_addresses();
    print_test_header("Single IR Level - No Nesting");
    $display("  Expected: 0,1,2,3,4");

    enable = 1;
    current_cycle = 0;
    wait_for_completion(200);
    enable = 0;
    verify_completion();
    rst_n = 0;
    @(posedge clk);
    rst_n = 1;
    repeat (2) @(posedge clk);

    // ========================================
    // TEST 2: Two-level nesting
    // ========================================
    init_configs();
    configure_ir(0, 0, 3, 1);  // IR[0]: 3 iterations, step=1
    configure_ir(1, 0, 4, 10);  // IR[1]: 4 iterations (outer loop)
    build_expected_innermost_addresses();
    print_test_header("Two-Level Nesting");
    $display("  Expected: IR[0] sequence (0,1,2) repeated 4 times");
    $display("  Expected: 0,1,2, 0,1,2, 0,1,2, 0,1,2");

    enable = 1;
    current_cycle = 0;
    wait_for_completion(500);
    enable = 0;
    verify_completion();
    rst_n = 0;
    @(posedge clk);
    rst_n = 1;
    repeat (2) @(posedge clk);

    // ========================================
    // TEST 3: Three-level nesting
    // ========================================
    init_configs();
    configure_ir(0, 0, 2, 1);  // IR[0]: 2 iterations (innermost)
    configure_ir(1, 0, 2, 10);  // IR[1]: 3 iterations
    configure_ir(2, 0, 2, 100);  // IR[2]: 2 iterations (outermost)
    build_expected_innermost_addresses();
    print_test_header("Three-Level Nesting");
    $display("  Expected: IR[0] sequence (0,1) repeated 3*2=6 times");
    $display("  Expected: 0,1, 0,1, 0,1, 0,1, 0,1, 0,1");

    enable = 1;
    current_cycle = 0;
    wait_for_completion(500);
    enable = 0;
    verify_completion();
    rst_n = 0;
    @(posedge clk);
    rst_n = 1;
    repeat (2) @(posedge clk);

    // ========================================
    // TEST 4: Four-level nesting (full)
    // ========================================
    init_configs();
    configure_ir(0, 0, 2, 1);  // IR[0]: innermost, 2 iterations
    configure_ir(1, 0, 2, 10);  // IR[1]: 2 iterations
    configure_ir(2, 0, 2, 100);  // IR[2]: 2 iterations
    configure_ir(3, 0, 2, 1000);  // IR[3]: outermost, 2 iterations
    build_expected_innermost_addresses();
    print_test_header("Four-Level Nesting");
    $display("  Expected: IR[0] sequence (0,1) repeated 2*2*2=8 times");
    $display("  Expected: 0,1, 0,1, 0,1, 0,1, 0,1, 0,1, 0,1, 0,1");

    enable = 1;
    current_cycle = 0;
    wait_for_completion(1000);
    enable = 0;
    verify_completion();
    rst_n = 0;
    @(posedge clk);
    rst_n = 1;
    repeat (2) @(posedge clk);

    // ========================================
    // TEST 5: With delays at each level
    // ========================================
    init_configs();
    configure_ir(0, 2, 3, 1);  // IR[0]: delay=2
    configure_ir(1, 1, 2, 10);  // IR[1]: delay=1
    build_expected_innermost_addresses();
    print_test_header("Two-Level with Delays");
    $display("  Expected: (0,1,2) repeated 2 times with delays");

    enable = 1;
    current_cycle = 0;
    wait_for_completion(500);
    enable = 0;
    verify_completion();
    rst_n = 0;
    @(posedge clk);
    rst_n = 1;
    repeat (2) @(posedge clk);

    // ========================================
    // TEST 6: Non-power-of-2 iterations
    // ========================================
    init_configs();
    configure_ir(0, 0, 7, 1);  // IR[0]: 7 iterations
    configure_ir(1, 0, 5, 20);  // IR[1]: 5 iterations
    build_expected_innermost_addresses();
    print_test_header("Non-power-of-2 Iterations");
    $display("  Expected: (0,1,2,3,4,5,6) repeated 5 times = 35 addresses");

    enable = 1;
    current_cycle = 0;
    wait_for_completion(1000);
    enable = 0;
    verify_completion();
    rst_n = 0;
    @(posedge clk);
    rst_n = 1;
    repeat (2) @(posedge clk);

    // ========================================
    // TEST 7: Large step values (innermost)
    // ========================================
    init_configs();
    configure_ir(0, 0, 3, 100);  // IR[0]: large step
    configure_ir(1, 0, 2, 1000);  // IR[1]: outer loop
    build_expected_innermost_addresses();
    print_test_header("Large Step Values");
    $display("  Expected: (0,100,200) repeated 2 times");

    enable = 1;
    current_cycle = 0;
    wait_for_completion(500);
    enable = 0;
    verify_completion();
    rst_n = 0;
    @(posedge clk);
    rst_n = 1;
    repeat (2) @(posedge clk);

    // ========================================
    // TEST 8: Maximum nesting with delays
    // ========================================
    init_configs();
    configure_ir(0, 3, 3, 1);  // 3 iterations, delay=3
    configure_ir(1, 2, 2, 10);  // 2 iterations, delay=2
    configure_ir(2, 1, 2, 100);  // 2 iterations, delay=1
    configure_ir(3, 1, 2, 1000);  // 2 iterations, delay=1
    build_expected_innermost_addresses();
    print_test_header("Full Nesting with All Delays");
    $display("  Expected: (0,1,2) repeated 2*2*2=8 times = 24 addresses");

    enable = 1;
    current_cycle = 0;
    wait_for_completion(3000);
    enable = 0;
    verify_completion();
    rst_n = 0;
    @(posedge clk);
    rst_n = 1;
    repeat (2) @(posedge clk);

    // ========================================
    // TEST 9: Verify repetition pattern
    // ========================================
    init_configs();
    configure_ir(0, 0, 4, 2);  // IR[0]: 0,2,4,6
    configure_ir(1, 0, 3, 0);  // IR[1]: repeat 3 times
    build_expected_innermost_addresses();
    print_test_header("Verify Repetition Pattern");
    $display("  Expected: (0,2,4,6) repeated exactly 3 times");
    $display("  Expected: 0,2,4,6, 0,2,4,6, 0,2,4,6");

    enable = 1;
    current_cycle = 0;
    wait_for_completion(500);
    enable = 0;
    verify_completion();
    rst_n = 0;
    @(posedge clk);
    rst_n = 1;
    repeat (2) @(posedge clk);

    // ========================================
    // TEST 10: Zero step (repeated address)
    // ========================================
    init_configs();
    configure_ir(0, 0, 4, 0);  // IR[0]: zero step (0,0,0,0)
    configure_ir(1, 0, 2, 10);  // IR[1]: repeat 2 times
    build_expected_innermost_addresses();
    print_test_header("Zero Step at Inner Level");
    $display("  Expected: (0,0,0,0) repeated 2 times");

    enable = 1;
    current_cycle = 0;
    wait_for_completion(500);
    enable = 0;
    verify_completion();
    rst_n = 0;
    @(posedge clk);
    rst_n = 1;
    repeat (2) @(posedge clk);

    // ========================================
    // TEST 11: Stress test - many iterations
    // ========================================
    init_configs();
    configure_ir(0, 0, 10, 1);  // IR[0]: 0-9
    configure_ir(1, 0, 10, 20);  // IR[1]: repeat 10 times
    build_expected_innermost_addresses();
    print_test_header("Stress Test - 100 addresses");
    $display("  Expected: (0,1,2,...,9) repeated 10 times = 100 addresses");

    enable = 1;
    current_cycle = 0;
    wait_for_completion(2000);
    enable = 0;
    verify_completion();
    rst_n = 0;
    @(posedge clk);
    rst_n = 1;
    repeat (2) @(posedge clk);

    // ========================================
    // TEST 12: Different step on inner loop
    // ========================================
    init_configs();
    configure_ir(0, 0, 5, 3);  // IR[0]: 0,3,6,9,12
    configure_ir(1, 0, 2, 0);  // IR[1]: repeat 2 times
    build_expected_innermost_addresses();
    print_test_header("Non-unit Step Pattern");
    $display("  Expected: (0,3,6,9,12) repeated 2 times");

    enable = 1;
    current_cycle = 0;
    wait_for_completion(500);
    enable = 0;
    verify_completion();
    rst_n = 0;
    @(posedge clk);
    rst_n = 1;
    repeat (2) @(posedge clk);

    // ========================================
    // Final Report
    // ========================================
    repeat (10) @(posedge clk);

    $display("");
    $display("+========================================+");
    $display("|   TEST SUMMARY                         |");
    $display("+========================================+");
    $display("|   Total Tests: %3d                     |", test_num);
    if (error_count == 0) begin
      $display("|   Status: ALL PASSED                   |");
    end else begin
      $display("|   Status: %3d ERRORS                   |", error_count);
    end
    $display("+========================================+");
    $display("");

    if (error_count == 0) begin
      $display("SUCCESS: All tests passed!");
    end else begin
      $display("FAILURE: %0d error(s) found", error_count);
    end

    $finish;
  end

  // ===== Timeout Watchdog =====
  initial begin
    #1000000;  // 1ms timeout
    $display("[ERROR] GLOBAL TIMEOUT: Simulation exceeded 1ms");
    $finish;
  end

  // ===== Waveform Dump =====
  initial begin
    $dumpfile("ir_top_tb.vcd");
    $dumpvars(0, ir_top_tb);
  end

  // ===== Performance Metrics =====
  real start_time, end_time, elapsed_time;
  int total_cycles;

  always @(posedge enable) begin
    start_time   = $realtime;
    total_cycles = 0;
  end

  always @(posedge clk) begin
    if (enable) total_cycles++;
  end

  always @(posedge ir_done) begin
    end_time = $realtime;
    elapsed_time = end_time - start_time;
    //if (expected_total_addrs > 0) begin
    //  $display("  Performance: %0d addresses in %0d cycles (%.2f cycles/addr)",
    //           expected_total_addrs, total_cycles,
    //           real'(total_cycles) / real'(expected_total_addrs));
    //end
  end

endmodule
