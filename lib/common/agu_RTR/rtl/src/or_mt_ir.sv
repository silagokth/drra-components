module or_mt_ir
  import agu_rtr_pkg::*;
#(
    parameter int ADDRESS_WIDTH,
    parameter int NUMBER_IR,
    parameter int NUMBER_MT,
    parameter int NUMBER_OR,
    parameter int REP_DELAY_WIDTH,
    parameter int REP_ITER_WIDTH,
    parameter int REP_STEP_WIDTH,
    parameter int TRANS_DELAY_WIDTH
) (
    input  logic                                   clk,
    input  logic                                   rst_n,
    input  logic                                   enable,
           agu_cfg_if.consumer                     cfg,
    output logic               [ADDRESS_WIDTH-1:0] ir_addr,
    output logic                                   ir_valid,
    output logic                                   ir_done
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
  logic [$clog2(NUMBER_OR+1)-1:0] active_or_delay_level;
  logic [$clog2(NUMBER_OR+1)-1:0] active_or_delay_level_next;

  // Hoisted from inside the RUN_CHILD case-item: older Genus / DC versions
  // reject `logic` declarations mid-case.
  logic                  need_delay;
  logic [NUMBER_OR-1:0]  level_increments;

  // --------------------------------------------------------------------------
  // Child Instantiation (MT_IR)
  // --------------------------------------------------------------------------
  mt_ir #(
      .ADDRESS_WIDTH    (ADDRESS_WIDTH),
      .NUMBER_IR        (NUMBER_IR),
      .NUMBER_MT        (NUMBER_MT),
      .REP_DELAY_WIDTH  (REP_DELAY_WIDTH),
      .REP_ITER_WIDTH   (REP_ITER_WIDTH),
      .REP_STEP_WIDTH   (REP_STEP_WIDTH),
      .TRANS_DELAY_WIDTH(TRANS_DELAY_WIDTH)
  ) mt_ir_inst (
      .clk     (clk),
      .rst_n   (rst_n),
      .enable  (child_enable),  // Controlled by OR State Machine
      .cfg     (cfg),
      .ir_addr (child_addr),
      .ir_valid(child_valid),
      .ir_done (child_done)
  );

  // Pass through valid data immediately
  assign ir_valid = child_valid;
  always_comb begin
    ir_addr = child_addr;
    for (int o = 0; o < NUMBER_OR; o++) begin
      if (or_iter_count[o] > 0) begin
        ir_addr += or_iter_count[o] * cfg.or_configs[o].step;
      end
    end
  end

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
      if (cfg.or_configs[i].iter > 0) begin
        max_or_level = i;
        any_or_configured = 1'b1;
      end
    end
  end

  // Check wrap conditions for each OR level. Gate with `iter > 0` so an
  // unconfigured level (iter == 0) does not underflow `iter - 1` to all-ones
  // and produce a wide compare with no functional purpose.
  logic or_level_at_last[NUMBER_OR];
  always_comb begin
    for (int i = 0; i < NUMBER_OR; i++) begin
      or_level_at_last[i] = (cfg.or_configs[i].iter > 0) &&
                            (or_iter_count[i] >= cfg.or_configs[i].iter - 1);
    end
  end

  // Check if all OR levels are done.
  // No `if (!rst_n)` guard here: the sequential reset of `or_iter_count`
  // already drives this comb chain to 0, so the explicit gate would only
  // add fanout on rst_n.
  logic all_or_done;
  always_comb begin
    all_or_done = 1'b1;
    if (any_or_configured) begin
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
    need_delay = 1'b0;
    level_increments = '0;

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
          child_enable = 1;
          if (child_done && all_or_done)
            state_next = DONE;
          else
            state_next = RUN_CHILD;
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
            if (level_increments[i] && cfg.or_configs[i].delay > 0 && !or_level_at_last[i]) begin
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
            or_delay_count_next = '0;
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
        if (or_delay_count >= cfg.or_configs[active_or_delay_level].delay) begin
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

  // Output Done Flag.
  // Driven from the registered `state` (not `state_next`) so the comb
  // path through the FSM next-state logic does not appear at the module
  // output. Costs 1 cycle of latency on the done pulse vs the prior
  // `state_next == DONE` form.
  assign ir_done = (state == DONE);

endmodule
