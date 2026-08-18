// config_controller — a resource's configuration register file.
//
// Holds NUM_OPTIONS configurations, each of REGS_PER_OPTION registers of
// REG_WIDTH bits. A CONF instruction writes one register; the resource's AGU
// selects which configuration is live, and the whole selected set is presented
// on `current`.
//
// The write port is deliberately plain (valid / option / register / data) so
// this module stays ISA-agnostic: the resource's decoder is what turns a CONF
// instruction into those four signals, and it is compiled once and shared.
//
// Sizing follows the ISA: a CONF payload carries $clog2(NUM_OPTIONS) address
// bits and spends the remainder on data, so REG_WIDTH should be
// PAYLOAD_BITWIDTH - $clog2(NUM_OPTIONS). The assertion below catches a
// resource whose registers no longer fit what its ISA can deliver.
module config_controller #(
    parameter int NUM_OPTIONS     = 4,
    parameter int REGS_PER_OPTION = 1,
    parameter int REG_WIDTH       = 16,

    // Width of the runtime selector, i.e. the AGU address feeding `select_option`.
    parameter int SELECT_WIDTH    = 2,

    // 0: `current` follows the option latched at the last select (swb).
    // 1: while `select_valid` is high, `current` follows the live selector
    //    instead, so a configuration is readable the same cycle its address
    //    is generated (dpu).
    parameter bit BYPASS_ON_VALID = 1'b0,

    // $clog2(1) is 0, which would make a zero-width port; clamp to 1 so a
    // single-option or single-register bank still elaborates.
    localparam int OPTION_IDX_W = (NUM_OPTIONS > 1) ? $clog2(NUM_OPTIONS) : 1,
    localparam int REG_IDX_W    = (REGS_PER_OPTION > 1) ? $clog2(REGS_PER_OPTION) : 1
) (
    input logic clk,
    input logic rst_n,

    // CONF write port — one register per instruction.
    input logic                       write_valid,
    input logic [OPTION_IDX_W-1:0]    write_option,
    input logic [REG_IDX_W-1:0]       write_reg,
    input logic [REG_WIDTH-1:0]       write_data,

    // Runtime selection, driven from the resource's AGU.
    input logic                       select_valid,
    input logic [SELECT_WIDTH-1:0]    select_option,

    output logic [REGS_PER_OPTION-1:0][REG_WIDTH-1:0] current
);

  logic [NUM_OPTIONS-1:0][REGS_PER_OPTION-1:0][REG_WIDTH-1:0] config_reg;

  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      config_reg <= '0;
    end else if (write_valid) begin
      config_reg[write_option][write_reg] <= write_data;
    end
  end

  // Latched selection. Holds the last option the AGU addressed, so the
  // configuration stays applied after the address strobe goes away.
  logic [SELECT_WIDTH-1:0] selected_option;
  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      selected_option <= '0;
    end else if (select_valid) begin
      selected_option <= select_option;
    end
  end

  logic [SELECT_WIDTH-1:0] active_option;
  assign active_option = (BYPASS_ON_VALID && select_valid) ? select_option : selected_option;

  // SELECT_WIDTH may exceed $clog2(NUM_OPTIONS); an out-of-range selector
  // reads as unconfigured rather than wrapping into another option.
  always_comb begin
    current = '0;
    if (active_option < NUM_OPTIONS) current = config_reg[active_option];
  end

endmodule
