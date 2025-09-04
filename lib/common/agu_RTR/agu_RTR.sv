`include "config.sv"

module agu_RTR #(

    parameter RESOURCE_INSTR_WIDTH,
    parameter INSTR_OPCODE_BITWIDTH,
    parameter ADDRESS_WIDTH,

    parameter REP_ITER_WIDTH,
    parameter REP_DELAY_WIDTH,
    parameter REP_STEP_WIDTH,
    parameter REP_LEVEL_WIDTH,
    parameter REP_OPTION_WIDTH,
    parameter TRANS_DELAY_WIDTH,
    parameter TRANS_LEVEL_WIDTH,

    parameter NUMBER_OR,    // OR: Outter R-Pattern
    parameter NUMBER_MT,    // MT: Middle T-Pattern
    parameter NUMBER_IR     // IR: Inner  R-Pattern
) (

    input  logic clk,
    input  logic rst_n,
    input  logic activation,
    input  logic instr_en,
    input  logic [RESOURCE_INSTR_WIDTH-1:0] instr,
    output logic address_valid,
    output logic [ADDRESS_WIDTH-1:0] address
);
    import agu_RTR_pkg::*;
    logic [INSTR_OPCODE_BITWIDTH-1:0] opcode;
    logic [RESOURCE_INSTR_WIDTH-INSTR_OPCODE_BITWIDTH-1:0] payload;

    assign opcode  = instr[RESOURCE_INSTR_WIDTH-1:RESOURCE_INSTR_WIDTH-INSTR_OPCODE_BITWIDTH];
    assign payload = instr[RESOURCE_INSTR_WIDTH-INSTR_OPCODE_BITWIDTH-1:0];

    logic rep_valid;
    logic repx_valid;
    logic trans_valid;
    logic en_address;
    logic init0_address;
    logic initIR_address;
    logic [1:0] level_IR;
    logic [ADDRESS_WIDTH-1:0] initVal_IR [NUMBER_IR-1:1];
    logic en_initVal_IR [NUMBER_IR-1:1];
    logic init_initVal_IR [NUMBER_IR-1:1];
    logic init0_initVal_IR [NUMBER_IR-1:1];

    rep_t rep;
    repx_t repx;
    trans_t trans;
    assign rep   = rep_valid   ? unpack_rep(payload)   : '{default: 0};
    assign repx  = repx_valid  ? unpack_repx(payload)  : '{default: 0};
    assign trans = trans_valid ? unpack_trans(payload) : '{default: 0};
    
    logic [REP_ITER_WIDTH-1:0]    regIR_iter   [(NUMBER_MT+1)*NUMBER_IR-1:0];
    logic [REP_DELAY_WIDTH-1:0]   regIR_delay  [(NUMBER_MT+1)*NUMBER_IR-1:0];
    logic [REP_STEP_WIDTH-1:0]    regIR_step   [(NUMBER_MT+1)*NUMBER_IR-1:0];
    logic                         regIR_config [(NUMBER_MT+1)*NUMBER_IR-1:0];

    logic [1:0] level_MT;
    
    `ifdef INCLUDE_MT_STATES
    logic [TRANS_DELAY_WIDTH-1:0] regMT_delay  [NUMBER_MT-1:0];
    logic                         regMT_config [NUMBER_MT-1:0];
    `endif

    `ifdef INCLUDE_OR_STATES
    logic initOR_address;
    logic [1:0] level_OR;
    logic [ADDRESS_WIDTH-1:0] initVal_OR [NUMBER_OR-1:0];
    logic en_initVal_OR [NUMBER_OR-1:0];
    logic init_initVal_OR [NUMBER_OR-1:0];
    logic flag_OR;
    logic [REP_ITER_WIDTH-1:0]    regOR_iter   [NUMBER_OR-1:0];
    logic [REP_DELAY_WIDTH-1:0]   regOR_delay  [NUMBER_OR-1:0];
    logic [REP_STEP_WIDTH-1:0]    regOR_step   [NUMBER_OR-1:0];
    logic                         regOR_config [NUMBER_OR-1:0];
    `endif

    /////////////////////////////////////// Config registers /////////////////////////////////////// 
    genvar i;
    for (i = 0; i < (NUMBER_MT+1)*NUMBER_IR; i++) begin : IR_registers
        register #(
            .WIDTH(REP_ITER_WIDTH)
        ) regIR_iter_inst (
            .clk(clk),
            .rst_n(rst_n),
            .enable(rep_valid && (i == rep._option*NUMBER_IR+rep._level) && (rep._level < NUMBER_IR)),
            .in_value(rep._iter),
            .out_value(regIR_iter[i])
        );

        register #(
            .WIDTH(REP_DELAY_WIDTH)
        ) regIR_delay_inst (
            .clk(clk),
            .rst_n(rst_n),
            .enable(rep_valid && (i == rep._option*NUMBER_IR+rep._level) && (rep._level < NUMBER_IR)),
            .in_value(rep._delay),
            .out_value(regIR_delay[i])
        );

        register #(
            .WIDTH(REP_STEP_WIDTH)
        ) regIR_step_inst (
            .clk(clk),
            .rst_n(rst_n),
            .enable(rep_valid && (i == rep._option*NUMBER_IR+rep._level) && (rep._level < NUMBER_IR)),
            .in_value(rep._step),
            .out_value(regIR_step[i])
        );

        register #(
            .WIDTH(1'b1)
        ) regIR_config_inst (
            .clk(clk),
            .rst_n(rst_n),
            .enable(rep_valid && (i == rep._option*NUMBER_IR+rep._level) && (rep._level < NUMBER_IR)),
            .in_value(1'b1),
            .out_value(regIR_config[i])
        );
    end

    `ifdef INCLUDE_MT_STATES
    for (i = 0; i < NUMBER_MT; i++) begin : MT_registers
        register #(
            .WIDTH(TRANS_DELAY_WIDTH)
        ) regMT_delay_inst (
            .clk(clk),
            .rst_n(rst_n),
            .enable(trans_valid && (i == trans._level)),
            .in_value(trans._delay),
            .out_value(regMT_delay[i])
        );
        register #(
            .WIDTH(1'b1)
        ) regMT_config_inst (
            .clk(clk),
            .rst_n(rst_n),
            .enable(trans_valid && (i == trans._level)),
            .in_value(1'b1),
            .out_value(regMT_config[i])
        );
    end
    `endif

    `ifdef INCLUDE_OR_STATES
    for (i = 0; i < NUMBER_OR; i++) begin : OR_registers
        register #(
            .WIDTH(REP_ITER_WIDTH)
        ) regOR_iter_inst (
            .clk(clk),
            .rst_n(rst_n),
            .enable(rep_valid && (i == rep._level-NUMBER_IR) && (rep._level >= NUMBER_IR)),
            .in_value(rep._iter),
            .out_value(regOR_iter[i])
        );

        register #(
            .WIDTH(REP_DELAY_WIDTH)
        ) regOR_delay_inst (
            .clk(clk),
            .rst_n(rst_n),
            .enable(rep_valid && (i == rep._level-NUMBER_IR) && (rep._level >= NUMBER_IR)),
            .in_value(rep._delay),
            .out_value(regOR_delay[i])
        );

        register #(
            .WIDTH(REP_STEP_WIDTH)
        ) regOR_step_inst (
            .clk(clk),
            .rst_n(rst_n),
            .enable(rep_valid && (i == rep._level-NUMBER_IR) && (rep._level >= NUMBER_IR)),
            .in_value(rep._step),
            .out_value(regOR_step[i])
        );

        register #(
            .WIDTH(1'b1)
        ) regOR_config_inst (
            .clk(clk),
            .rst_n(rst_n),
            .enable(rep_valid && (i == rep._level-NUMBER_IR) && (rep._level >= NUMBER_IR)),
            .in_value(1'b1),
            .out_value(regOR_config[i])
        );
    end
    `endif

    /////////////////////////////////////// Counters ///////////////////////////////////////
    `ifdef INCLUDE_IR_STATES
    `ifdef INCLUDE_OR_STATES
    always @(posedge clk, negedge rst_n) begin : Address_Counter
        if (!rst_n) begin
            address <= 0;
        end else if (init0_address) begin
            address <= !flag_OR ? 0 : initVal_OR[level_OR];
        end else if (initIR_address) begin
            address <= !flag_OR ? initVal_IR[level_IR] : initVal_IR[level_IR] + initVal_OR[level_OR];
        end else if (initOR_address) begin
            address <= initVal_OR[level_OR];
        end else if (en_address) begin
            address <= address + regIR_step[NUMBER_IR*level_MT+0];
        end
    end
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

    for (i = 1; i < NUMBER_IR; i++) begin : initVal_IR_counters
        step_counter #(
            .COUNT_WIDTH(ADDRESS_WIDTH),
            .STEP_WIDTH(REP_STEP_WIDTH)
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
    `else
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
    `ifdef INCLUDE_OR_STATES
    for (i = 0; i < NUMBER_OR; i++) begin : initVal_OR_counters
        step_counter #(
            .COUNT_WIDTH(ADDRESS_WIDTH),
            .STEP_WIDTH(REP_STEP_WIDTH)
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
    `endif
///////////////////////////////////////   controller

    controller #(
        .INSTR_OPCODE_BITWIDTH(INSTR_OPCODE_BITWIDTH),
        .ADDRESS_WIDTH(ADDRESS_WIDTH),

        .REP_ITER_WIDTH(REP_ITER_WIDTH),
        .REP_DELAY_WIDTH(REP_DELAY_WIDTH),
        .REP_STEP_WIDTH(REP_STEP_WIDTH),
        .REP_LEVEL_WIDTH(REP_LEVEL_WIDTH),
        .REP_OPTION_WIDTH(REP_OPTION_WIDTH),
        .TRANS_DELAY_WIDTH(TRANS_DELAY_WIDTH),
        .TRANS_LEVEL_WIDTH(TRANS_LEVEL_WIDTH),

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
        .level_MT(level_MT),
        `ifdef INCLUDE_MT_STATES
        .regMT_delay(regMT_delay),
        .regMT_config(regMT_config),
        .init0_address(init0_address),
        `endif
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
        
        .clk(clk),
        .rst_n(rst_n),
        .activation(activation),
        .instr_en(instr_en),
        .opcode(opcode),
        .en_address(en_address),
        .address_valid(address_valid),
        .rep_valid(rep_valid),
        .repx_valid(repx_valid),
        .trans_valid(trans_valid)
    );
endmodule