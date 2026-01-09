`timescale 1ns / 1ps

module mt_ir_tb
  import agu_RTR_pkg::*;
();

  // --------------------------------------------------------------------------
  // DPI Imports (Reusing your existing C++ model)
  // --------------------------------------------------------------------------
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

  // --------------------------------------------------------------------------
  // Parameters & Signals
  // --------------------------------------------------------------------------
  localparam int ADDRESS_WIDTH = 16;
  localparam int NUMBER_IR = 4;
  localparam int NUMBER_MT = 3;
  localparam int CLK_PERIOD = 10;
  localparam int REP_DELAY_WIDTH = 6;
  localparam int REP_ITER_WIDTH = 6;
  localparam int TRANS_DELAY_WIDTH = 12;

  typedef agu_config_class#(
      .NUMBER_IR        (NUMBER_IR),
      .NUMBER_MT        (1),                 // Single MT for this testbench
      .NUMBER_OR        (1),                 // Single OR for this testbench
      .REP_DELAY_WIDTH  (REP_DELAY_WIDTH),
      .REP_ITER_WIDTH   (REP_ITER_WIDTH),
      .TRANS_DELAY_WIDTH(TRANS_DELAY_WIDTH)
  )::rep_config_t rep_config_t;

  typedef agu_config_class#(
      .NUMBER_IR        (NUMBER_IR),
      .NUMBER_MT        (1),                 // Single MT for this testbench
      .NUMBER_OR        (1),                 // Single OR for this testbench
      .REP_DELAY_WIDTH  (REP_DELAY_WIDTH),
      .REP_ITER_WIDTH   (REP_ITER_WIDTH),
      .TRANS_DELAY_WIDTH(TRANS_DELAY_WIDTH)
  )::trans_config_t trans_config_t;

  logic                            clk;
  logic                            rst_n;
  logic                            enable;

  // Configuration Signals
  trans_config_t [    NUMBER_MT-1:0] mt_configs;
  rep_config_t   [NUMBER_MT:0][NUMBER_IR-1:0] ir_configs;

  // Outputs
  logic          [ADDRESS_WIDTH-1:0] ir_addr;
  logic                              ir_valid;
  logic                              ir_done;

  // Testbench State
  int current_cycle;
  int test_num = 0;
  int error_count = 0;
  int addr_count = 0;
  int expected_total_addrs;
  int current_queue_size;

  // Validation Queues
  logic [ADDRESS_WIDTH-1:0] addr_queue [$];
  int                       cycle_queue[$];

  // --------------------------------------------------------------------------
  // DUT Instantiation
  // --------------------------------------------------------------------------
  mt_ir #(
      .ADDRESS_WIDTH(ADDRESS_WIDTH),
      .NUMBER_IR    (NUMBER_IR),
      .NUMBER_MT    (NUMBER_MT),
      .REP_DELAY_WIDTH(REP_DELAY_WIDTH),
      .REP_ITER_WIDTH (REP_ITER_WIDTH),
      .TRANS_DELAY_WIDTH(TRANS_DELAY_WIDTH)
  ) dut (
      .clk        (clk),
      .rst_n      (rst_n),
      .enable     (enable),
      .mt_configs (mt_configs),
      .ir_configs(ir_configs), // SystemVerilog handles unpacked array mapping here
      .ir_addr    (ir_addr),
      .ir_valid   (ir_valid),
      .ir_done    (ir_done)
  );

  // --------------------------------------------------------------------------
  // Clock Generation
  // --------------------------------------------------------------------------
  initial begin
    clk = 0;
    forever #(CLK_PERIOD / 2) clk = ~clk;
  end

  // --------------------------------------------------------------------------
  // Helper Tasks
  // --------------------------------------------------------------------------

  // Initialize all configurations to 0
  task automatic init_configs();
    for (int m = 0; m <= NUMBER_MT; m++) begin
      for (int i = 0; i < NUMBER_IR; i++) begin
        ir_configs[m][i] = '0; // Clear struct
      end
    end
    for (int t = 0; t < NUMBER_MT; t++) begin
      mt_configs[t] = '0;
    end
  endtask

  // Configure a specific Lane's IR settings
  task automatic configure_lane_ir(
    input int lane,
    input int level,
    input int delay,
    input int iter,
    input int step
  );
    ir_configs[lane][level].delay = delay;
    ir_configs[lane][level].iter = iter;
    ir_configs[lane][level].is_configured = 1;
  endtask

  // Configure transition delay (Delay AFTER lane 'trans_idx' finishes)
  task automatic configure_mt_trans(input int trans_idx, input int delay);
    mt_configs[trans_idx].delay = delay;
    mt_configs[trans_idx].is_configured = 1;
  endtask

  // --------------------------------------------------------------------------
  // Build Expected Sequence (Multi-Lane Logic)
  // --------------------------------------------------------------------------
  // This iterates through enabled lanes, calls the C++ model for each,
  // and stitches the results together by offsetting the expected cycles.
  task automatic build_expected_full_sequence();
    int global_cycle_offset = 0;
    int max_lane_idx = 0;
    int event_count = 0;
    int mt_config_count = 0;
    int stack_counter = 0;
    int lane_addr_count;
    int max_cyc_in_lane;
    int type_arr[];
    int iter_arr[];
    int delay_arr[];

    addr_queue.delete();
    cycle_queue.delete();

    type_arr  = new[NUMBER_IR*NUMBER_MT + NUMBER_MT + 1];
    iter_arr  = new[NUMBER_IR*NUMBER_MT + NUMBER_MT + 1];
    delay_arr = new[NUMBER_IR*NUMBER_MT + NUMBER_MT + 1];

    $display("  Building expected sequence from configurations...");
    max_lane_idx = 0;
    for (int k = 0; k <= NUMBER_MT; k++) begin
      if (ir_configs[k][0].is_configured) begin
        max_lane_idx = k;
      end
      if (k < NUMBER_MT && mt_configs[k].is_configured) begin
        mt_config_count++;
      end
    end
    for (int m = 0; m <= max_lane_idx; m++) begin
      type_arr[stack_counter]  = 2; // Event
      iter_arr[stack_counter]  = 0;
      delay_arr[stack_counter] = 0;
      stack_counter++;
      event_count++;
      for (int i = 0; i < NUMBER_IR; i++) begin
        if (!ir_configs[m][i].is_configured) continue;
        type_arr[stack_counter]  = 0; // Repetition
        iter_arr[stack_counter] = ir_configs[m][i].iter;
        delay_arr[stack_counter] = ir_configs[m][i].delay;
        stack_counter++;
      end
    end
    for (int t = 0; t < mt_config_count; t++) begin
      if (event_count < t + 2) begin
        if (event_count == t + 1) begin
          $display("  [WARNING] MT Transition %0d configured, adding event", t);
          type_arr[stack_counter]  = 2; // Event
          iter_arr[stack_counter]  = 0;
          delay_arr[stack_counter] = 0;
          stack_counter++;
          event_count++;
        end else begin
          $display("  [ERROR] MT Transition %0d configured but no subsequent lane enabled.", t);
          error_count++;
        end
      end
      if (mt_configs[t].is_configured) begin
        type_arr[stack_counter]  = 1; // Transition
        iter_arr[stack_counter]  = 0;
        delay_arr[stack_counter] = mt_configs[t].delay;
        stack_counter++;
      end
    end

    // Call C++ Model for this lane
    cpp_build_pattern(type_arr, iter_arr, delay_arr, stack_counter);
    stack_counter = 0;

    // Extract results
    lane_addr_count = cpp_get_address_queue_size();
    max_cyc_in_lane = 0; // Track the duration of this lane

    for (int k = 0; k < lane_addr_count; k++) begin
      int val = cpp_pop_expected_address();
      int cyc = cpp_pop_expected_cycle();

      addr_queue.push_back(val);
      // The cycle is relative to the start of THIS lane.
      // Add the global offset to make it relative to the 'enable' of the whole MT unit.
      cycle_queue.push_back(cyc + global_cycle_offset);

      if (cyc > max_cyc_in_lane) max_cyc_in_lane = cyc;
    end

    expected_total_addrs = addr_queue.size();
    $display("    Total Expected Addresses: %0d", expected_total_addrs);
    for (int c=0; c < expected_total_addrs; c++) begin
      $display("cycle %d addr=%0d", cycle_queue[c], addr_queue[c]);
    end
  endtask

  // --------------------------------------------------------------------------
  // Verification Tasks
  // --------------------------------------------------------------------------
  task automatic wait_for_completion(input int timeout_cycles = 50);
    int cnt = 0;
    while (!ir_done && cnt < timeout_cycles) begin
      @(posedge clk);
      cnt++;
    end
    if (cnt >= timeout_cycles) begin
      $display("  [ERROR] TIMEOUT: ir_done not asserted within %0d cycles", timeout_cycles);
      error_count++;
    end
  endtask

  task automatic check_outputs();
    if (ir_valid) begin
      if (addr_queue.size() == 0) begin
        $display("  [ERROR] EXTRA ADDRESS: Got addr=%0d at cycle %0d but none expected", ir_addr, current_cycle);
        error_count++;
      end else begin
        logic [ADDRESS_WIDTH-1:0] exp_addr = addr_queue.pop_front();
        int exp_cyc = cycle_queue.pop_front();

        // Check Address
        if (ir_addr !== exp_addr) begin
          $display("  [ERROR] ADDRESS MISMATCH at idx %0d: Exp=%0d, Got=%0d", addr_count, exp_addr, ir_addr);
          error_count++;
        end

        // Check Cycle (allowing +/- 1 slip if simulation/model boundary conditions vary slightly)
        // Strict check:
        else if (current_cycle !== exp_cyc) begin
           $display("  [ERROR] CYCLE MISMATCH at idx %0d: Exp=%0d, Got=%0d (Addr=%0d)", addr_count, exp_cyc, current_cycle, ir_addr);
           error_count++;
        end
        else begin
           if (addr_count < 10) $display("    match: addr=%0d, cyc=%0d", ir_addr, current_cycle);
        end
        addr_count++;
      end
    end
  endtask

  task automatic verify_final();
    if (addr_queue.size() > 0) begin
      $display("  [ERROR] MISSING DATA: %0d addresses were expected but not received.", addr_queue.size());
      error_count++;
    end
    if (!ir_done) begin
      $display("  [ERROR] ir_done was not asserted at end of test.");
      error_count++;
    end
  endtask

  // --------------------------------------------------------------------------
  // Main Monitor
  // --------------------------------------------------------------------------
  always @(posedge clk) begin
    if (rst_n && enable) begin
      check_outputs();
      current_cycle++;
    end
  end

  // --------------------------------------------------------------------------
  // Test Sequence
  // --------------------------------------------------------------------------
  initial begin
    // Setup
    rst_n = 0;
    enable = 0;
    init_configs();

    $display("");
    $display("+========================================+");
    $display("|   MT_IR TESTBENCH                      |");
    $display("|   (Multi-Lane Sequence Verification)   |");
    $display("+========================================+");

    // Reset
    @(posedge clk); rst_n = 1;

    // ============================================================
    // TEST 1: Single Lane (Lane 0 only)
    // ============================================================
    test_num++;
    $display("\nTEST %0d: Single Lane (Legacy Mode)", test_num);
    init_configs();
    // Lane 0: 5 iterations
    configure_lane_ir(0, 0, 0, 5, 1);

    build_expected_full_sequence();

    current_cycle = 0;
    addr_count = 0;
    enable = 1;
    wait_for_completion();
    enable = 0;
    verify_final();

    rst_n = 0; @(posedge clk); rst_n = 1;

    // ============================================================
    // TEST 2: Two Lanes, No Delay
    // Lane 0 -> Immediate -> Lane 1
    // ============================================================
    test_num++;
    $display("\nTEST %0d: Two Lanes, No Delay", test_num);
    init_configs();
    // Lane 0: 0, 1, 2
    configure_lane_ir(0, 0, 0, 3, 1);
    // Lane 1: 10, 11, 12, 13
    configure_lane_ir(1, 0, 0, 4, 1);

    // Config transition: Lane 0->1 delay = 0
    configure_mt_trans(0, 0);

    build_expected_full_sequence();

    current_cycle = 0;
    addr_count = 0;
    enable = 1;
    wait_for_completion();
    enable = 0;
    verify_final();

    rst_n = 0; @(posedge clk); rst_n = 1;

    // ============================================================
    // TEST 3: Two Lanes, With 5 Cycle Delay
    // Lane 0 -> Wait 5 -> Lane 1
    // ============================================================
    test_num++;
    $display("\nTEST %0d: Two Lanes, 5 Cycle Delay", test_num);
    init_configs();
    // Lane 0: 0, 1
    configure_lane_ir(0, 0, 0, 2, 1);
    // Lane 1: 0, 1, 2
    configure_lane_ir(1, 0, 0, 3, 1);

    // Config transition: Lane 0->1 delay = 5
    configure_mt_trans(0, 5);

    build_expected_full_sequence();

    current_cycle = 0;
    addr_count = 0;
    enable = 1;
    wait_for_completion();
    enable = 0;
    verify_final();

    rst_n = 0; @(posedge clk); rst_n = 1;

    // ============================================================
    // TEST 4: Three Lanes (Full Chain) with Mixed Delays
    // Lane 0 (Delay 2) -> Lane 1 (Delay 10) -> Lane 2
    // ============================================================
    test_num++;
    $display("\nTEST %0d: Three Lanes Mixed Delays", test_num);
    init_configs();

    // Lane 0: Simple 0..3
    configure_lane_ir(0, 0, 0, 4, 1);

    // Lane 1: Nested Loop
    configure_lane_ir(1, 0, 0, 2, 1); // inner
    configure_lane_ir(1, 1, 0, 2, 10); // outer

    // Lane 2: Simple 0..2
    configure_lane_ir(2, 0, 0, 3, 1);

    // Transitions
    configure_mt_trans(0, 2);  // 0->1 wait 2
    configure_mt_trans(1, 10); // 1->2 wait 10

    build_expected_full_sequence();

    current_cycle = 0;
    addr_count = 0;
    enable = 1;
    wait_for_completion();
    enable = 0;
    verify_final();

    rst_n = 0; @(posedge clk); rst_n = 1;

    // ============================================================
    // TEST 5: Two Lanes, No Repetitions, Just Transitions
    // ============================================================
    test_num++;
    $display("\nTEST %0d: Two Lanes, No Repetitions", test_num);
    init_configs();

    // Transitions
    configure_mt_trans(0, 1);  // 0->1 wait 1

    build_expected_full_sequence();

    current_cycle = 0;
    addr_count = 0;
    enable = 1;
    wait_for_completion();
    enable = 0;
    verify_final();

    rst_n = 0; @(posedge clk); rst_n = 1;

    // ============================================================
    // TEST 6: Three Lanes, No Repetitions, Just Transitions
    // ============================================================
    test_num++;
    $display("\nTEST %0d: Three Lanes, No Repetitions", test_num);
    init_configs();

    // Transitions
    configure_mt_trans(0, 5);  // 0->1 wait 5
    configure_mt_trans(1, 3);  // 1->2 wait 3

    build_expected_full_sequence();

    current_cycle = 0;
    addr_count = 0;
    enable = 1;
    wait_for_completion();
    enable = 0;
    verify_final();

    rst_n = 0; @(posedge clk); rst_n = 1;

    // ============================================================
    // TEST 7: Four Lanes, No Repetitions, Just Transitions
    // ============================================================
    test_num++;
    $display("\nTEST %0d: Four Lanes, No Repetitions", test_num);
    init_configs();

    // Transitions
    configure_mt_trans(0, 5);  // 0->1 wait 5
    configure_mt_trans(1, 3);  // 1->2 wait 3
    configure_mt_trans(2, 20);  // 1->2 wait 3

    build_expected_full_sequence();

    current_cycle = 0;
    addr_count = 0;
    enable = 1;
    wait_for_completion();
    enable = 0;
    verify_final();

    rst_n = 0; @(posedge clk); rst_n = 1;

    // ============================================================
    // TEST 8: Two lanes, One Repetition Each, No Delays, keep enable high
    // ============================================================
    test_num++;
    $display("\nTEST %0d: Two Lanes, One Repetition Each, No Delays, keep enable high", test_num);
    init_configs();

    configure_lane_ir(0, 0, 0, 2, 1); // Lane 0: 2 iterations
    configure_lane_ir(1, 0, 0, 3, 1); // Lane 1: 3 iterations
    configure_mt_trans(0, 0);  // 0->1 wait 0

    build_expected_full_sequence();
    current_queue_size = addr_queue.size();
    for (int c=0; c < current_queue_size; c++) begin
      cycle_queue.push_back(current_queue_size + c); // Append same addresses after first run
      addr_queue.push_back(addr_queue[c]);
    end

    current_cycle = 0;
    addr_count = 0;
    enable = 1;
    wait_for_completion();
    @(posedge clk);
    wait_for_completion(); // Wait again without disabling
    enable = 0;
    verify_final();

    // ============================================================
    // Final Summary
    // ============================================================
    $display("\n+========================================+");
    if (error_count == 0) $display("|   STATUS: PASSED                       |");
    else                  $display("|   STATUS: FAILED (%0d errors)           |", error_count);
    $display("+========================================+\n");

    $finish;
  end

endmodule
