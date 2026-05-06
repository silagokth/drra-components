// AGU configuration interface.
//
// Replaces the parameterized class typedef previously declared as
// `agu_config_class#(...)::agu_config_t` in `agu_rtr_pkg`. Each AGU
// instance gets its own interface instance, parameterized to the
// widths the producer and consumer agree on. Interfaces are
// synthesizable by Vivado, Design Compiler, Genus and Verilator,
// unlike SV classes.
//
// Producer (resource controller) drives the configuration signals
// via the `producer` modport. Consumer (`agu_rtr` and its
// submodules) reads them via the `consumer` modport.

interface agu_cfg_if #(
    parameter int NUMBER_IR         = 4,
    parameter int NUMBER_MT         = 3,
    parameter int NUMBER_OR         = 4,
    parameter int REP_DELAY_WIDTH   = 6,
    parameter int REP_ITER_WIDTH    = 6,
    parameter int REP_STEP_WIDTH    = 6,
    parameter int TRANS_DELAY_WIDTH = 12
);
  // OR_DIM / MT_DIM: clamp to >=1 so packed dims never collapse to 0
  // when NUMBER_OR or NUMBER_MT is 0. Real entry count still tracks
  // NUMBER_OR / NUMBER_MT.
  localparam int OR_DIM = (NUMBER_OR > 0) ? NUMBER_OR : 1;
  localparam int MT_DIM = (NUMBER_MT > 0) ? NUMBER_MT : 1;

  // Per-loop repetition config (used for IR levels and OR levels):
  //   delay = cycles to wait before next iteration
  //   iter  = total iteration count for this level
  //   step  = address stride applied per iteration
  typedef struct packed {
    logic [REP_DELAY_WIDTH-1:0] delay;
    logic [REP_ITER_WIDTH-1:0]  iter;
    logic [REP_STEP_WIDTH-1:0]  step;
    logic                       is_configured;
  } rep_config_t;

  // Per-MT-transition config: delay between consecutive IR lanes.
  typedef struct packed {
    logic [TRANS_DELAY_WIDTH-1:0] delay;
    logic                         is_configured;
  } trans_config_t;

  // or_configs[NUMBER_OR]
  //   Outer-loop (OR) repetition config, one entry per OR level.
  //   Indexed [0 .. NUMBER_OR-1]. Innermost OR = idx 0.
  rep_config_t   [ OR_DIM-1:0]                or_configs;

  // mt_configs[NUMBER_MT]
  //   MT transition delay between IR lanes. NUMBER_MT transitions
  //   exist between NUMBER_MT+1 IR lanes.
  //   Indexed [0 .. NUMBER_MT-1]. mt_configs[k] = delay AFTER lane k,
  //   before entering lane k+1.
  trans_config_t [ MT_DIM-1:0]                mt_configs;

  // ir_configs[NUMBER_MT+1][NUMBER_IR]
  //   Per-IR-lane repetition config.
  //   1st dim = IR lane index, [0 .. NUMBER_MT]. NUMBER_MT+1 lanes
  //            because NUMBER_MT transitions need NUMBER_MT+1 lanes
  //            to sit between.
  //   2nd dim = repetition level inside that lane, [0 .. NUMBER_IR-1].
  //            Innermost loop = idx 0.
  rep_config_t   [NUMBER_MT:0][NUMBER_IR-1:0] ir_configs;

  modport producer(output or_configs, mt_configs, ir_configs);
  modport consumer(input or_configs, mt_configs, ir_configs);
endinterface
