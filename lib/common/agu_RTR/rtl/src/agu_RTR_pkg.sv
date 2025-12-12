package agu_RTR_pkg;
  parameter int BASE_STATES = 1;  // IDLE state
  parameter int GEN_STATES = 1;  // GENERATE state
  parameter int IR_STATES = 4;
  parameter int MT_STATES = 2;
  parameter int OR_STATES = 2;
  parameter int TOTAL_STATES = IR_STATES + MT_STATES + OR_STATES + BASE_STATES + GEN_STATES;
  parameter int STATE_WIDTH = $clog2(TOTAL_STATES);

  class rep_config_class #(
      parameter int DELAY_WIDTH = 6,
      parameter int ITER_WIDTH  = 6
  );
    typedef struct packed {
      logic [DELAY_WIDTH-1:0] delay;
      logic [ITER_WIDTH-1:0] iter;
      logic is_configured;
    } rep_t;
  endclass

  class trans_config_class #(
      parameter int DELAY_WIDTH = 12
  );
    typedef struct packed {
      logic [DELAY_WIDTH-1:0] delay;
      logic is_configured;
    } trans_t;
  endclass

  class agu_config_class #(
      parameter int NUMBER_IR         = 4,
      parameter int NUMBER_MT         = 3,
      parameter int NUMBER_OR         = 4,
      parameter int REP_DELAY_WIDTH   = 6,
      parameter int REP_ITER_WIDTH    = 6,
      parameter int TRANS_DELAY_WIDTH = 12
  );
    typedef struct packed {
      logic [REP_DELAY_WIDTH-1:0] delay;
      logic [REP_ITER_WIDTH-1:0]  iter;
      logic                       is_configured;
    } rep_config_t;

    typedef struct packed {
      logic [TRANS_DELAY_WIDTH-1:0] delay;
      logic                         is_configured;
    } trans_config_t;

    typedef struct packed {
      rep_config_t [(NUMBER_OR > 0 ? NUMBER_OR : 1)-1:0] or_configs;
      trans_config_t [(NUMBER_MT > 0 ? NUMBER_MT : 1)-1:0] mt_configs;
      rep_config_t [NUMBER_IR-1:0][NUMBER_MT:0] ir_configs;
    } agu_config_t;
  endclass

  //typedef struct packed {
  //  logic [REP_DELAY_WIDTH-1:0] delay;
  //  logic [REP_ITER_WIDTH-1:0] iter;
  //  logic [REP_STEP_WIDTH-1:0] step;
  //  logic is_configured;
  //} rep_config_t;

  //typedef struct packed {
  //  logic [TRANS_DELAY_WIDTH-1:0] delay;
  //  logic is_configured;
  //} trans_config_t;

endpackage : agu_RTR_pkg
