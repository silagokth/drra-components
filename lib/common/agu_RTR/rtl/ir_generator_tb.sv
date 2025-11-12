`timescale 1ns / 1ps

module ir_generator_tb
  import agu_RTR_pkg::*;
();

  // Parameters
  localparam int ADDRESS_WIDTH = 16;
  localparam int CLK_PERIOD = 10;  // 10ns = 100MHz

  // DUT signals
  logic                            clk;
  logic                            rst_n;
  logic                            enable;
  rep_config_t                     rep_config;
  logic        [ADDRESS_WIDTH-1:0] ir_addr;
  logic                            ir_valid;
  logic                            ir_done;

  // Testbench variables
  int                              test_num = 0;
  int                              error_count = 0;
  int                              addr_count = 0;
  logic        [ADDRESS_WIDTH-1:0] expected_addr;
  logic        [ADDRESS_WIDTH-1:0] addr_queue      [$];  // Queue to store expected addresses

  // ===== DUT Instantiation =====
  ir_generator #(
      .ADDRESS_WIDTH(ADDRESS_WIDTH),
      .DELAY_WIDTH  (REP_DELAY_WIDTH),
      .ITER_WIDTH   (REP_ITER_WIDTH),
      .STEP_WIDTH   (REP_STEP_WIDTH)
  ) dut (
      .clk(clk),
      .rst_n(rst_n),
      .enable(enable),
      .rep_config(rep_config),
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

  // Build expected address sequence
  task automatic build_expected_addresses(input int iterations, input int step);
    addr_queue.delete();
    for (int i = 0; i < iterations; i++) begin
      addr_queue.push_back(i * step);
    end
  endtask

  // Apply configuration
  task automatic configure(input int delay, input int iter, input int step);
    rep_config.delay = delay;
    rep_config.iter  = iter;
    rep_config.step  = step;
    build_expected_addresses(iter, step);
  endtask

  // Wait for generation to complete
  task automatic wait_for_completion(input int timeout_cycles = 10000);
    int cycle_count = 0;

    while (!ir_done && cycle_count < timeout_cycles) begin
      @(posedge clk);
      cycle_count++;
    end

    if (cycle_count >= timeout_cycles) begin
      $error("[TEST %0d] TIMEOUT: ir_done not asserted within %0d cycles", test_num,
             timeout_cycles);
      error_count++;
    end
  endtask

  // Check generated addresses
  task automatic check_address(input logic [ADDRESS_WIDTH-1:0] addr, input logic valid);
    if (valid) begin
      if (addr_queue.size() == 0) begin
        $error("[TEST %0d] EXTRA ADDRESS: Got addr=%0d but no more addresses expected", test_num,
               addr);
        error_count++;
      end else begin
        expected_addr = addr_queue.pop_front();
        if (addr !== expected_addr) begin
          $error("[TEST %0d] ADDRESS MISMATCH: Expected=%0d, Got=%0d", test_num, expected_addr,
                 addr);
          error_count++;
        end else begin
          $display("  [Cycle %0t] Address %0d: PASS (addr=%0d)", $time, addr_count, addr);
        end
        addr_count++;
      end
    end
  endtask

  // Print test header
  task automatic print_test_header(input string test_name, input int delay, input int iter,
                                   input int step);
    test_num++;
    addr_count = 0;
    $display("\n========================================");
    $display("TEST %0d: %s", test_num, test_name);
    $display("  Config: delay=%0d, iter=%0d, step=%0d", delay, iter, step);
    $display("  Expected addresses: %p", addr_queue);
    $display("========================================");
  endtask

  // Verify final state
  task automatic verify_completion();
    if (addr_queue.size() != 0) begin
      $error("[TEST %0d] MISSING ADDRESSES: %0d addresses not generated", test_num,
             addr_queue.size());
      error_count++;
    end else begin
      $display("  [TEST %0d] COMPLETE: All %0d addresses generated correctly", test_num,
               addr_count);
    end

    if (!ir_done) begin
      $error("[TEST %0d] ir_done not asserted at completion", test_num);
      error_count++;
    end
  endtask

  // ===== Monitor Process =====
  always @(posedge clk) begin
    if (rst_n && enable) begin
      check_address(ir_addr, ir_valid);
    end
  end

  // ===== Main Test Sequence =====
  initial begin
    // Initialize
    rst_n = 0;
    enable = 0;
    rep_config = '{delay: 0, iter: 0, step: 0};

    $display("\n");
    $display("+----------------------------------------+");
    $display("|   IR GENERATOR TESTBENCH               |");
    $display("+----------------------------------------+");

    // Reset
    repeat (5) @(posedge clk);
    rst_n = 1;
    repeat (2) @(posedge clk);

    // ========================================
    // TEST 1: Basic Sequential (no delay, step=1)
    // ========================================
    configure(.delay(0), .iter(5), .step(1));
    print_test_header("Basic Sequential", 0, 5, 1);

    enable = 1;
    wait_for_completion(100);
    enable = 0;
    verify_completion();
    repeat (5) @(posedge clk);

    // ========================================
    // TEST 2: Sequential with Delay
    // ========================================
    configure(.delay(3), .iter(4), .step(1));
    print_test_header("Sequential with Delay", 3, 4, 1);

    enable = 1;
    wait_for_completion(100);
    enable = 0;
    verify_completion();
    repeat (5) @(posedge clk);

    // ========================================
    // TEST 3: Strided Access (step=4)
    // ========================================
    configure(.delay(0), .iter(8), .step(4));
    print_test_header("Strided Access", 0, 8, 4);

    enable = 1;
    wait_for_completion(100);
    enable = 0;
    verify_completion();
    repeat (5) @(posedge clk);

    // ========================================
    // TEST 4: Strided with Delay
    // ========================================
    configure(.delay(2), .iter(6), .step(10));
    print_test_header("Strided with Delay", 2, 6, 10);

    enable = 1;
    wait_for_completion(200);
    enable = 0;
    verify_completion();
    repeat (5) @(posedge clk);

    // ========================================
    // TEST 5: Single Iteration
    // ========================================
    configure(.delay(1), .iter(1), .step(1));
    print_test_header("Single Iteration", 1, 1, 1);

    enable = 1;
    wait_for_completion(100);
    enable = 0;
    verify_completion();
    repeat (5) @(posedge clk);

    // ========================================
    // TEST 6: Large Delay
    // ========================================
    configure(.delay(10), .iter(3), .step(2));
    print_test_header("Large Delay", 10, 3, 2);

    enable = 1;
    wait_for_completion(200);
    enable = 0;
    verify_completion();
    repeat (5) @(posedge clk);

    // ========================================
    // TEST 7: Zero Step (same address repeated)
    // ========================================
    configure(.delay(0), .iter(4), .step(0));
    print_test_header("Zero Step", 0, 4, 0);

    enable = 1;
    wait_for_completion(100);
    enable = 0;
    verify_completion();
    repeat (5) @(posedge clk);

    // ========================================
    // TEST 8: Enable Control (start/stop)
    // ========================================
    configure(.delay(1), .iter(10), .step(1));
    print_test_header("Enable Control", 1, 10, 1);

    $display("  Testing enable on/off control...");
    enable = 1;
    repeat (10) @(posedge clk);  // Let it run a bit
    enable = 0;
    $display("  [Paused at cycle %0t]", $time);
    repeat (5) @(posedge clk);  // Pause
    enable = 1;
    $display("  [Resumed at cycle %0t]", $time);
    wait_for_completion(300);
    enable = 0;
    verify_completion();
    repeat (5) @(posedge clk);

    // ========================================
    // TEST 9: Back-to-back Operations
    // ========================================
    $display("\n========================================");
    $display("TEST %0d: Back-to-back Operations", test_num + 1);
    $display("========================================");

    for (int i = 0; i < 3; i++) begin
      test_num++;
      configure(.delay(0), .iter(3), .step(5));
      $display("  [Iteration %0d] Starting...", i);
      enable = 1;
      wait_for_completion(100);
      enable = 0;
      verify_completion();
      @(posedge clk);
    end

    // ========================================
    // TEST 10: Maximum Values
    // ========================================
    configure(.delay(63), .iter(63), .step(63));
    print_test_header("Maximum Values", 63, 63, 63);

    enable = 1;
    wait_for_completion(5000);
    enable = 0;
    verify_completion();
    repeat (5) @(posedge clk);

    // ========================================
    // Final Report
    // ========================================
    repeat (10) @(posedge clk);

    $display("\n");
    $display("+----------------------------------------+");
    $display("|   TEST SUMMARY                         |");
    $display("+----------------------------------------+");
    $display("|   Total Tests: %3d                     |", test_num);
    if (error_count == 0) begin
      $display("|   Status: ALL PASSED                   |");
    end else begin
      $display("|   Status: %3d ERRORS                   |", error_count);
    end
    $display("+----------------------------------------+");
    $display("\n");

    if (error_count == 0) begin
      $display("SUCCESS: All tests passed!");
    end else begin
      $display("FAILURE: %0d error(s) found", error_count);
    end

    $finish;
  end

  // ===== Timeout Watchdog =====
  initial begin
    #500000;  // 500us timeout
    $error("GLOBAL TIMEOUT: Simulation exceeded 500us");
    $finish;
  end

  // ===== Waveform Dump =====
  initial begin
    $dumpfile("ir_generator_tb.vcd");
    $dumpvars(0, ir_generator_tb);
  end

endmodule
