// agu_controller — producer side of agu_cfg_if.
//
// Registers per-AGU configuration decoded from EVT / REP / TRANS instruction
// fields and drives it onto the agu_cfg_if.producer interfaces. Counterpart to
// agu_rtr (the consumer). Resource-specific instruction structs (evt_t / rep_t
// / trans_t, generated per-resource from isa.json) are unpacked by the resource
// top and passed in here as plain buses, so this module stays ISA-agnostic and
// is compiled once and shared.
//
// Used by the io resource. A resource that spans several slots resolves its
// own (slot, port) to an AGU index in its top and passes the result in via
// `evt_port`, so this module never has to know the slot layout.
module agu_controller #(
    parameter int ADDRESS_WIDTH     = 16,
    parameter int NUM_AGUS          = 2,
    parameter int NUMBER_IR         = 4,
    parameter int NUMBER_MT         = 3,
    parameter int NUMBER_OR         = 4,
    parameter int REP_DELAY_WIDTH   = 6,
    parameter int REP_ITER_WIDTH    = 6,
    parameter int REP_STEP_WIDTH    = 6,
    parameter int TRANS_DELAY_WIDTH = 12,
    // Widths of the rep / trans instruction fields (isa.json) and whether rep
    // has an ext form. The registers above are independent of them: a base rep
    // writes the low bits, an ext rep the bits from the field width up.
    parameter int REP_DELAY_FIELD_WIDTH   = REP_DELAY_WIDTH / 2,
    parameter int REP_ITER_FIELD_WIDTH    = REP_ITER_WIDTH / 2,
    parameter int REP_STEP_FIELD_WIDTH    = REP_STEP_WIDTH / 2,
    parameter int TRANS_DELAY_FIELD_WIDTH = TRANS_DELAY_WIDTH,
    parameter bit REP_EXT                 = 1'b1,
    parameter int REP_LANE_FIELD_WIDTH    = 2
) (
    input  logic clk,
    input  logic rst_n,

    // EVT — resource top supplies the already-resolved port (incl. any slot
    // offset) and the init address.
    input  logic                          evt_valid,
    input  logic [$clog2(NUM_AGUS)-1:0]   evt_port,
    input  logic [ADDRESS_WIDTH-1:0]      evt_init_addr,

    // REP — instruction fields; rep_ext selects base (low bits) or ext.
    input  logic                               rep_valid,
    input  logic                               rep_ext,
    input  logic [REP_DELAY_FIELD_WIDTH-1:0]   rep_delay,
    input  logic [REP_ITER_FIELD_WIDTH-1:0]    rep_iter,
    input  logic [REP_STEP_FIELD_WIDTH-1:0]    rep_step,
    // 0: opens a level. k > 0: lane k's step at the last OR level opened.
    input  logic [REP_LANE_FIELD_WIDTH-1:0]    rep_lane,

    // TRANS
    input  logic                               trans_valid,
    input  logic [TRANS_DELAY_FIELD_WIDTH-1:0] trans_delay,

    input  logic [NUM_AGUS-1:0]                    agu_done,
    agu_cfg_if.producer                            agu_configs  [NUM_AGUS],

    // One initial address per lane. Each EVT opens a lane and brings its own
    // base with it, so a second EVT on a port must not overwrite the first's:
    // held per AGU, both lanes swept from whichever base was written last.
    output logic [NUM_AGUS-1:0][NUMBER_MT:0][ADDRESS_WIDTH-1:0] init_address,

    // High per AGU once any of its mt/ir configs has been written. Optional —
    // consumers that don't need it (io/rf/dpu) leave it unconnected.
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

  // Base rep: the field zero-extended (step: sign-extended, it is signed) to
  // the register. Ext rep: the field into the bits from the field width up;
  // nothing when the register is no wider than the field.
  localparam logic [REP_DELAY_WIDTH-1:0] DELAY_LO = REP_DELAY_WIDTH'({REP_DELAY_FIELD_WIDTH{1'b1}});
  localparam logic [REP_ITER_WIDTH-1:0]  ITER_LO  = REP_ITER_WIDTH'({REP_ITER_FIELD_WIDTH{1'b1}});
  localparam logic [REP_STEP_WIDTH-1:0]  STEP_LO  = REP_STEP_WIDTH'({REP_STEP_FIELD_WIDTH{1'b1}});

  initial begin
    if (REP_ITER_WIDTH > (REP_EXT ? 2 : 1) * REP_ITER_FIELD_WIDTH)
      $error("agu_controller: REP_ITER_WIDTH exceeds what rep can load");
    if (REP_DELAY_WIDTH > (REP_EXT ? 2 : 1) * REP_DELAY_FIELD_WIDTH)
      $error("agu_controller: REP_DELAY_WIDTH exceeds what rep can load");
  end

  function automatic logic [REP_DELAY_WIDTH-1:0] load_delay(
      input logic [REP_DELAY_WIDTH-1:0] old, input logic [REP_DELAY_FIELD_WIDTH-1:0] field, input logic ext);
    return !ext ? REP_DELAY_WIDTH'(field)
                : (old & DELAY_LO) | (REP_DELAY_WIDTH'(field) << REP_DELAY_FIELD_WIDTH);
  endfunction

  function automatic logic [REP_ITER_WIDTH-1:0] load_iter(
      input logic [REP_ITER_WIDTH-1:0] old, input logic [REP_ITER_FIELD_WIDTH-1:0] field, input logic ext);
    return !ext ? REP_ITER_WIDTH'(field)
                : (old & ITER_LO) | (REP_ITER_WIDTH'(field) << REP_ITER_FIELD_WIDTH);
  endfunction

  // Explicit sign extension: REP_STEP_WIDTH'($signed(field)) loses the step on Questa 2022.4 (CI).
  function automatic logic [REP_STEP_WIDTH-1:0] sext_step(input logic [REP_STEP_FIELD_WIDTH-1:0] field);
    logic [REP_STEP_WIDTH-1:0] r;
    for (int i = 0; i < REP_STEP_WIDTH; i++)
      r[i] = field[(i < REP_STEP_FIELD_WIDTH) ? i : REP_STEP_FIELD_WIDTH-1];
    return r;
  endfunction

  function automatic logic [REP_STEP_WIDTH-1:0] load_step(
      input logic [REP_STEP_WIDTH-1:0] old, input logic [REP_STEP_FIELD_WIDTH-1:0] field, input logic ext);
    return !ext ? sext_step(field)
                : (old & STEP_LO) | (sext_step(field) << REP_STEP_FIELD_WIDTH);
  endfunction

  logic ext_rep;
  assign ext_rep = REP_EXT && rep_ext;


  // A REP nest deeper than NUMBER_IR spills its outer levels into the OR levels.
  // The level counter must therefore span IR *and* OR (else it wraps modulo
  // NUMBER_IR and silently overwrites IR level 0). OR_CNT/OR_IDX_W guard the
  // NUMBER_OR==0 case, where or_configs is a 1-deep dummy array.
  localparam int OR_CNT   = (NUMBER_OR > 0) ? NUMBER_OR : 1;
  localparam int OR_IDX_W = (OR_CNT > 1) ? $clog2(OR_CNT) : 1;
  localparam int IR_IDX_W = (NUMBER_IR > 1) ? $clog2(NUMBER_IR) : 1;
  localparam int LVL_W    = $clog2(NUMBER_IR + OR_CNT + 1);
  localparam int MT_CNT   = (NUMBER_MT > 0) ? NUMBER_MT : 1;
  localparam int MT_IDX_W = (MT_CNT > 1) ? $clog2(MT_CNT + 1) : 1;  

  agu_config_t agu_configs_reg     [NUM_AGUS];

  // Range levels (agu_cfg_if rl_configs): layout as there.
  localparam int LANE_W = (NUMBER_MT > 0) ? $clog2(NUMBER_MT + 1) : 1;
  typedef struct packed {
    logic [REP_DELAY_WIDTH-1:0] delay;
    logic [REP_ITER_WIDTH-1:0]  iter;
    logic [LANE_W-1:0]          lo;
    logic [LANE_W-1:0]          hi;
    logic                       is_configured;
  } rl_config_t;
  rl_config_t [((NUMBER_OR > 0) ? NUMBER_OR : 1)-1:0] rl_configs_reg [NUM_AGUS];
  logic [((NUMBER_OR > 0) ? NUMBER_OR : 1)-1:0][NUMBER_MT:0][REP_STEP_WIDTH-1:0] rl_lane_steps_reg [NUM_AGUS];

  // The program is read postfix: an EVT pushes a group of one lane, a TRANS
  // joins the top two groups, a REP repeats the top group: the lane's own (IR)
  // repetition for a single lane, a range level over the group's lanes for a
  // joined group. OR levels remain for a single lane after a TRANS with no
  // second group, and for IR levels spilling over NUMBER_IR.
  logic [NUMBER_MT:0][LANE_W-1:0] grp_lo_reg   [NUM_AGUS];
  logic [LANE_W:0]                grp_depth_reg[NUM_AGUS];
  logic [$clog2(((NUMBER_OR > 0) ? NUMBER_OR : 1) + 1)-1:0] rl_count_reg [NUM_AGUS];
  logic                           last_rl_reg  [NUM_AGUS]; // last base REP was a range level
  logic [OR_CNT-1:0][NUMBER_MT:0][REP_STEP_WIDTH-1:0] or_lane_steps_reg [NUM_AGUS];
  logic        use_or_reg          [NUM_AGUS];
  logic        or_configured_reg   [NUM_AGUS];

  logic [$clog2(NUM_AGUS)-1:0]    agu_config_index;
  logic [MT_IDX_W-1:0] current_lane_index  [NUM_AGUS];
  logic [LVL_W-1:0]               current_rep_level   [NUM_AGUS];
  logic [MT_IDX_W-1:0] current_trans_index [NUM_AGUS];

  // Active target AGU: the EVT's own port while an EVT is dispatched, otherwise
  // the index latched at the last EVT.
  logic [$clog2(NUM_AGUS)-1:0] agu_index;
  assign agu_index = evt_valid ? evt_port : agu_config_index;

  // The lane an EVT opens: the one after the lane in use. current_lane_index
  // resets to all-ones, so the first EVT on an AGU wraps it to lane 0. Sized
  // MT_IDX_W like current_lane_index: with NUMBER_MT==0, $clog2(NUMBER_MT+1)
  // is 0, and a [-1:0] lane index is two bits wide, so the wrap gave lane 2
  // and every EVT wrote outside ir_configs and agu_init_address.
  logic [MT_IDX_W-1:0] evt_lane;
  assign evt_lane = current_lane_index[agu_index] + 1'b1;

  // Selectors indexing the AGU currently being configured.
  //
  // A full-width REP field is assembled from two instructions: a base REP
  // (_ext=0) carrying the lower half, and a REPX (_ext=1) carrying the upper
  // half. Only the base advances current_rep_level (below), so the REPX must
  // point back one level to land on the same entry its base just wrote.
  logic [MT_IDX_W-1:0]            rep_opt;
  logic [LVL_W-1:0]               abs_lvl;   // absolute level of this REP
  logic                           rep_to_or; // this level lands in OR, not IR
  logic [IR_IDX_W-1:0]            ir_lvl;    // IR 2nd-dim index (when !rep_to_or)
  logic [OR_IDX_W-1:0]            or_lvl;    // OR index      (when  rep_to_or)
  always_comb begin
    rep_opt = current_lane_index[agu_index];
    abs_lvl = !ext_rep ? current_rep_level[agu_index]
                       : (current_rep_level[agu_index] == '0
                            ? '0
                            : current_rep_level[agu_index] - 1);
    // Spill to OR once IR is full. A TRANS (use_or_reg) forces the OR path and
    // resets the counter, so its OR index is the raw level; the auto-spill path
    // keeps counting through IR, so its OR index is level - NUMBER_IR.
    rep_to_or = use_or_reg[agu_index]
                || ((NUMBER_OR > 0) && (abs_lvl >= NUMBER_IR));
    ir_lvl    = abs_lvl[IR_IDX_W-1:0];
    or_lvl    = (NUMBER_OR > 0) ? (use_or_reg[agu_index] ? abs_lvl[OR_IDX_W-1:0]
                                                   : OR_IDX_W'(abs_lvl - LVL_W'(NUMBER_IR)))
                              : '0;
  end

  // A base REP on a joined group: a range level.
  logic [LANE_W:0]     depth;
  logic [LANE_W-1:0]   top_lo;
  logic                rep_to_rl;
  logic [$clog2(((NUMBER_OR > 0) ? NUMBER_OR : 1) + 1)-1:0] rl_idx, rl_last;
  always_comb begin
    depth     = grp_depth_reg[agu_index];
    top_lo    = (depth > 0) ? grp_lo_reg[agu_index][depth - 1] : '0;
    rep_to_rl = (depth >= 1) && (top_lo != LANE_W'(current_lane_index[agu_index]));
    rl_idx    = rl_count_reg[agu_index];
    rl_last   = (rl_idx == 0) ? '0 : rl_idx - 1;
  end

  // A lane rep writes one lane's step at the level the last base rep opened.
  logic                lane_rep;
  logic [LVL_W-1:0]    lane_abs_lvl;
  logic                lane_to_or;
  logic [OR_IDX_W-1:0] lane_or_lvl;
  always_comb begin
    lane_rep     = (rep_lane != '0);
    lane_abs_lvl = (current_rep_level[agu_index] == '0) ? '0
                                                        : current_rep_level[agu_index] - 1;
    lane_to_or   = use_or_reg[agu_index]
                   || ((NUMBER_OR > 0) && (lane_abs_lvl >= NUMBER_IR));
    lane_or_lvl  = (NUMBER_OR > 0) ? (use_or_reg[agu_index] ? lane_abs_lvl[OR_IDX_W-1:0]
                                                    : OR_IDX_W'(lane_abs_lvl - LVL_W'(NUMBER_IR)))
                               : '0;
  end

  always_ff @(posedge clk or negedge rst_n) begin : ff_agu_config_process
    if (!rst_n) begin
      agu_configs_reg     <= '{default: '0};
      or_lane_steps_reg   <= '{default: '0};
      rl_configs_reg      <= '{default: '0};
      rl_lane_steps_reg   <= '{default: '0};
      grp_lo_reg          <= '{default: '0};
      grp_depth_reg       <= '{default: '0};
      rl_count_reg        <= '{default: '0};
      last_rl_reg         <= '{default: 1'b0};
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
          or_lane_steps_reg[i]   <= '0;
          rl_configs_reg[i]      <= '0;
          rl_lane_steps_reg[i]   <= '0;
          grp_lo_reg[i]          <= '0;
          grp_depth_reg[i]       <= '0;
          rl_count_reg[i]        <= '0;
          last_rl_reg[i]         <= 1'b0;
          use_or_reg[i]          <= 1'b0;
          or_configured_reg[i]   <= 1'b0;
          current_lane_index[i]  <= '1;
          current_rep_level[i]   <= '0;
          current_trans_index[i] <= '0;
        end
      end

      if (evt_valid) begin
        agu_config_index              <= agu_index;
        current_lane_index[agu_index] <= evt_lane;
        current_rep_level[agu_index]  <= '0;
        if (grp_depth_reg[agu_index] <= NUMBER_MT) begin
          grp_lo_reg[agu_index][grp_depth_reg[agu_index]] <= evt_lane;
          grp_depth_reg[agu_index] <= grp_depth_reg[agu_index] + 1'b1;
        end

        // Default configuration of the lane this EVT opens: one iteration,
        // returning init_addr alone.
        //
        // It has to land on the lane being entered rather than on lane 0. A
        // second EVT on the same port opens lane 1, and writing the default to
        // lane 0 again left lane 1 with is_configured low, so mt_ir counted one
        // lane, the transition had nothing to step into, and only the first
        // lane ever ran.
        agu_configs_reg[agu_index].ir_configs[evt_lane][0].iter          <= 1;
        agu_configs_reg[agu_index].ir_configs[evt_lane][0].is_configured <= 1'b1;

      end else if (rep_valid && lane_rep) begin
        if (last_rl_reg[agu_index]) begin
          // At a range level, the lane is counted from its lo.
          if (LANE_W'(rl_configs_reg[agu_index][rl_last].lo + rep_lane) <= NUMBER_MT)
            rl_lane_steps_reg[agu_index][rl_last][LANE_W'(rl_configs_reg[agu_index][rl_last].lo + rep_lane)] <=
                load_step(rl_lane_steps_reg[agu_index][rl_last][LANE_W'(rl_configs_reg[agu_index][rl_last].lo + rep_lane)], rep_step, ext_rep);
        end else if (lane_to_or && rep_lane <= NUMBER_MT)
          or_lane_steps_reg[agu_index][lane_or_lvl][rep_lane] <= load_step(or_lane_steps_reg[agu_index][lane_or_lvl][rep_lane], rep_step, ext_rep);

      end else if (rep_valid && ((!ext_rep && rep_to_rl) || (ext_rep && last_rl_reg[agu_index]))) begin
        // Range level over the top group: a base REP opens the next one, a
        // REPX writes the upper half of the last.
        if (!ext_rep) begin
          if (rl_idx < ((NUMBER_OR > 0) ? NUMBER_OR : 1)) begin
            rl_configs_reg[agu_index][rl_idx].delay <= load_delay('0, rep_delay, 1'b0);
            rl_configs_reg[agu_index][rl_idx].iter  <= load_iter('0, rep_iter, 1'b0);
            rl_configs_reg[agu_index][rl_idx].lo    <= top_lo;
            rl_configs_reg[agu_index][rl_idx].hi    <= LANE_W'(current_lane_index[agu_index]);
            rl_configs_reg[agu_index][rl_idx].is_configured <= 1'b1;
            for (int l = 0; l <= NUMBER_MT; l++)
              rl_lane_steps_reg[agu_index][rl_idx][l] <=
                  (l >= top_lo && l <= current_lane_index[agu_index]) ? load_step('0, rep_step, 1'b0) : '0;
            rl_count_reg[agu_index] <= rl_idx + 1'b1;
          end
          last_rl_reg[agu_index] <= 1'b1;
        end else begin
          rl_configs_reg[agu_index][rl_last].delay <= load_delay(rl_configs_reg[agu_index][rl_last].delay, rep_delay, 1'b1);
          rl_configs_reg[agu_index][rl_last].iter  <= load_iter(rl_configs_reg[agu_index][rl_last].iter, rep_iter, 1'b1);
          for (int l = 0; l <= NUMBER_MT; l++)
            if (l >= rl_configs_reg[agu_index][rl_last].lo && l <= rl_configs_reg[agu_index][rl_last].hi)
              rl_lane_steps_reg[agu_index][rl_last][l] <= load_step(rl_lane_steps_reg[agu_index][rl_last][l], rep_step, 1'b1);
        end

      end else if (rep_valid) begin

        if (rep_to_or) begin
          for (int l = 0; l <= NUMBER_MT; l++)
            or_lane_steps_reg[agu_index][or_lvl][l] <= load_step(or_lane_steps_reg[agu_index][or_lvl][l], rep_step, ext_rep);
          agu_configs_reg[agu_index].or_configs[or_lvl].delay <= load_delay(agu_configs_reg[agu_index].or_configs[or_lvl].delay, rep_delay, ext_rep);
          agu_configs_reg[agu_index].or_configs[or_lvl].iter  <= load_iter(agu_configs_reg[agu_index].or_configs[or_lvl].iter, rep_iter, ext_rep);
          agu_configs_reg[agu_index].or_configs[or_lvl].step  <= load_step(agu_configs_reg[agu_index].or_configs[or_lvl].step, rep_step, ext_rep);
          agu_configs_reg[agu_index].or_configs[or_lvl].is_configured <= 1'b1;
          or_configured_reg[agu_index] <= 1'b1;

        end else begin
          agu_configs_reg[agu_index].ir_configs[rep_opt][ir_lvl].delay <= load_delay(agu_configs_reg[agu_index].ir_configs[rep_opt][ir_lvl].delay, rep_delay, ext_rep);
          agu_configs_reg[agu_index].ir_configs[rep_opt][ir_lvl].iter  <= load_iter(agu_configs_reg[agu_index].ir_configs[rep_opt][ir_lvl].iter, rep_iter, ext_rep);
          agu_configs_reg[agu_index].ir_configs[rep_opt][ir_lvl].step  <= load_step(agu_configs_reg[agu_index].ir_configs[rep_opt][ir_lvl].step, rep_step, ext_rep);
          agu_configs_reg[agu_index].ir_configs[rep_opt][ir_lvl].is_configured <= 1'b1;
        end

        // Only the base REP advances the level; the REPX writes the upper half
        // of the same entry (see abs_lvl above).
        if (!ext_rep) begin
          current_rep_level[agu_index] <= current_rep_level[agu_index] + 1;
          last_rl_reg[agu_index] <= 1'b0;
        end

      end else if (trans_valid && !or_configured_reg[agu_index]) begin
        // Join the top two groups: the transition is the one into the first
        // lane of the top group. With fewer than two groups, the next one.
        if (grp_depth_reg[agu_index] >= 2) begin
          agu_configs_reg[agu_index].mt_configs[MT_IDX_W'(grp_lo_reg[agu_index][grp_depth_reg[agu_index] - 1] - 1)].delay         <= TRANS_DELAY_WIDTH'(trans_delay);
          agu_configs_reg[agu_index].mt_configs[MT_IDX_W'(grp_lo_reg[agu_index][grp_depth_reg[agu_index] - 1] - 1)].is_configured <= 1'b1;
          grp_depth_reg[agu_index] <= grp_depth_reg[agu_index] - 1'b1;
        end else begin
          agu_configs_reg[agu_index].mt_configs[current_trans_index[agu_index]].delay         <= TRANS_DELAY_WIDTH'(trans_delay);
          agu_configs_reg[agu_index].mt_configs[current_trans_index[agu_index]].is_configured <= 1'b1;
        end
        // A TRANS with no second group sends the reps to an OR level.
        if (grp_depth_reg[agu_index] < 2) begin
          use_or_reg[agu_index] <= 1'b1;
          current_rep_level[agu_index] <= '0;
        end
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
      assign agu_configs[gi].or_lane_steps = or_lane_steps_reg[gi];
      assign agu_configs[gi].rl_configs    = rl_configs_reg[gi];
      assign agu_configs[gi].rl_lane_steps = rl_lane_steps_reg[gi];
    end
  endgenerate

  logic [NUM_AGUS-1:0][NUMBER_MT:0][ADDRESS_WIDTH-1:0] agu_init_address;
  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      agu_init_address <= '0;
    end else begin
      for (int i = 0; i < NUM_AGUS; i++) begin
        if (agu_done[i]) begin
          agu_init_address[i] <= '0;
        end
      end
      if (evt_valid) begin
        agu_init_address[agu_index][evt_lane] <= evt_init_addr;
      end
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
