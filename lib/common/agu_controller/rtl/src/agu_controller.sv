// agu_controller — producer side of agu_cfg_if.
//
// Registers per-AGU configuration decoded from EVT / REP / TRANS instruction
// fields and drives it onto the agu_cfg_if.producer interfaces. Counterpart to
// agu_rtr (the consumer). Resource-specific instruction structs (evt_t / rep_t
// / trans_t, generated per-resource from isa.json) are unpacked by the resource
// top and passed in here as plain buses, so this module stays ISA-agnostic and
// is compiled once and shared.
//
// Used by the io / iosram_top / iosram_btm / iosram_both resources. The only
// per-resource variation is the EVT port (the iosram split adds a slot offset);
// that is resolved in the resource top and passed in via `evt_port`.
module agu_controller #(
    parameter int ADDRESS_WIDTH     = 16,
    parameter int NUM_AGUS          = 2,
    parameter int NUMBER_IR         = 4,
    parameter int NUMBER_MT         = 3,
    parameter int NUMBER_OR         = 4,
    parameter int REP_DELAY_WIDTH   = 6,
    parameter int REP_ITER_WIDTH    = 6,
    parameter int REP_STEP_WIDTH    = 6,
    parameter int TRANS_DELAY_WIDTH = 12
) (
    input  logic clk,
    input  logic rst_n,

    // EVT — resource top supplies the already-resolved port (incl. any slot
    // offset) and the init address.
    input  logic                          evt_valid,
    input  logic [$clog2(NUM_AGUS)-1:0]   evt_port,
    input  logic [ADDRESS_WIDTH-1:0]      evt_init_addr,

    // REP — half-width fields; base/ext halves selected by rep_ext.
    input  logic                          rep_valid,
    input  logic                          rep_ext,
    input  logic [REP_DELAY_WIDTH/2-1:0]  rep_delay,
    input  logic [REP_ITER_WIDTH/2-1:0]   rep_iter,
    input  logic [REP_STEP_WIDTH/2-1:0]   rep_step,

    // TRANS
    input  logic                          trans_valid,
    input  logic [TRANS_DELAY_WIDTH-1:0]  trans_delay,

    input  logic [NUM_AGUS-1:0]                    agu_done,
    agu_cfg_if.producer                            agu_configs  [NUM_AGUS],
    output logic [NUM_AGUS-1:0][ADDRESS_WIDTH-1:0] init_address,

    // High per AGU once any of its mt/ir configs has been written. Optional —
    // consumers that don't need it (io/iosram/rf/dpu) leave it unconnected.
    output logic [NUM_AGUS-1:0]                    agu_is_configured
);

  // ───────────────────────────────────────────────────────────────────────────
  // Registered configuration struct. Layout MUST stay byte-identical to
  // agu_cfg_if's sub-structs and array dimensions — it is driven onto the
  // interface whole-array below. Mirrors lib/common/jinja/agu_config.j2 (the
  // canonical layout used by the resource packages); keep the two in sync.
  // ───────────────────────────────────────────────────────────────────────────
  typedef struct packed {
    logic [REP_DELAY_WIDTH-1:0] delay;
    logic [REP_ITER_WIDTH-1:0]  iter;
    logic [REP_STEP_WIDTH-1:0]  step;
    logic                       is_configured;
  } rep_config_t;

  typedef struct packed {
    logic [TRANS_DELAY_WIDTH-1:0] delay;
    logic                         is_configured;
  } trans_config_t;

  typedef struct packed {
    rep_config_t   [((NUMBER_OR > 0) ? NUMBER_OR : 1)-1:0] or_configs;
    trans_config_t [((NUMBER_MT > 0) ? NUMBER_MT : 1)-1:0] mt_configs;
    rep_config_t   [NUMBER_MT:0][NUMBER_IR-1:0]            ir_configs;
  } agu_config_t;

  localparam int DELAY_HI = REP_DELAY_WIDTH/2;
  localparam int ITER_HI  = REP_ITER_WIDTH/2;
  localparam int STEP_HI  = REP_STEP_WIDTH/2;

  // A REP nest deeper than NUMBER_IR spills its outer levels into the OR levels.
  // The level counter must therefore span IR *and* OR (else it wraps modulo
  // NUMBER_IR and silently overwrites IR level 0). OR_CNT/OR_IDX_W guard the
  // NUMBER_OR==0 resources (iosram_btm/both) where or_configs is a 1-deep dummy.
  localparam int OR_CNT   = (NUMBER_OR > 0) ? NUMBER_OR : 1;
  localparam int OR_IDX_W = (OR_CNT > 1) ? $clog2(OR_CNT) : 1;
  localparam int IR_IDX_W = (NUMBER_IR > 1) ? $clog2(NUMBER_IR) : 1;
  localparam int LVL_W    = $clog2(NUMBER_IR + OR_CNT + 1);

  agu_config_t agu_configs_reg     [NUM_AGUS];
  logic        use_or_reg          [NUM_AGUS];
  logic        or_configured_reg   [NUM_AGUS];

  logic [$clog2(NUM_AGUS)-1:0]    agu_config_index;
  logic [$clog2(NUMBER_MT+1)-1:0] current_lane_index  [NUM_AGUS];
  logic [LVL_W-1:0]               current_rep_level   [NUM_AGUS];
  logic [$clog2(NUMBER_MT+1)-1:0] current_trans_index [NUM_AGUS];

  // Active target AGU: the EVT's own port while an EVT is dispatched, otherwise
  // the index latched at the last EVT.
  logic [$clog2(NUM_AGUS)-1:0] agu_index;
  assign agu_index = evt_valid ? evt_port : agu_config_index;

  // Selectors indexing the AGU currently being configured.
  //
  // A full-width REP field is assembled from two instructions: a base REP
  // (_ext=0) carrying the lower half, and a REPX (_ext=1) carrying the upper
  // half. Only the base advances current_rep_level (below), so the REPX must
  // point back one level to land on the same entry its base just wrote.
  logic [$clog2(NUMBER_MT+1)-1:0] rep_opt;
  logic [LVL_W-1:0]               abs_lvl;   // absolute level of this REP
  logic                           rep_to_or; // this level lands in OR, not IR
  logic [IR_IDX_W-1:0]            ir_lvl;    // IR 2nd-dim index (when !rep_to_or)
  logic [OR_IDX_W-1:0]            or_lvl;    // OR index      (when  rep_to_or)
  always_comb begin
    rep_opt = current_lane_index[agu_index];
    abs_lvl = !rep_ext ? current_rep_level[agu_index]
                       : (current_rep_level[agu_index] == '0
                            ? '0
                            : current_rep_level[agu_index] - 1);
    // Spill to OR once IR is full. A TRANS (use_or_reg) forces the OR path and
    // resets the counter, so its OR index is the raw level; the auto-spill path
    // keeps counting through IR, so its OR index is level - NUMBER_IR.
    rep_to_or = use_or_reg[agu_index]
                || ((NUMBER_OR > 0) && (abs_lvl >= NUMBER_IR));
    ir_lvl    = abs_lvl[IR_IDX_W-1:0];
    or_lvl    = use_or_reg[agu_index] ? abs_lvl[OR_IDX_W-1:0]
                                      : (abs_lvl - NUMBER_IR);
  end

  always_ff @(posedge clk or negedge rst_n) begin : ff_agu_config_process
    if (!rst_n) begin
      agu_configs_reg     <= '{default: '0};
      use_or_reg          <= '{default: 1'b0};
      or_configured_reg   <= '{default: 1'b0};
      agu_config_index    <= '0;
      current_lane_index  <= '{default: '1};
      current_rep_level   <= '{default: '0};
      current_trans_index <= '{default: '0};
    end else begin
      // Reset state for any AGU that just finished. Run unconditionally in
      // parallel with evt/rep dispatch — gating evt/rep on `!|agu_done` would
      // silently drop dispatches that land on the same cycle as some other
      // AGU's done (the mul_512_1_1 chunks-1..7-missing bug).
      for (int i = 0; i < NUM_AGUS; i++) begin
        if (agu_done[i]) begin
          agu_configs_reg[i]     <= '0;
          use_or_reg[i]          <= 1'b0;
          or_configured_reg[i]   <= 1'b0;
          current_lane_index[i]  <= '1;
          current_rep_level[i]   <= '0;
          current_trans_index[i] <= '0;
        end
      end

      if (evt_valid) begin
        agu_config_index              <= agu_index;
        current_lane_index[agu_index] <= current_lane_index[agu_index] + 1;
        current_rep_level[agu_index]  <= '0;

        // NOTE: Default configuration of the AGU is return init_addr only
        agu_configs_reg[agu_index].ir_configs[0][0].iter          <= 1;
        agu_configs_reg[agu_index].ir_configs[0][0].is_configured <= 1'b1;

      end else if (rep_valid) begin

        if (rep_to_or) begin
          if (!rep_ext) begin
            agu_configs_reg[agu_index].or_configs[or_lvl].delay[DELAY_HI-1:0] <= rep_delay;
            agu_configs_reg[agu_index].or_configs[or_lvl].iter[ITER_HI-1:0]   <= rep_iter;
            agu_configs_reg[agu_index].or_configs[or_lvl].step[STEP_HI-1:0]   <= rep_step;
          end else begin
            agu_configs_reg[agu_index].or_configs[or_lvl].delay[REP_DELAY_WIDTH-1:DELAY_HI] <= rep_delay;
            agu_configs_reg[agu_index].or_configs[or_lvl].iter[REP_ITER_WIDTH-1:ITER_HI]    <= rep_iter;
            agu_configs_reg[agu_index].or_configs[or_lvl].step[REP_STEP_WIDTH-1:STEP_HI]    <= rep_step;
          end
          agu_configs_reg[agu_index].or_configs[or_lvl].is_configured <= 1'b1;
          or_configured_reg[agu_index] <= 1'b1;

        end else begin
          if (!rep_ext) begin
            agu_configs_reg[agu_index].ir_configs[rep_opt][ir_lvl].delay[DELAY_HI-1:0] <= rep_delay;
            agu_configs_reg[agu_index].ir_configs[rep_opt][ir_lvl].iter[ITER_HI-1:0]   <= rep_iter;
            agu_configs_reg[agu_index].ir_configs[rep_opt][ir_lvl].step[STEP_HI-1:0]   <= rep_step;
          end else begin
            agu_configs_reg[agu_index].ir_configs[rep_opt][ir_lvl].delay[REP_DELAY_WIDTH-1:DELAY_HI] <= rep_delay;
            agu_configs_reg[agu_index].ir_configs[rep_opt][ir_lvl].iter[REP_ITER_WIDTH-1:ITER_HI]    <= rep_iter;
            agu_configs_reg[agu_index].ir_configs[rep_opt][ir_lvl].step[REP_STEP_WIDTH-1:STEP_HI]    <= rep_step;
          end
          agu_configs_reg[agu_index].ir_configs[rep_opt][ir_lvl].is_configured <= 1'b1;
        end

        // Only the base REP advances the level; the REPX writes the upper half
        // of the same entry (see abs_lvl above).
        if (!rep_ext)
          current_rep_level[agu_index] <= current_rep_level[agu_index] + 1;

      end else if (trans_valid && !or_configured_reg[agu_index]) begin
        agu_configs_reg[agu_index].mt_configs[current_trans_index[agu_index]].delay         <= trans_delay;
        agu_configs_reg[agu_index].mt_configs[current_trans_index[agu_index]].is_configured <= 1'b1;
        use_or_reg[agu_index] <= 1'b1;

        current_rep_level[agu_index]   <= '0;
        current_trans_index[agu_index] <= current_trans_index[agu_index] + 1;
      end
    end
  end

  genvar gi;
  generate
    for (gi = 0; gi < NUM_AGUS; gi++) begin : gen_cfg_drive
      assign agu_configs[gi].or_configs = agu_configs_reg[gi].or_configs;
      assign agu_configs[gi].mt_configs = agu_configs_reg[gi].mt_configs;
      assign agu_configs[gi].ir_configs = agu_configs_reg[gi].ir_configs;
    end
  endgenerate

  logic [NUM_AGUS-1:0][ADDRESS_WIDTH-1:0] agu_init_address;
  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      agu_init_address <= '0;
    end else if (evt_valid) begin
      agu_init_address[agu_index] <= evt_init_addr;
    end
  end
  assign init_address = agu_init_address;

  // ───────────────────────────────────────────────────────────────────────────
  // AGU configured check
  // ───────────────────────────────────────────────────────────────────────────
  always_comb begin
    agu_is_configured = '0;
    for (int i = 0; i < NUM_AGUS; i++) begin
      // mt_configs has NUMBER_MT entries (transitions); ir_configs first dim
      // has NUMBER_MT+1 entries (lanes). Iterate them separately so the bounds
      // match each array.
      for (int l = 0; l < NUMBER_MT; l++) begin
        if (agu_configs_reg[i].mt_configs[l].is_configured)
          agu_is_configured[i] = 1;
      end
      for (int l = 0; l <= NUMBER_MT; l++) begin
        for (int r = 0; r < NUMBER_IR; r++) begin
          if (agu_configs_reg[i].ir_configs[l][r].is_configured)
            agu_is_configured[i] = 1;
        end
      end
    end
  end

endmodule
