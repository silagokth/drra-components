`define INCLUDE_IR_STATES
`define INCLUDE_MT_STATES
`define INCLUDE_OR_STATES

`define BASE_STATES 1 // IDLE

`ifdef INCLUDE_IR_STATES
`define IR_STATES 4
`else
`define IR_STATES 0
`endif

`ifdef INCLUDE_MT_STATES
`define MT_STATES 2
`else
`define MT_STATES 0
`endif

`ifdef INCLUDE_OR_STATES
`define OR_STATES 2
`else
`define OR_STATES 0
`endif

`define TOTAL_STATES (`BASE_STATES + `IR_STATES + `MT_STATES + `OR_STATES)
`define CLOG2(x) ((x) <=  4) ? 2 : \
                 ((x) <=  8) ? 3 : \
                 ((x) <= 16) ? 4 : \
                 -1

`define STATE_WIDTH `CLOG2(`TOTAL_STATES)

