`include "config.sv"

module agu_RTR #(
    parameter int ADDRESS_WIDTH,

    parameter int REP_LEVEL_WIDTH,
    parameter int REP_DELAY_WIDTH,
    parameter int REP_ITER_WIDTH,
    parameter int REP_STEP_WIDTH,
    parameter int TRANS_LEVEL_WIDTH,
    parameter int TRANS_DELAY_WIDTH,

    parameter int NUMBER_OR,
    parameter int NUMBER_MT,
    parameter int NUMBER_IR
) (
    input logic clk,
    input logic rst_n,
    input logic activation,

`ifdef INCLUDE_IR_STATES
    input logic rep_valid,
    input logic repx_valid,
`elsif INCLUDE_OR_STATES
    input logic rep_valid,
    input logic repx_valid,
`endif

`ifdef INCLUDE_IR_STATES
    input logic [  REP_DELAY_WIDTH-1:0] rep_delay_IR [(NUMBER_MT+1)*NUMBER_IR],
    input logic [   REP_ITER_WIDTH-1:0] rep_iter_IR  [(NUMBER_MT+1)*NUMBER_IR],
    input logic [   REP_STEP_WIDTH-1:0] rep_step_IR  [(NUMBER_MT+1)*NUMBER_IR],
    input logic                         rep_config_IR[(NUMBER_MT+1)*NUMBER_IR],
`endif
`ifdef INCLUDE_MT_STATES
    input logic                         trans_valid,
    input logic [TRANS_DELAY_WIDTH-1:0] trans_delay  [              NUMBER_MT],
    input logic                         trans_config [              NUMBER_MT],
`endif
`ifdef INCLUDE_OR_STATES
    input logic [  REP_DELAY_WIDTH-1:0] rep_delay_OR [              NUMBER_OR],
    input logic [   REP_ITER_WIDTH-1:0] rep_iter_OR  [              NUMBER_OR],
    input logic [   REP_STEP_WIDTH-1:0] rep_step_OR  [              NUMBER_OR],
    input logic                         rep_config_OR[              NUMBER_OR],
`endif

    output logic                     address_valid,
    output logic [ADDRESS_WIDTH-1:0] address
);

  logic en_address;

`ifdef INCLUDE_IR_STATES
  logic [   REP_ITER_WIDTH-1:0] regIR_iter      [(NUMBER_MT+1)*NUMBER_IR];
  logic [  REP_DELAY_WIDTH-1:0] regIR_delay     [(NUMBER_MT+1)*NUMBER_IR];
  logic [   REP_STEP_WIDTH-1:0] regIR_step      [(NUMBER_MT+1)*NUMBER_IR];
  logic                         regIR_config    [(NUMBER_MT+1)*NUMBER_IR];
  logic                         initIR_address;
  logic [$clog2(NUMBER_IR)-1:0] level_IR;
  logic [    ADDRESS_WIDTH-1:0] initVal_IR      [          1:NUMBER_IR-1];
  logic                         en_initVal_IR   [          1:NUMBER_IR-1];
  logic                         init_initVal_IR [          1:NUMBER_IR-1];
  logic                         init0_initVal_IR[          1:NUMBER_IR-1];
`endif

`ifdef INCLUDE_MT_STATES
  logic [TRANS_DELAY_WIDTH-1:0] regMT_delay   [NUMBER_MT];
  logic                         regMT_config  [NUMBER_MT];
  logic                         init0_address;
`endif

  logic [$clog2(NUMBER_MT+1)-1:0] level_MT;

`ifdef INCLUDE_OR_STATES
  logic [   REP_ITER_WIDTH-1:0] regOR_iter     [NUMBER_OR];
  logic [  REP_DELAY_WIDTH-1:0] regOR_delay    [NUMBER_OR];
  logic [   REP_STEP_WIDTH-1:0] regOR_step     [NUMBER_OR];
  logic                         regOR_config   [NUMBER_OR];
  logic                         initOR_address;
  logic [$clog2(NUMBER_OR)-1:0] level_OR;
  logic [    ADDRESS_WIDTH-1:0] initVal_OR     [NUMBER_OR];
  logic                         en_initVal_OR  [NUMBER_OR];
  logic                         init_initVal_OR[NUMBER_OR];
  logic                         flag_OR;
`endif

  /////////////////////////////////////// Config registers /////////////////////////////////////// 

  genvar i;
`ifdef INCLUDE_IR_STATES
  generate
    for (i = 0; i < (NUMBER_MT + 1) * NUMBER_IR; i++) begin : IR_registers
      register #(
          .WIDTH(REP_ITER_WIDTH)
      ) regIR_iter_inst (
          .clk(clk),
          .rst_n(rst_n),
          .init0(),
          .enable(rep_valid && rep_config_IR[i]),
          .in_value(rep_iter_IR[i]),
          .out_value(regIR_iter[i])
      );

      register #(
          .WIDTH(REP_DELAY_WIDTH)
      ) regIR_delay_inst (
          .clk(clk),
          .rst_n(rst_n),
          .init0(),
          .enable(rep_valid && rep_config_IR[i]),
          .in_value(rep_delay_IR[i]),
          .out_value(regIR_delay[i])
      );

      register #(
          .WIDTH(REP_STEP_WIDTH)
      ) regIR_step_inst (
          .clk(clk),
          .rst_n(rst_n),
          .init0(),
          .enable(rep_valid && rep_config_IR[i]),
          .in_value(rep_step_IR[i]),
          .out_value(regIR_step[i])
      );

      register #(
          .WIDTH(1'b1)
      ) regIR_config_inst (
          .clk(clk),
          .rst_n(rst_n),
          .init0(),
          .enable(rep_valid && rep_config_IR[i]),
          .in_value(rep_config_IR[i]),
          .out_value(regIR_config[i])
      );
    end
  endgenerate
`endif

`ifdef INCLUDE_MT_STATES
  generate
    for (i = 0; i < NUMBER_MT; i++) begin : MT_registers
      register #(
          .WIDTH(TRANS_DELAY_WIDTH)
      ) regMT_delay_inst (
          .clk(clk),
          .rst_n(rst_n),
          .init0(),
          .enable(trans_valid && trans_config[i]),
          .in_value(trans_delay[i]),
          .out_value(regMT_delay[i])
      );

      register #(
          .WIDTH(1'b1)
      ) regMT_config_inst (
          .clk(clk),
          .rst_n(rst_n),
          .init0(),
          .enable(trans_valid && trans_config[i]),
          .in_value(trans_config[i]),
          .out_value(regMT_config[i])
      );
    end
  endgenerate
`endif

`ifdef INCLUDE_OR_STATES
  generate
    for (i = 0; i < NUMBER_OR; i++) begin : OR_registers
      register #(
          .WIDTH(REP_ITER_WIDTH)
      ) regOR_iter_inst (
          .clk(clk),
          .rst_n(rst_n),
          .init0(),
          .enable(rep_valid && rep_config_OR[i]),
          .in_value(rep_iter_OR[i]),
          .out_value(regOR_iter[i])
      );

      register #(
          .WIDTH(REP_DELAY_WIDTH)
      ) regOR_delay_inst (
          .clk(clk),
          .rst_n(rst_n),
          .init0(),
          .enable(rep_valid && rep_config_OR[i]),
          .in_value(rep_delay_OR[i]),
          .out_value(regOR_delay[i])
      );

      register #(
          .WIDTH(REP_STEP_WIDTH)
      ) regOR_step_inst (
          .clk(clk),
          .rst_n(rst_n),
          .init0(),
          .enable(rep_valid && rep_config_OR[i]),
          .in_value(rep_step_OR[i]),
          .out_value(regOR_step[i])
      );

      register #(
          .WIDTH(1'b1)
      ) regOR_config_inst (
          .clk(clk),
          .rst_n(rst_n),
          .init0(),
          .enable(rep_valid && rep_config_OR[i]),
          .in_value(rep_config_OR[i]),
          .out_value(regOR_config[i])
      );
    end
  endgenerate
`endif

  /////////////////////////////////////// Counters ///////////////////////////////////////
`ifdef INCLUDE_IR_STATES
`ifdef INCLUDE_OR_STATES
  logic [ADDRESS_WIDTH-1:0] address_reg;
  logic [ADDRESS_WIDTH-1:0] address_next;

  always_ff @(posedge clk, negedge rst_n) begin : Address_Counter
    if (!rst_n) begin
      address_reg <= 0;
    end else begin
      address_reg <= address_next;
    end
  end

  always_comb begin
    // default: hold value
    address_next = address_reg;

    if (init0_address) begin
      address_next = flag_OR ? initVal_OR[level_OR] : '0;
    end else if (initIR_address) begin
      address_next = flag_OR ? initVal_IR[level_IR] + initVal_OR[level_OR] : initVal_IR[level_IR];
    end else if (initOR_address) begin
      address_next = initVal_OR[level_OR];
    end else if (en_address) begin
      address_next = address_reg + regIR_step[NUMBER_IR*level_MT+0];
    end
  end

  assign address = address_next;
`endif
`ifndef INCLUDE_OR_STATES
  always @(posedge clk, negedge rst_n) begin : Address_Counter
    if (!rst_n) begin
      address <= 0;
    end else if (init0_address) begin
      address <= 0;
    end else if (initIR_address) begin
      address <= initVal_IR[level_IR];
    end else if (en_address) begin
      address <= address + regIR_step[NUMBER_IR*level_MT+0];
    end
  end
`endif
`endif

`ifndef INCLUDE_IR_STATES
`ifdef INCLUDE_OR_STATES
  always @(posedge clk, negedge rst_n) begin : Address_Counter
    if (!rst_n) begin
      address <= 0;
    end else if (initOR_address) begin
      address <= initVal_OR[level_OR];
    end else if (en_address) begin
      address <= address + 1'b1;
    end
  end
`endif
`ifndef INCLUDE_OR_STATES
  always @(posedge clk, negedge rst_n) begin : Address_Counter
    if (!rst_n) begin
      address <= 0;
    end else if (en_address) begin
      address <= address + 1'b1;
    end
  end
`endif
`endif

`ifdef INCLUDE_IR_STATES
  generate
    for (i = 1; i < NUMBER_IR; i++) begin : initVal_IR_counters
      step_counter #(
          .COUNT_WIDTH(ADDRESS_WIDTH),
          .STEP_WIDTH (REP_STEP_WIDTH)
      ) step_counter_IR (
          .clk(clk),
          .rst_n(rst_n),
          .enable(en_initVal_IR[i]),
          .init0(init0_initVal_IR[i]),
          .init(init_initVal_IR[i]),
          .init_value(initVal_IR[level_IR]),
          .step(regIR_step[NUMBER_IR*level_MT+i]),
          .count(initVal_IR[i])
      );
    end
  endgenerate
`endif

`ifdef INCLUDE_OR_STATES
  generate
    for (i = 0; i < NUMBER_OR; i++) begin : initVal_OR_counters
      step_counter #(
          .COUNT_WIDTH(ADDRESS_WIDTH),
          .STEP_WIDTH (REP_STEP_WIDTH)
      ) step_counter_OR (
          .clk(clk),
          .rst_n(rst_n),
          .enable(en_initVal_OR[i]),
          .init0(),
          .init(init_initVal_OR[i]),
          .init_value(initVal_OR[level_OR]),
          .step(regOR_step[i]),
          .count(initVal_OR[i])
      );
    end
  endgenerate
`endif

  ///////////////////////////////////////   controller
  controller #(
      .REP_ITER_WIDTH(REP_ITER_WIDTH),
      .REP_DELAY_WIDTH(REP_DELAY_WIDTH),
      .TRANS_DELAY_WIDTH(TRANS_DELAY_WIDTH),
      .NUMBER_OR(NUMBER_OR),
      .NUMBER_MT(NUMBER_MT),
      .NUMBER_IR(NUMBER_IR)
  ) controller_inst (
`ifdef INCLUDE_IR_STATES
      .regIR_iter(regIR_iter),
      .regIR_delay(regIR_delay),
      .regIR_config(regIR_config),
      .initIR_address(initIR_address),
      .level_IR(level_IR),
      .en_initVal_IR(en_initVal_IR),
      .init_initVal_IR(init_initVal_IR),
      .init0_initVal_IR(init0_initVal_IR),
`endif
`ifdef INCLUDE_MT_STATES
      .regMT_delay(regMT_delay),
      .regMT_config(regMT_config),
      .init0_address(init0_address),
`endif
      .level_MT(level_MT),
`ifdef INCLUDE_OR_STATES
      .regOR_iter(regOR_iter),
      .regOR_delay(regOR_delay),
      .regOR_config(regOR_config),
      .initOR_address(initOR_address),
      .level_OR(level_OR),
      .en_initVal_OR(en_initVal_OR),
      .init_initVal_OR(init_initVal_OR),
      .flag_OR(flag_OR),
`endif

      .en_address(en_address),

      .clk(clk),
      .rst_n(rst_n),
      .activation(activation),
      .address_valid(address_valid)
  );
endmodule
