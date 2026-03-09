`timescale 1ns / 1ps

module agu_tb
  import agu_rtr_pkg::*;
  import timing_model_pkg::*;
  import tb_utils_pkg::*;
();
  // Parameters
  localparam int AddressWidth = 16;
  localparam int NumberIr = 4;
  localparam int NumberMt = 3;
  localparam int NumberOr = 4;
  localparam int ClkPeriod = 10;
  localparam int RepDelayWidth = 6;
  localparam int RepIterWidth = 6;
  localparam int RepStepWidth = 6;
  localparam int TransDelayWidth = 12;

  // Configurations
  typedef agu_config_class#(
      .NUMBER_IR        (NumberIr),
      .NUMBER_MT        (NumberMt),
      .NUMBER_OR        (NumberOr),
      .REP_DELAY_WIDTH  (RepDelayWidth),
      .REP_ITER_WIDTH   (RepIterWidth),
      .REP_STEP_WIDTH   (RepStepWidth),
      .TRANS_DELAY_WIDTH(TransDelayWidth)
  )::agu_config_t agu_config_t;

  // DUT signals
  logic clk;
  logic rst_n;
  logic enable;
  agu_config_t agu_config;
  logic [AddressWidth-1:0] agu_addr;
  logic agu_act;
  logic agu_valid;
  logic agu_done;
  logic pass;

  // Testbench State
  int current_cycle;
  int current_lane_idx;
  int current_trans_idx;
  int current_rep_level;
  int addr_count = 0;
  logic use_or_reg;
  logic or_configured_reg;

  // Helpers
  TimingModelHelper tm;
  TestReporter reporter;

  // DUT
  agu_rtr #(
      .ADDRESS_WIDTH    (AddressWidth),
      .NUMBER_IR        (NumberIr),
      .NUMBER_MT        (NumberMt),
      .NUMBER_OR        (NumberOr),
      .REP_DELAY_WIDTH  (RepDelayWidth),
      .REP_ITER_WIDTH   (RepIterWidth),
      .REP_STEP_WIDTH   (RepStepWidth),
      .TRANS_DELAY_WIDTH(TransDelayWidth)
  ) dut (
      .clk       (clk),
      .rst_n     (rst_n),
      .enable    (enable),
      .activation(agu_act),
      .agu_config(agu_config),
      .addr      (agu_addr),
      .addr_valid(agu_valid),
      .done      (agu_done)
  );

  // Clock
  initial begin
    clk = 0;
    forever #(ClkPeriod / 2) clk = ~clk;
  end

  // Random seed
  int seed;
  initial begin
    seed = 0;  // default seed

    // Override if +seed= is provided
    if ($value$plusargs("seed=%d", seed)) begin
      $display($sformatf("Using provided seed: %0d", seed));
    end else begin
      $display($sformatf("Using random seed: %0d", seed));
    end
    void'($urandom(seed));
  end

  // --------------------------------------------------------------------------
  // Helper Tasks
  // --------------------------------------------------------------------------
  task automatic init_test();
    // Reset timing model
    tm.clear();

    // Reset DUT config
    agu_config = '{default: '0};
    agu_act = 0;
    current_lane_idx = -1;
    current_trans_idx = 0;
    current_rep_level = 0;
    addr_count = 0;
    current_cycle = 0;
    use_or_reg = 0;
    or_configured_reg = 0;
  endtask

  task automatic add_event();
    tm.add_event();
    current_lane_idx++;
    current_rep_level = 0;
    current_trans_idx = 0;

    // // NOTE: default, returns the first address
    // agu_config.ir_configs[0][0].iter = 1;
    // agu_config.ir_configs[0][0].is_configured = 1;
  endtask

  task automatic add_rep_random();
    int iter;
    int step;
    int delay;
    iter  = $urandom_range(2, 5);
    step  = $urandom_range(0, 10);
    delay = $urandom_range(0, 5);
    add_rep(iter, step, delay);
  endtask

  task automatic add_rep(input int iter, input int step, input int delay);
    // Add to timing model
    tm.add_repetition(iter, delay, step);

    // Add to DUT configs
    if (use_or_reg) begin
      agu_config.or_configs[current_rep_level].delay = delay;
      agu_config.or_configs[current_rep_level].iter = iter;
      agu_config.or_configs[current_rep_level].step = step;
      agu_config.or_configs[current_rep_level].is_configured = 1;
      or_configured_reg = 1;
    end else begin
      agu_config.ir_configs[current_lane_idx][current_rep_level].delay = delay;
      agu_config.ir_configs[current_lane_idx][current_rep_level].iter = iter;
      agu_config.ir_configs[current_lane_idx][current_rep_level].step = step;
      agu_config.ir_configs[current_lane_idx][current_rep_level].is_configured = 1;
    end

    // Increment rep level
    current_rep_level++;
  endtask

  task automatic add_trans_random();
    int delay;
    delay = $urandom_range(1, 5);
    add_trans(delay);
  endtask

  task automatic add_trans(input int delay);
    // Add to timing model
    tm.add_transition(delay);

    // Add to DUT configs
    agu_config.mt_configs[current_trans_idx].delay = delay;
    agu_config.mt_configs[current_trans_idx].is_configured = 1;

    use_or_reg = 1;  // Once a transition is added, subsequent reps are for OR level
    current_rep_level = 0;
    current_trans_idx++;
  endtask

  task automatic reset_dut();
    agu_act = 0;
    rst_n   = 0;
    @(posedge clk);
    rst_n = 1;
    repeat (2) @(posedge clk);
  endtask

  task automatic run_test(string name);
    int timeout;

    pass = 1;
    timeout = tm.build();
    reporter.print_test_header(name, tm.expression, tm.addr_count, timeout);
    timeout += 3;  // extra cycle for activation

    enable = 1;
    current_cycle = -1;
    agu_act = 1;

    // Wait for completion
    fork
      begin
        while (!agu_done) @(posedge clk);
      end
      begin
        repeat (timeout) @(posedge clk);
        reporter.log_error($sformatf("Timeout after %0d cycles", timeout));
        pass = 0;
      end
    join_any
    disable fork;

    enable = 0;

    // Verify completion
    if (tm.remaining() > 0) begin
      reporter.log_error($sformatf("%0d addresses not generated", tm.remaining()));
      pass = 0;
    end
    if (!agu_done) begin
      reporter.log_error("Done signal not asserted");
      pass = 0;
    end

    if (pass) reporter.log_pass($sformatf("Test completed in %0d cycles", current_cycle));
    reset_dut();
  endtask

  // Monitor
  always @(posedge clk) begin
    automatic int result;
    if (rst_n && enable) begin
      if (current_cycle == -1) begin
        current_cycle++;
        continue;
      end
      result = tm.verify(current_cycle, agu_addr, agu_valid);
      if (result == 0) begin
        pass = 0;
        reporter.log_error(
            $sformatf(
            "Mismatch at idx %0d (cycle %0d, addr %0d)", addr_count, current_cycle, agu_addr));
      end else if (result == -1) begin
        pass = 0;
        reporter.log_error($sformatf("Extra address %0d", agu_addr));
      end
      if (agu_valid) addr_count++;
      current_cycle++;
    end
  end

  // --------------------------------------------------------------------------
  // Test Sequence
  // --------------------------------------------------------------------------
  initial begin
    tm = new();
    reporter = new("AGU TESTBENCH");
    reporter.print_banner();

    rst_n  = 0;
    enable = 0;
    reset_dut();

    for (int n_ir = 0; n_ir <= NumberIr; n_ir++) begin
      for (int n_mt = 0; n_mt <= NumberMt; n_mt++) begin
        for (int n_or = 0; n_or <= NumberOr; n_or++) begin
          if (n_ir == 0 && n_mt == 0) continue;
          if (n_mt == 0 && n_or > 0) continue;  // ORs require at least 1 MT
          init_test();
          for (int i = 0; i <= n_mt; i++) begin
            add_event();
            // Add IR repetitions
            for (int j = 0; j < n_ir; j++) add_rep_random();  // or random/varied values
          end
          // Add MT transitions
          for (int t = 0; t < n_mt; t++) add_trans_random();  // or random/varied values
          // Add OR repetitions
          for (int r = 0; r < n_or; r++) add_rep_random();  // or random/varied values
          run_test($sformatf("Pattern: IR=%0d, MT=%0d, OR=%0d", n_ir, n_mt, n_or));
        end
      end
    end

    reporter.print_summary();
    $finish;
  end

  // Timeout watchdog
  initial begin
    #100000000;
    reporter.log_error("[ERROR] Global timeout reached");
    $finish;
  end

  // Waveform dump
  initial begin
    $dumpfile("agu_tb.vcd");
    $dumpvars(0, agu_tb);
  end
endmodule
