package agu_rtr_pkg;
  parameter int BASE_STATES = 1;  // IDLE state
  parameter int GEN_STATES = 1;  // GENERATE state
  parameter int IR_STATES = 4;
  parameter int MT_STATES = 2;
  parameter int OR_STATES = 2;
  parameter int TOTAL_STATES = IR_STATES + MT_STATES + OR_STATES + BASE_STATES + GEN_STATES;
  parameter int STATE_WIDTH = $clog2(TOTAL_STATES);
endpackage : agu_rtr_pkg
