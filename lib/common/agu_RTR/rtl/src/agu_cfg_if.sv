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
  localparam int OR_DIM = (NUMBER_OR > 0) ? NUMBER_OR : 1;
  localparam int MT_DIM = (NUMBER_MT > 0) ? NUMBER_MT : 1;

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

  rep_config_t   [ OR_DIM-1:0]                or_configs;
  trans_config_t [ MT_DIM-1:0]                mt_configs;
  rep_config_t   [NUMBER_MT:0][NUMBER_IR-1:0] ir_configs;

  modport producer(output or_configs, mt_configs, ir_configs);
  modport consumer(input or_configs, mt_configs, ir_configs);
endinterface
