module ir_top
  import agu_RTR_pkg::rep_config_t;
#(
    parameter int ADDRESS_WIDTH,
    parameter int NUMBER_IR,
    parameter int DELAY_WIDTH,
    parameter int ITER_WIDTH,
    parameter int STEP_WIDTH
) (
    input  logic                            clk,
    input  logic                            rst_n,
    input  logic                            enable,
    input  rep_config_t [    NUMBER_IR-1:0] rep_configs,
    output logic        [ADDRESS_WIDTH-1:0] ir_addr,
    output logic                            ir_valid,
    output logic                            ir_done
);

  // ============================================================================
  // Internal Signals
  // ============================================================================

  // Loop counter state for each level
  logic [ADDRESS_WIDTH-1:0] loop_counter[NUMBER_IR];
  logic [ADDRESS_WIDTH-1:0] loop_counter_next[NUMBER_IR];

  // AGU address state for each level (each has its own address counter)
  logic [ADDRESS_WIDTH-1:0] agu_addr[NUMBER_IR];
  logic [ADDRESS_WIDTH-1:0] agu_addr_next[NUMBER_IR];

  // Loop control signals
  logic [NUMBER_IR-1:0] loop_last;  // Asserted when loop reaches its end value
  logic [NUMBER_IR-1:0] loop_clear;  // Triggers loop counter reset
  logic [NUMBER_IR:0] loop_enable;  // Enable signal for each loop (0=innermost)
  logic [NUMBER_IR:0] loop_enable_prev;  // Previous cycle's enable (for start triggers)

  // IR generator interface signals (for compatibility)
  logic [NUMBER_IR-1:0] ir_valid_array;
  logic [NUMBER_IR-1:0] ir_done_array;
  logic [ADDRESS_WIDTH-1:0] ir_addr_array[NUMBER_IR];
  logic [NUMBER_IR-1:0] ir_enable_array;

  // Level configuration tracking
  logic [NUMBER_IR-1:0] level_configured;
  logic [$clog2(NUMBER_IR)-1:0] max_level_configured;

  // Control signals
  logic agu_valid, agu_valid_next;
  logic all_loops_done;
  logic step_enable;
  logic init_signal;

  // ============================================================================
  // Level Configuration Detection
  // ============================================================================

  generate
    for (genvar k = 0; k < NUMBER_IR; k++) begin : gen_level_configured
      assign level_configured[k] = (rep_configs[k].iter > 0);
    end
  endgenerate

  always_comb begin
    max_level_configured = '0;
    for (int n = 0; n < NUMBER_IR; n++) begin
      if (level_configured[n]) begin
        max_level_configured = n;
      end
    end
  end

  // ============================================================================
  // Hardware Loop Logic (Based on VHDL ntx_dag)
  // ============================================================================

  // Initialization happens on first enable after reset
  assign init_signal = enable && !agu_valid && !all_loops_done;
  assign step_enable = enable && !all_loops_done && !init_signal;

  // Loop counter update logic
  generate
    for (genvar k = 0; k < NUMBER_IR; k++) begin : gen_loop_logic

      // Loop reaches last value when counter equals iter-1
      assign loop_last[k]  = (loop_counter[k] == (rep_configs[k].iter - 1)) && level_configured[k];

      // Clear when this loop completes and is enabled
      assign loop_clear[k] = loop_last[k] && loop_enable[k];

      // Loop counter next value
      always_comb begin
        if (!level_configured[k]) begin
          loop_counter_next[k] = '0;
        end else if (init_signal) begin
          loop_counter_next[k] = '0;  // Initialize on first enable
        end else if (loop_clear[k] && step_enable) begin
          loop_counter_next[k] = '0;  // Reset to 0 when loop completes
        end else if (loop_enable[k] && step_enable) begin
          loop_counter_next[k] = loop_counter[k] + 1'b1;  // Increment
        end else begin
          loop_counter_next[k] = loop_counter[k];  // Hold
        end
      end

    end
  endgenerate

  // Loop enable chain (carry propagation from inner to outer loops)
  // Innermost loop (level 0) is always enabled when stepping
  assign loop_enable[0] = 1'b1;

  generate
    for (genvar k = 1; k < NUMBER_IR; k++) begin : gen_loop_enable
      // Outer loop enables when inner loop clears (completes)
      assign loop_enable[k] = loop_clear[k-1];
    end
  endgenerate

  // Extra bit for overflow detection
  assign loop_enable[NUMBER_IR] = (NUMBER_IR > 0) ? loop_clear[NUMBER_IR-1] : 1'b0;

  // All loops done when outermost configured loop completes
  assign all_loops_done = loop_enable[max_level_configured+1];

  // ============================================================================
  // Address Generation Units (One per level)
  // ============================================================================
  // Each AGU maintains its own address that resets when its loop completes

  generate
    for (genvar k = 0; k < NUMBER_IR; k++) begin : gen_agu_logic

      always_comb begin
        if (!level_configured[k]) begin
          agu_addr_next[k] = '0;
        end else if (init_signal) begin
          // Initialize to base address (0 or could use rep_configs[k].base)
          agu_addr_next[k] = '0;
        end else if (loop_clear[k] && step_enable) begin
          // Reset AGU address when this loop completes
          agu_addr_next[k] = '0;
        end else if (loop_enable[k] && step_enable) begin
          // Increment by stride when this loop is enabled
          agu_addr_next[k] = agu_addr[k] + rep_configs[k].step;
        end else begin
          agu_addr_next[k] = agu_addr[k];  // Hold
        end
      end

    end
  endgenerate

  // ============================================================================
  // Valid Signal Generation
  // ============================================================================

  always_comb begin
    if (init_signal) begin
      agu_valid_next = 1'b1;  // Start valid on initialization
    end else if (!enable || all_loops_done) begin
      agu_valid_next = 1'b0;
    end else begin
      agu_valid_next = 1'b1;
    end
  end

  // ============================================================================
  // IR Generator Instances (For Compatibility)
  // ============================================================================

  genvar i;
  generate
    for (i = 0; i < NUMBER_IR; i++) begin : gen_ir_lane

      // Enable each IR generator based on loop enable signals
      assign ir_enable_array[i] = loop_enable[i] && step_enable && level_configured[i];

      ir_generator #(
          .ADDRESS_WIDTH(ADDRESS_WIDTH),
          .DELAY_WIDTH  (DELAY_WIDTH),
          .ITER_WIDTH   (ITER_WIDTH),
          .STEP_WIDTH   (STEP_WIDTH)
      ) ir_gen_inst (
          .clk       (clk),
          .rst_n     (rst_n),
          .enable    (ir_enable_array[i]),
          .rep_config(rep_configs[i]),
          .ir_addr   (ir_addr_array[i]),
          .ir_valid  (ir_valid_array[i]),
          .ir_done   (ir_done_array[i])
      );
    end
  endgenerate

  // ============================================================================
  // Output Assignment - Output from Level 0 AGU
  // ============================================================================

  assign ir_addr  = agu_addr[0];  // Output address from level 0 (innermost loop)
  assign ir_valid = agu_valid;
  assign ir_done  = all_loops_done;

  // ============================================================================
  // Sequential Logic
  // ============================================================================

  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      // Reset all loop counters
      for (int k = 0; k < NUMBER_IR; k++) begin
        loop_counter[k] <= '0;
        agu_addr[k] <= '0;
      end
      loop_enable_prev <= '0;
      agu_valid <= 1'b0;

    end else begin
      // Update loop counters and AGU addresses
      for (int k = 0; k < NUMBER_IR; k++) begin
        loop_counter[k] <= loop_counter_next[k];
        agu_addr[k] <= agu_addr_next[k];
      end

      // Store previous enable for start trigger detection
      if (step_enable) begin
        loop_enable_prev <= loop_enable;
      end

      // Update valid signal
      agu_valid <= agu_valid_next;
    end
  end

endmodule
