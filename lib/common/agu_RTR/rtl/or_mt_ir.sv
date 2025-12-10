module or_mt_ir
  import agu_RTR_pkg::*;
#(
    parameter int ADDRESS_WIDTH,
    parameter int NUMBER_IR,
    parameter int NUMBER_MT,
    parameter int NUMBER_OR  // Number of nesting levels for the Outer Loop
) (
    input logic clk,
    input logic rst_n,
    input logic enable,

    // Configurations
    // OR configs: Controls the repetition of the whole MT sequence
    input rep_config_t   [NUMBER_OR-1:0] or_configs,
    // MT/IR configs: Passed down to the child
    input trans_config_t [NUMBER_MT-1:0] mt_configs,
    input rep_config_t   [NUMBER_IR-1:0] rep_configs[NUMBER_MT+1],

    // Outputs
    output logic [ADDRESS_WIDTH-1:0] ir_addr,
    output logic                     ir_valid,
    output logic                     ir_done
);

  // --------------------------------------------------------------------------
  // Internal Signals & State
  // --------------------------------------------------------------------------
  typedef enum logic [1:0] {
    IDLE,
    RUN_CHILD,
    OR_DELAY,
    DONE
  } state_t;

  state_t state, state_next;

  // Child signals
  logic child_enable;
  logic child_valid;
  logic child_done;
  logic [ADDRESS_WIDTH-1:0] child_addr;

  // OR Level Counters (Same structure as ir)
  logic [REP_ITER_WIDTH-1:0] or_iter_count[NUMBER_OR];
  logic [REP_ITER_WIDTH-1:0] or_iter_count_next[NUMBER_OR];
  logic [REP_DELAY_WIDTH-1:0] or_delay_count, or_delay_count_next;

  // Logic to track active delay level for OR
  logic [$clog2(NUMBER_OR)-1:0] active_or_delay_level;
  logic [$clog2(NUMBER_OR)-1:0] active_or_delay_level_next;

  // --------------------------------------------------------------------------
  // Child Instantiation (MT_IR)
  // --------------------------------------------------------------------------
  mt_ir #(
      .ADDRESS_WIDTH(ADDRESS_WIDTH),
      .NUMBER_IR    (NUMBER_IR),
      .NUMBER_MT    (NUMBER_MT)
  ) mt_ir_inst (
      .clk        (clk),
      .rst_n      (rst_n),
      .enable     (child_enable),  // Controlled by OR State Machine
      .mt_configs (mt_configs),
      .rep_configs(rep_configs),
      .ir_addr    (child_addr),
      .ir_valid   (child_valid),
      .ir_done    (child_done)
  );

  // Pass through valid data immediately
  assign ir_addr  = child_addr;
  assign ir_valid = child_valid;

  // --------------------------------------------------------------------------
  // OR Logic: Level Calculations (Adapted from ir.sv)
  // --------------------------------------------------------------------------

  // Max level calculation: Determine the highest active OR level
  logic [$clog2(NUMBER_OR+1)-1:0] max_or_level;
  logic any_or_configured;
  always_comb begin
    max_or_level = 0;
    any_or_configured = 1'b0;
    for (int i = 0; i < NUMBER_OR; i++) begin
      if (or_configs[i].iter > 0) begin
        max_or_level = i;
        any_or_configured = 1'b1;
      end
    end
  end

  // Check wrap conditions for each OR level
  logic or_level_at_last[NUMBER_OR];
  always_comb begin
    for (int i = 0; i < NUMBER_OR; i++) begin
      or_level_at_last[i] = (or_iter_count[i] >= or_configs[i].iter - 1);
    end
  end

  // Check if all OR levels are done
  logic all_or_done;
  always_comb begin
    all_or_done = 1'b1;
    if (!rst_n) begin
      all_or_done = 1'b0;
    end else if (any_or_configured) begin
      for (int i = 0; i <= max_or_level; i++) begin
        if (!or_level_at_last[i]) all_or_done = 1'b0;
      end
    end
  end

  // --------------------------------------------------------------------------
  // State Machine
  // --------------------------------------------------------------------------
  always_comb begin
    // Default assignments
    state_next = state;
    child_enable = 1'b0;
    or_delay_count_next = or_delay_count;
    active_or_delay_level_next = active_or_delay_level;

    for (int i = 0; i < NUMBER_OR; i++) begin
      or_iter_count_next[i] = or_iter_count[i];
    end

    // Reset counters while waiting in IDLE
    if (state == IDLE && !enable) begin
      for (int i = 0; i < NUMBER_OR; i++) begin
        or_iter_count_next[i] = '0;
      end
    end

    case (state)
      // ----------------------------------------------------------------------
      // IDLE
      // ----------------------------------------------------------------------
      IDLE: begin
        if (enable) begin
          // Start immediately.
          // Note: We don't check child_done here because child starts fresh.
          state_next   = RUN_CHILD;
          child_enable = 1'b1;
        end
      end

      // ----------------------------------------------------------------------
      // RUN_CHILD
      // Enable the MT child and wait for it to finish its sequence.
      // ----------------------------------------------------------------------
      RUN_CHILD: begin
        child_enable = 1'b1;

        if (child_done) begin
          // The child finished one full MT sequence (Lane 0 -> ... -> Lane N)
          // --- OR Counter Update Logic (Triggered on child completion) ---
          logic need_delay;
          logic [NUMBER_OR-1:0] level_increments;

          // 1. Calculate cascading increments
          level_increments[0] = 1'b1;  // Innermost OR always increments
          for (int i = 1; i < NUMBER_OR; i++) begin
            level_increments[i] = 1'b1;
            for (int j = 0; j < i; j++) begin
              if (!or_level_at_last[j]) level_increments[i] = 1'b0;
            end
          end

          // 2. Check for Delays
          need_delay = 1'b0;
          active_or_delay_level_next = 0;
          for (int i = 0; i < NUMBER_OR; i++) begin
            if (level_increments[i] && or_configs[i].delay > 0 && !or_level_at_last[i]) begin
              if (!need_delay) begin
                need_delay = 1'b1;
                active_or_delay_level_next = i;
              end
            end
          end

          // 3. Apply Increments
          for (int i = 0; i < NUMBER_OR; i++) begin
            if (level_increments[i]) begin
              if (or_level_at_last[i]) begin
                or_iter_count_next[i] = '0;
              end else begin
                or_iter_count_next[i] = or_iter_count[i] + 1'b1;
              end
            end
          end

          // 4. Determine Next State
          if (all_or_done) begin
            state_next = DONE;
          end else if (need_delay) begin
            state_next = OR_DELAY;
            or_delay_count_next = 1;
          end else begin
            state_next = RUN_CHILD;

            if (need_delay) begin
              // Load the configured delay
              // Note: We use the delay value directly. The state machine consumes
              // 1 cycle per count.
              or_delay_count_next = 1;
            end else begin
              // If config delay is 0, we set count to 0.
              // The OR_DELAY state will handle the 1-cycle minimum reset duration.
              or_delay_count_next = '0;
            end
          end
        end
      end

      // ----------------------------------------------------------------------
      // OR_DELAY
      // Holds child_enable low to reset the child, and waits user-defined cycles.
      // ----------------------------------------------------------------------
      OR_DELAY: begin
        child_enable = 1'b0;  // Resets mt_ir logic to IDLE

        // Check against the configured delay for the active level
        // Even if config delay is 0, we spend this 1 cycle here effectively resetting.
        if (or_delay_count >= or_configs[active_or_delay_level].delay) begin
          state_next = RUN_CHILD;
          or_delay_count_next = '0;
        end else begin
          or_delay_count_next = or_delay_count + 1'b1;
        end
      end

      // ----------------------------------------------------------------------
      // DONE
      // ----------------------------------------------------------------------
      DONE: begin
        if (!enable) begin
          state_next = IDLE;
        end
      end

      default: state_next = IDLE;
    endcase
  end

  // --------------------------------------------------------------------------
  // Sequential Logic
  // --------------------------------------------------------------------------
  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      state <= IDLE;
      or_delay_count <= '0;
      active_or_delay_level <= '0;
      for (int i = 0; i < NUMBER_OR; i++) begin
        or_iter_count[i] <= '0;
      end
    end else begin
      state <= state_next;
      or_delay_count <= or_delay_count_next;
      active_or_delay_level <= active_or_delay_level_next;
      for (int i = 0; i < NUMBER_OR; i++) begin
        or_iter_count[i] <= or_iter_count_next[i];
      end
    end
  end

  // Output Done Flag
  assign ir_done = (state_next == DONE);

endmodule
