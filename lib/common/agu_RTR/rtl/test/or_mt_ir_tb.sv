`timescale 1ns / 1ps

module or_mt_ir_tb
  import agu_RTR_pkg::*;
();

  // --------------------------------------------------------------------------
  // DPI Imports
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
  localparam int NUMBER_OR = 2;
  localparam int CLK_PERIOD = 10;
  localparam int REP_DELAY_WIDTH = 6;
  localparam int REP_ITER_WIDTH = 6;
  localparam int REP_STEP_WIDTH = 6;
  localparam int TRANS_DELAY_WIDTH = 12;

  logic clk;
  logic rst_n;
  logic enable;

  // Configurations
  typedef agu_config_class#(
      .NUMBER_IR        (NUMBER_IR),
      .NUMBER_MT        (NUMBER_MT),
      .NUMBER_OR        (NUMBER_OR),
      .REP_DELAY_WIDTH  (REP_DELAY_WIDTH),
      .REP_ITER_WIDTH   (REP_ITER_WIDTH),
      .REP_STEP_WIDTH   (REP_STEP_WIDTH),
      .TRANS_DELAY_WIDTH(TRANS_DELAY_WIDTH)
  )::agu_config_t agu_config_t;
  agu_config_t agu_config;

  // Outputs
  logic [ADDRESS_WIDTH-1:0] ir_addr;
  logic ir_valid;
  logic ir_done;

  // Testbench State
  int current_cycle;
  int test_num = 0;
  int error_count = 0;
  int addr_count = 0;

  // --------------------------------------------------------------------------
  // Data Structures for Expectation Generation
  // --------------------------------------------------------------------------

  // 1. Structure to hold ONE full pass of the MT (Lanes + Transitions) sequence
  typedef struct {
    logic [ADDRESS_WIDTH-1:0] addrs[$];
    int                       rel_cycles[$];
    int                       duration;       // Total cycles this sequence takes
  } mt_sequence_cache_t;
  mt_sequence_cache_t mt_sequence_cache;

  // 2. Final Expected Queue (Grand total)
  logic [ADDRESS_WIDTH-1:0] final_addr_queue[$];
  int final_cycle_queue[$];

  // 3. Global time tracker for recursion
  int global_sim_time;

  // --------------------------------------------------------------------------
  // DUT Instantiation
  // --------------------------------------------------------------------------
  or_mt_ir #(
      .ADDRESS_WIDTH    (ADDRESS_WIDTH),
      .NUMBER_IR        (NUMBER_IR),
      .NUMBER_MT        (NUMBER_MT),
      .NUMBER_OR        (NUMBER_OR),
      .REP_DELAY_WIDTH  (REP_DELAY_WIDTH),
      .REP_ITER_WIDTH   (REP_ITER_WIDTH),
      .REP_STEP_WIDTH   (REP_STEP_WIDTH),
      .TRANS_DELAY_WIDTH(TRANS_DELAY_WIDTH)
  ) dut (
      .clk       (clk),
      .rst_n     (rst_n),
      .enable    (enable),
      .or_configs(agu_config.or_configs),
      .mt_configs(agu_config.mt_configs),
      .ir_configs(agu_config.ir_configs),
      .ir_addr   (ir_addr),
      .ir_valid  (ir_valid),
      .ir_done   (ir_done)
  );

  // --------------------------------------------------------------------------
  // Clock
  // --------------------------------------------------------------------------
  initial begin
    clk = 0;
    forever #(CLK_PERIOD / 2) clk = ~clk;
  end

  // --------------------------------------------------------------------------
  // Helper Tasks: Configuration
  // --------------------------------------------------------------------------
  task automatic init_configs();
    for (int k = 0; k < NUMBER_OR; k++) begin
      agu_config.or_configs[k] = '0;
    end
    for (int m = 0; m <= NUMBER_MT; m++) begin
      for (int i = 0; i < NUMBER_IR; i++) begin
        agu_config.ir_configs[m][i] = '0;
      end
    end
    for (int t = 0; t < NUMBER_MT; t++) begin
      agu_config.mt_configs[t] = '0;
    end
  endtask

  task automatic configure_lane_ir(input int lane, input int level, input int delay, input int iter,
                                   input int step);
    $display("    Configuring IR: lane=%0d, level=%0d, delay=%0d, iter=%0d", lane, level, delay,
             iter);
    agu_config.ir_configs[lane][level].delay = delay;
    agu_config.ir_configs[lane][level].iter = iter;
    agu_config.ir_configs[lane][level].step = step;
    agu_config.ir_configs[lane][level].is_configured = 1;
  endtask

  task automatic configure_mt_trans(input int trans_idx, input int delay);
    agu_config.mt_configs[trans_idx].delay = delay;
    agu_config.mt_configs[trans_idx].is_configured = 1;
  endtask

  task automatic configure_or(input int level, input int iter, input int delay);
    agu_config.or_configs[level].iter  = iter;
    agu_config.or_configs[level].delay = delay;
    // Note: or_configs doesn't have explicit 'step' or 'is_configured' in previous definitions,
    // but assuming standard rep_config_t structure, we use iter/delay.
    // If is_configured logic is needed, ensure struct has it.
    // Based on logic, iter > 0 implies configured.
  endtask

  // --------------------------------------------------------------------------
  // Expectation Generation Logic
  // --------------------------------------------------------------------------

  // Step 1: Build the "Base Unit" - The sequence of one full MT run
  task automatic build_mt_sequence_struct();
    int max_lane_idx = 0;
    int stack_counter = 0;
    int type_arr[];
    int iter_arr[];
    int delay_arr[];
    int mt_offset = 0;

    // Clear cache
    mt_sequence_cache.addrs.delete();
    mt_sequence_cache.rel_cycles.delete();
    mt_sequence_cache.duration = 0;

    // Allocate max possible size
    type_arr = new[NUMBER_IR * NUMBER_MT + NUMBER_MT + 10];
    iter_arr = new[NUMBER_IR * NUMBER_MT + NUMBER_MT + 10];
    delay_arr = new[NUMBER_IR * NUMBER_MT + NUMBER_MT + 10];

    // Determine active lanes
    for (int k = 0; k <= NUMBER_MT; k++) begin
      if (agu_config.ir_configs[k][0].is_configured) max_lane_idx = k;
    end

    // Iterate through sequence (Lane -> Trans -> Lane...)
    for (int m = 0; m <= max_lane_idx; m++) begin
      int lane_duration = 0;
      int lane_addr_count;

      // 1. Build C++ Pattern for THIS lane
      stack_counter = 0;
      type_arr[stack_counter] = 2;  // Event
      iter_arr[stack_counter] = 0;
      delay_arr[stack_counter] = 0;
      stack_counter++;

      for (int i = 0; i < NUMBER_IR; i++) begin
        if (!agu_config.ir_configs[m][i].is_configured) continue;
        $display("      Adding IR Config i=%0d, m=%0d: iter=%0d, delay=%0d", i, m,
                 agu_config.ir_configs[m][i].iter, agu_config.ir_configs[m][i].delay);
        type_arr[stack_counter]  = 0;  // Repetition
        iter_arr[stack_counter]  = agu_config.ir_configs[m][i].iter;
        delay_arr[stack_counter] = agu_config.ir_configs[m][i].delay;
        stack_counter++;
      end

      cpp_build_pattern(type_arr, iter_arr, delay_arr, stack_counter);

      lane_addr_count = cpp_get_address_queue_size();

      // 2. Extract and Offset
      for (int k = 0; k < lane_addr_count; k++) begin
        int val = cpp_pop_expected_address();
        int cyc = cpp_pop_expected_cycle();
        mt_sequence_cache.addrs.push_back(val);
        mt_sequence_cache.rel_cycles.push_back(cyc + mt_offset);
        if (cyc > lane_duration) lane_duration = cyc;
      end

      // Update offset for next item in sequence
      // Lane duration is strictly the last cycle relative to start.
      // But we need to account for the +1 cycle implicit in state machine transitions if any.
      // For simplicity, we assume the previous logic: max_cyc is the last active cycle.
      // The next lane starts after this lane finishes.
      // In mt_ir, transition happens AFTER lane done.
      if (lane_addr_count > 0) begin
        // If lane generated data, duration is max_cyc + 1 (since cyc is 0-indexed)
        mt_offset += (lane_duration + 1);
      end else begin
        // Empty lane? Usually implies configuration error or 1 cycle check.
        mt_offset += 1;
      end

      // 3. Add Transition Delay if not last lane
      if (m < max_lane_idx) begin
        if (agu_config.mt_configs[m].is_configured) begin
          mt_offset += agu_config.mt_configs[m].delay;
        end
      end
    end

    mt_sequence_cache.duration = mt_offset;
  endtask

  // Step 2: Recursive Task to Simulate OR Nesting
  task automatic simulate_or_recursion(input int level);
    // Base Case: Level < 0 means we are at the leaf (inside the innermost OR)
    // Here we "execute" the MT sequence
    if (level < 0) begin
      foreach (mt_sequence_cache.addrs[i]) begin
        final_addr_queue.push_back(mt_sequence_cache.addrs[i]);
        final_cycle_queue.push_back(mt_sequence_cache.rel_cycles[i] + global_sim_time);
      end
      // Advance time by the duration of the MT sequence
      global_sim_time += mt_sequence_cache.duration;
      return;
    end

    // Recursive Step
    if (agu_config.or_configs[level].iter > 0) begin
      for (int i = 0; i < agu_config.or_configs[level].iter; i++) begin
        // Recurse deeper
        simulate_or_recursion(level - 1);

        // Add Delay between iterations (but not after the last one of this level)
        if (i < agu_config.or_configs[level].iter - 1) begin
          global_sim_time += agu_config.or_configs[level].delay;
        end
      end
    end else begin
      // If configured iter is 0, usually treat as 1 pass or disabled.
      // Current RTL treats iter=0 as effectively disabled/skipped loops or 1 pass?
      // RTL: `max_or_level` ignores iter=0. If all iter=0, runs once?
      // Let's assume standard behavior: pure passthrough if not configured.
      simulate_or_recursion(level - 1);
    end
  endtask

  // Step 3: Master Build Task
  task automatic build_grand_expected_sequence();
    final_addr_queue.delete();
    final_cycle_queue.delete();
    global_sim_time = 0;

    // 1. Pre-calculate the repeated block
    build_mt_sequence_struct();

    // 2. Run recursion
    // We start from the highest OR level
    simulate_or_recursion(NUMBER_OR - 1);

    $display("    Total Expected: %0d addresses", final_addr_queue.size());
  endtask

  // --------------------------------------------------------------------------
  // Verification Tasks
  // --------------------------------------------------------------------------
  task automatic wait_for_completion(input int timeout_cycles = 100000);
    int cnt = 0;
    while (!ir_done && cnt < timeout_cycles) begin
      @(posedge clk);
      cnt++;
    end
    if (cnt >= timeout_cycles) begin
      $display("  [ERROR] TIMEOUT: ir_done not asserted");
      error_count++;
    end
  endtask

  task automatic check_outputs();
    if (ir_valid) begin
      if (final_addr_queue.size() == 0) begin
        $display("  [ERROR] EXTRA ADDRESS: Got %0d at cycle %0d", ir_addr, current_cycle);
        error_count++;
      end else begin
        logic [ADDRESS_WIDTH-1:0] exp_addr = final_addr_queue.pop_front();
        int exp_cyc = final_cycle_queue.pop_front();

        if (ir_addr !== exp_addr) begin
          $display("  [ERROR] ADDR MISMATCH: Exp=%0d, Got=%0d (idx %0d)", exp_addr, ir_addr,
                   addr_count);
          error_count++;
        end else if (current_cycle !== exp_cyc) begin
          // Allow small slip due to simulator delta nuances, but generally strict
          $display("  [ERROR] CYC MISMATCH: Exp=%0d, Got=%0d (Addr=%0d)", exp_cyc, current_cycle,
                   ir_addr);
          error_count++;
        end
        addr_count++;
      end
    end
  endtask

  task automatic verify_final();
    if (final_addr_queue.size() > 0) begin
      $display("  [ERROR] MISSING DATA: %0d addresses remaining", final_addr_queue.size());
      error_count++;
    end
  endtask

  // --------------------------------------------------------------------------
  // Main Process
  // --------------------------------------------------------------------------
  always @(posedge clk) begin
    if (rst_n && enable) begin
      check_outputs();
      current_cycle++;
    end
  end

  initial begin
    rst_n  = 0;
    enable = 0;
    init_configs();
    @(posedge clk);
    rst_n = 1;

    $display("\n");
    $display("+========================================+");
    $display("|   OR_MT_IR TESTBENCH                   |");
    $display("+========================================+");

    // ============================================================
    // TEST 1: Baseline - Single Lane, No OR (1 iter)
    // ============================================================
    test_num++;
    $display("\nTEST %0d: Baseline (1 Lane, 1 OR pass)", test_num);
    init_configs();
    configure_lane_ir(0, 0, 0, 3, 1);  // Lane 0: 0,1,2
    configure_or(0, 1, 0);  // OR[0]: 1 iter, 0 delay

    build_grand_expected_sequence();

    current_cycle = 0;
    addr_count = 0;
    enable = 1;
    wait_for_completion();
    enable = 0;
    verify_final();
    rst_n = 0;
    @(posedge clk);
    rst_n = 1;

    // ============================================================
    // TEST 2: OR Repetition (2 iters)
    // ============================================================
    test_num++;
    $display("\nTEST %0d: OR Repetition (2 iterations)", test_num);
    init_configs();
    configure_lane_ir(0, 0, 0, 3, 1);  // Lane 0: 0,1,2
    configure_or(0, 2, 0);  // OR[0]: 2 iters

    // Expected: 0,1,2, 0,1,2
    build_grand_expected_sequence();

    current_cycle = 0;
    addr_count = 0;
    enable = 1;
    wait_for_completion();
    enable = 0;
    verify_final();
    rst_n = 0;
    @(posedge clk);
    rst_n = 1;

    // ============================================================
    // TEST 3: OR Repetition with Delay
    // ============================================================
    test_num++;
    $display("\nTEST %0d: OR Repetition (2 iters, 5 cycle delay)", test_num);
    init_configs();
    configure_lane_ir(0, 0, 0, 2, 1);  // Lane 0: 0,1
    configure_or(0, 2, 5);  // OR[0]: 2 iters, 5 delay

    // Expected: 0,1 ..wait 5.. 0,1
    build_grand_expected_sequence();

    current_cycle = 0;
    addr_count = 0;
    enable = 1;
    wait_for_completion();
    enable = 0;
    verify_final();
    rst_n = 0;
    @(posedge clk);
    rst_n = 1;

    // ============================================================
    // TEST 4: Nested OR (2 levels)
    // ============================================================
    test_num++;
    $display("\nTEST %0d: Nested OR (Level 0: 2x, Level 1: 2x)", test_num);
    init_configs();
    configure_lane_ir(0, 0, 0, 2, 1);  // Lane 0: 0,1
    configure_or(0, 2, 2);  // OR[0]: 2 iters, delay 2
    configure_or(1, 2, 10);  // OR[1]: 2 iters, delay 10

    // Pattern: (Block -> d2 -> Block) -> d10 -> (Block -> d2 -> Block)
    build_grand_expected_sequence();

    current_cycle = 0;
    addr_count = 0;
    enable = 1;
    wait_for_completion();
    enable = 0;
    verify_final();
    rst_n = 0;
    @(posedge clk);
    rst_n = 1;

    // ============================================================
    // TEST 5: Complex Full Stack (OR + MT + IR)
    // ============================================================
    test_num++;
    $display("\nTEST %0d: Full Stack (OR + MT Transitions + IR)", test_num);
    init_configs();

    // Lane 0: 0,1
    configure_lane_ir(0, 0, 0, 2, 1);
    // Lane 1: 0,1,2
    configure_lane_ir(1, 0, 0, 3, 1);

    // MT: Lane 0 -> Wait 3 -> Lane 1
    configure_mt_trans(0, 3);

    // OR: Repeat whole thing 2 times with delay 5
    configure_or(0, 2, 5);

    build_grand_expected_sequence();

    current_cycle = 0;
    addr_count = 0;
    enable = 1;
    wait_for_completion();
    enable = 0;
    verify_final();
    rst_n = 0;
    @(posedge clk);
    rst_n = 1;

    // ============================================================
    // TEST 6: No OR
    // ============================================================
    test_num++;
    $display("\nTEST %0d: No OR (Level 0: 10x)", test_num);
    init_configs();
    configure_lane_ir(0, 0, 0, 10, 1);  // Lane 0: 0..9

    build_grand_expected_sequence();

    current_cycle = 0;
    addr_count = 0;
    enable = 1;
    repeat (10) @(posedge clk);
    enable = 0;
    verify_final();
    rst_n = 0;
    @(posedge clk);
    rst_n = 1;

    // ============================================================
    // TEST 7: No OR, Two IR levels
    // ============================================================
    test_num++;
    $display("\nTEST %0d: No OR (Level 0: 10x)", test_num);
    init_configs();
    configure_lane_ir(0, 0, 1, 4, 1);  // Lane 0: 0,1,2,3
    configure_lane_ir(0, 1, 57, 8, 1);  // Lane 1: 8times, 57 delay

    build_grand_expected_sequence();

    current_cycle = 0;
    addr_count = 0;
    enable = 1;
    wait_for_completion(2000);
    enable = 0;
    verify_final();

    $display("\n+========================================+");
    if (error_count == 0) $display("|   STATUS: PASSED                       |");
    else $display("|   STATUS: FAILED (%0d errors)           |", error_count);
    $display("+========================================+\n");
    $finish;
  end

endmodule
