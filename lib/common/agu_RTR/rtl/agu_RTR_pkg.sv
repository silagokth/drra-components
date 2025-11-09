package agu_RTR_pkg;

  parameter int BASE_STATES = 1;  // IDLE state
  parameter int GEN_STATES = 1;  // GENERATE state
  parameter int IR_STATES = 4;
  parameter int MT_STATES = 2;
  parameter int OR_STATES = 2;
  parameter int TOTAL_STATES = IR_STATES + MT_STATES + OR_STATES + BASE_STATES + GEN_STATES;
  parameter int STATE_WIDTH = $clog2(TOTAL_STATES);

  parameter int REP_DELAY_WIDTH = 6;
  parameter int REP_ITER_WIDTH = 6;
  parameter int REP_STEP_WIDTH = 6;

  typedef struct packed {
    logic [REP_DELAY_WIDTH-1:0] delay;
    logic [REP_ITER_WIDTH-1:0]  iter;
    logic [REP_STEP_WIDTH-1:0]  step;
  } rep_config_t;

endpackage
