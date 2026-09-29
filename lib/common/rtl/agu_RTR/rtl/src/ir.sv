module ir
  import agu_rtr_pkg::*;
#(
    parameter int ADDRESS_WIDTH,
    parameter int NUMBER_IR,
    parameter int DELAY_WIDTH,
    parameter int ITER_WIDTH,
    parameter int STEP_WIDTH,
    parameter int LANE = 0
) (
    input  logic                     clk,
    input  logic                     rst_n,
    input  logic                     enable,
    agu_cfg_if.consumer              cfg,
    output logic [ADDRESS_WIDTH-1:0] ir_addr,
    output logic                     ir_valid,
    output logic                     ir_done
);

  typedef enum logic [1:0] {
    IDLE,
    OUTPUTTING,
    DELAYING,
    DONE
  } state_t;
  state_t state, state_next;

  logic [ITER_WIDTH-1:0] iter_count[NUMBER_IR];
  logic [ITER_WIDTH-1:0] iter_count_next[NUMBER_IR];
  logic [DELAY_WIDTH-1:0] delay_count, delay_count_next;

  // Per-level address accumulator. Holds `iter_count[i] * step[i]` as a
  // running sum so the output path uses adders only — no multipliers.
  // ASSUMPTION: cfg.ir_configs[LANE][i].step is held constant by the
  // controller for the duration of an AGU run. If runtime step changes
  // are ever introduced, this approach must be revisited.
  logic [ADDRESS_WIDTH-1:0] level_addr      [NUMBER_IR];
  logic [ADDRESS_WIDTH-1:0] level_addr_next [NUMBER_IR];

  // $clog2 is 0 when NUMBER_IR==1, which would make an illegal [-1:0] range;
  // clamp to a 1-bit index so IR=1 (a single level) elaborates cleanly.
  localparam int IR_IDX_W = (NUMBER_IR > 1) ? $clog2(NUMBER_IR) : 1;
  logic [IR_IDX_W-1:0] active_delay_level;
  logic [IR_IDX_W-1:0] active_delay_level_next;

  // Check wrap conditions. An unconfigured level (iter == 0) is treated
  // as "perpetually at_last" so it never blocks the cascade or the
  // all_done check. The contiguous-config invariant (no gaps between
  // configured levels) means iter == 0 only occurs above the highest
  // configured level, so this rule is consistent with the original
  // intent of the loop.
  logic level_at_last[NUMBER_IR];
  always_comb begin
    for (int i = 0; i < NUMBER_IR; i++) begin
      level_at_last[i] = (cfg.ir_configs[LANE][i].iter == 0) ||
                         (iter_count[i] >= cfg.ir_configs[LANE][i].iter - 1);
    end
  end

  // Check all done. Loop bound is the parameter NUMBER_IR (compile-time
  // constant) so synth tools that reject runtime for-loop bounds elaborate
  // cleanly. Unconfigured levels return level_at_last == 1, so they pass
  // through without affecting the result.
  logic all_done;
  always_comb begin
    all_done = 1'b1;
    for (int i = 0; i < NUMBER_IR; i++) begin
      if (!level_at_last[i]) all_done = 1'b0;
    end
  end

  //-------------------------------------------------------------------------
  // Unified Processing Flag
  // This signal is High when we are effectively outputting data.
  // It covers the steady state (OUTPUTTING) and the zero-latency start (IDLE+Enable).
  //-------------------------------------------------------------------------
  logic is_processing;
  assign is_processing = (state == OUTPUTTING) || (state == IDLE && enable);

  // State Machine and Counter Logic
  always_comb begin
    // Defaults
    state_next = state;
    delay_count_next = delay_count;
    active_delay_level_next = active_delay_level;
    ir_done = 1'b0;

    for (int i = 0; i < NUMBER_IR; i++) begin
      iter_count_next[i] = iter_count[i];
      level_addr_next[i] = level_addr[i];
    end

    // Reset counters while waiting in IDLE
    if (state == IDLE && !enable) begin
      for (int i = 0; i < NUMBER_IR; i++) begin
        iter_count_next[i] = '0;
        level_addr_next[i] = '0;
      end
    end

    //---------------------------------------------------------------------
    // UNIFIED UPDATE LOGIC
    // Runs in OUTPUTTING state OR immediately when IDLE receives enable.
    // This ensures Cycle 0 outputs '0', and Cycle 1 outputs '1' (or delays).
    //---------------------------------------------------------------------
    if (is_processing) begin
        logic need_delay;
        logic [NUMBER_IR-1:0] level_increments;

        // 1. Calculate cascading increments
        level_increments[0] = 1'b1; // Innermost always increments
        for (int i = 1; i < NUMBER_IR; i++) begin
          level_increments[i] = 1'b1;
          for (int j = 0; j < i; j++) begin
            if (!level_at_last[j]) level_increments[i] = 1'b0;
          end
        end

        // 2. Check for Delays (innermost incrementing level)
        need_delay = 1'b0;
        active_delay_level_next = 0;
        for (int i = 0; i < NUMBER_IR; i++) begin
          if (level_increments[i] && cfg.ir_configs[LANE][i].delay > 0 && !level_at_last[i]) begin
             // Priority to inner loops: only capture if we haven't found one yet
            if (!need_delay) begin
              need_delay = 1'b1;
              active_delay_level_next = i;
            end
          end
        end

        // 3. Apply Increments
        for (int i = 0; i < NUMBER_IR; i++) begin
          if (level_increments[i]) begin
            if (level_at_last[i]) begin
              iter_count_next[i] = '0;
              level_addr_next[i] = '0;
            end else begin
              iter_count_next[i] = iter_count[i] + 1'b1;
              level_addr_next[i] = level_addr[i]
                                   + ADDRESS_WIDTH'(cfg.ir_configs[LANE][i].step);
            end
          end
        end

        // 4. Determine Next State
        if (all_done) begin
          ir_done = 1'b1;
          state_next = IDLE;
        end else if (need_delay) begin
          state_next = DELAYING;
          delay_count_next = 1; // 1 cycle used by current output
        end else begin
          state_next = OUTPUTTING;
        end
    end

    //---------------------------------------------------------------------
    // DELAYING State Logic
    //---------------------------------------------------------------------
    else if (state == DELAYING) begin
      if (delay_count >= cfg.ir_configs[LANE][active_delay_level].delay) begin
        state_next = OUTPUTTING;
        delay_count_next = '0;
      end else begin
        delay_count_next = delay_count + 1'b1;
      end
    end

    //---------------------------------------------------------------------
    // DONE State Logic
    //---------------------------------------------------------------------
    else if (state == DONE) begin
      if (!enable) begin
        state_next = IDLE;
      end
    end
  end

  // Sequential Logic
  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      state <= IDLE;
      delay_count <= '0;
      active_delay_level <= '0;
      for (int i = 0; i < NUMBER_IR; i++) begin
        iter_count[i] <= '0;
        level_addr[i] <= '0;
      end
    end else begin
      state <= state_next;
      delay_count <= delay_count_next;
      active_delay_level <= active_delay_level_next;
      for (int i = 0; i < NUMBER_IR; i++) begin
        iter_count[i] <= iter_count_next[i];
        level_addr[i] <= level_addr_next[i];
      end
    end
  end

  // Outputs.
  // Sum the per-level address accumulators rather than recomputing
  // `iter_count[i] * step[i]` combinationally. Removes NUMBER_IR
  // multipliers from the addr critical path.
  always_comb begin
    ir_addr = '0;
    for (int i = 0; i < NUMBER_IR; i++) begin
      if (cfg.ir_configs[LANE][i].iter > 0)
        ir_addr = ir_addr + level_addr[i];
    end
  end

  // Valid is High immediately upon enable (Zero Latency)
  // and stays High during subsequent OUTPUTTING states.
  assign ir_valid = is_processing;

endmodule
