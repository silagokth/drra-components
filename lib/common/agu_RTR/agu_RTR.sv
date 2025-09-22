`include "config.sv"

module agu_RTR #(
    parameter ADDRESS_WIDTH,

    parameter REP_LEVEL_WIDTH,
    parameter REP_DELAY_WIDTH,
    parameter REP_ITER_WIDTH,
    parameter REP_STEP_WIDTH,
    parameter TRANS_LEVEL_WIDTH,
    parameter TRANS_DELAY_WIDTH,

    parameter NUMBER_OR,
    parameter NUMBER_MT,
    parameter NUMBER_IR 
) (

    input  logic                         clk,
    input  logic                         rst_n,
    input  logic                         activation,

    input  logic                         rep_valid,
    input  logic                         repx_valid,
    input  logic                         trans_valid,
    input  logic [REP_LEVEL_WIDTH-1:0]   rep_level,
    input  logic [REP_DELAY_WIDTH-1:0]   rep_delay,
    input  logic [REP_ITER_WIDTH-1:0]    rep_iter,
    input  logic [REP_STEP_WIDTH-1:0]    rep_step,
    input  logic [TRANS_LEVEL_WIDTH-1:0] trans_level,
    input  logic [TRANS_DELAY_WIDTH-1:0] trans_delay,

    output logic                         address_valid,
    output logic [ADDRESS_WIDTH-1:0]     address
);
    logic                                init0_config;
    logic                                en_address;

    `ifdef INCLUDE_IR_STATES
    logic [REP_ITER_WIDTH-1:0]           regIR_iter       [(NUMBER_MT+1)*NUMBER_IR];
    logic [REP_DELAY_WIDTH-1:0]          regIR_delay      [(NUMBER_MT+1)*NUMBER_IR];
    logic [REP_STEP_WIDTH-1:0]           regIR_step       [(NUMBER_MT+1)*NUMBER_IR];
    logic                                regIR_config     [(NUMBER_MT+1)*NUMBER_IR];
    logic                                IRbarOR;
    logic                                init0_config_IR;
    logic                                en_config_IR;
    logic [$clog2(NUMBER_IR)-1:0]        config_IR; 
    logic                                en_decoder_config_IR;
    logic [$clog2(NUMBER_IR*(NUMBER_MT+1))-1:0] in_decoder_config_IR;
    logic [2**$clog2(NUMBER_IR*(NUMBER_MT+1))-1:0] decoded_config_IR;
    logic                                initIR_address;
    logic [$clog2(NUMBER_IR)-1:0]        level_IR;
    logic [ADDRESS_WIDTH-1:0]            initVal_IR       [1:NUMBER_IR-1];
    logic                                en_initVal_IR    [1:NUMBER_IR-1];
    logic                                init_initVal_IR  [1:NUMBER_IR-1];
    logic                                init0_initVal_IR [1:NUMBER_IR-1];
    `endif

    `ifdef INCLUDE_MT_STATES
    logic [TRANS_DELAY_WIDTH-1:0]        regMT_delay       [NUMBER_MT];
    logic                                regMT_config      [NUMBER_MT];
    logic                                en_decoder_config_MT;
    logic [2**$clog2(NUMBER_MT)-1:0]     en_config_MT;
    logic                                init0_address;
    `endif  

    logic [$clog2(NUMBER_MT+1)-1:0]      level_MT;

    `ifdef INCLUDE_OR_STATES
    logic [REP_ITER_WIDTH-1:0]           regOR_iter        [NUMBER_OR];
    logic [REP_DELAY_WIDTH-1:0]          regOR_delay       [NUMBER_OR];
    logic [REP_STEP_WIDTH-1:0]           regOR_step        [NUMBER_OR];
    logic                                regOR_config      [NUMBER_OR];
    logic                                en_config_OR;
    logic [$clog2(NUMBER_OR)-1:0]        config_OR; 
    logic                                en_decoder_config_OR;
    logic [2**$clog2(NUMBER_OR)-1:0]     decoded_config_OR;
    logic                                initOR_address;
    logic [$clog2(NUMBER_OR)-1:0]        level_OR;
    logic [ADDRESS_WIDTH-1:0]            initVal_OR        [NUMBER_OR];
    logic                                en_initVal_OR     [NUMBER_OR];
    logic                                init_initVal_OR   [NUMBER_OR];
    logic                                flag_OR;
    `endif

    /////////////////////////////////////// Config registers /////////////////////////////////////// 

    genvar i;
    `ifdef INCLUDE_IR_STATES
    up_counter #(
        .WIDTH($clog2(NUMBER_IR))
        ) counter_config_IR (
            .clk(clk),
            .rst_n(rst_n),
            .enable(en_config_IR),
            .init0(init0_config_IR),
            .count(config_IR)
    );

    assign IRbarOR = rep_level < NUMBER_IR[REP_LEVEL_WIDTH-1:0] ? 1'b1 : 1'b0;

    assign in_decoder_config_IR = NUMBER_IR * level_MT + config_IR;
    decoder #(
        .WIDTH($clog2(NUMBER_IR*(NUMBER_MT+1)))
        ) decoder_IR_option (
            .enable(en_decoder_config_IR),
            .in_data(in_decoder_config_IR),
            .out_data(decoded_config_IR)
    );

    generate
        for (i = 0; i < (NUMBER_MT+1)*NUMBER_IR; i++) begin : IR_registers
            register #(
                .WIDTH(REP_ITER_WIDTH)
            ) regIR_iter_inst (
                .clk(clk),
                .rst_n(rst_n),
                .init0(init0_config),
                .enable(decoded_config_IR[i]),
                .in_value(rep_iter),
                .out_value(regIR_iter[i])
            );

            register #(
                .WIDTH(REP_DELAY_WIDTH)
            ) regIR_delay_inst (
                .clk(clk),
                .rst_n(rst_n),
                .init0(init0_config),
                .enable(decoded_config_IR[i]),
                .in_value(rep_delay),
                .out_value(regIR_delay[i])
            );

            register #(
                .WIDTH(REP_STEP_WIDTH)
            ) regIR_step_inst (
                .clk(clk),
                .rst_n(rst_n),
                .init0(init0_config),
                .enable(decoded_config_IR[i]),
                .in_value(rep_step),
                .out_value(regIR_step[i])
            );

            register #(
                .WIDTH(1'b1)
            ) regIR_config_inst (
                .clk(clk),
                .rst_n(rst_n),
                .init0(init0_config),
                .enable(decoded_config_IR[i]),
                .in_value(1'b1),
                .out_value(regIR_config[i])
            );
        end
    endgenerate
    `endif

    `ifdef INCLUDE_MT_STATES
    decoder #(
        .WIDTH($clog2(NUMBER_MT))
        ) decoder_MT_option (
            .enable(en_decoder_config_MT),
            .in_data(level_MT[$clog2(NUMBER_MT)-1:0]),
            .out_data(en_config_MT)
    );

    generate
        for (i = 0; i < NUMBER_MT; i++) begin : MT_registers
            register #(
                .WIDTH(TRANS_DELAY_WIDTH)
            ) regMT_delay_inst (
                .clk(clk),
                .rst_n(rst_n),
                .init0(init0_config),
                .enable(en_config_MT[i]),
                .in_value(trans_delay),
                .out_value(regMT_delay[i])
            );

            register #(
                .WIDTH(1'b1)
            ) regMT_config_inst (
                .clk(clk),
                .rst_n(rst_n),
                .init0(init0_config),
                .enable(en_config_MT[i]),
                .in_value(1'b1),
                .out_value(regMT_config[i])
            );
        end
    endgenerate
    `endif

    `ifdef INCLUDE_OR_STATES
    up_counter #(
        .WIDTH($clog2(NUMBER_OR))
        ) counter_config_OR (
            .clk(clk),
            .rst_n(rst_n),
            .enable(en_config_OR),
            .init0(),
            .count(config_OR)
    );

    decoder #(
        .WIDTH($clog2(NUMBER_OR))
        ) decoder_OR_option (
            .enable(en_decoder_config_OR),
            .in_data(config_OR),
            .out_data(decoded_config_OR)
    );

    generate
        for (i = 0; i < NUMBER_OR; i++) begin : OR_registers
            register #(
                .WIDTH(REP_ITER_WIDTH)
            ) regOR_iter_inst (
                .clk(clk),
                .rst_n(rst_n),
                .init0(init0_config),
                .enable(decoded_config_OR[i]),
                .in_value(rep_iter),
                .out_value(regOR_iter[i])
            );

            register #(
                .WIDTH(REP_DELAY_WIDTH)
            ) regOR_delay_inst (
                .clk(clk),
                .rst_n(rst_n),
                .init0(init0_config),
                .enable(decoded_config_OR[i]),
                .in_value(rep_delay),
                .out_value(regOR_delay[i])
            );

            register #(
                .WIDTH(REP_STEP_WIDTH)
            ) regOR_step_inst (
                .clk(clk),
                .rst_n(rst_n),
                .init0(init0_config),
                .enable(decoded_config_OR[i]),
                .in_value(rep_step),
                .out_value(regOR_step[i])
            );

            register #(
                .WIDTH(1'b1)
            ) regOR_config_inst (
                .clk(clk),
                .rst_n(rst_n),
                .init0(init0_config),
                .enable(decoded_config_OR[i]),
                .in_value(1'b1),
                .out_value(regOR_config[i])
            );
        end
    endgenerate
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
    endgenerate
    `endif

    `ifdef INCLUDE_OR_STATES
    generate
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
        .IRbarOR(IRbarOR),
        .init0_config_IR(init0_config_IR),
        .en_config_IR(en_config_IR),
        .en_decoder_config_IR(en_decoder_config_IR),
        .initIR_address(initIR_address),
        .level_IR(level_IR),
        .en_initVal_IR(en_initVal_IR),
        .init_initVal_IR(init_initVal_IR),
        .init0_initVal_IR(init0_initVal_IR),
        `endif
        `ifdef INCLUDE_MT_STATES
        .regMT_delay(regMT_delay),
        .regMT_config(regMT_config),
        .en_decoder_config_MT(en_decoder_config_MT),
        .init0_address(init0_address),
        `endif
        .level_MT(level_MT),
        `ifdef INCLUDE_OR_STATES
        .regOR_iter(regOR_iter),
        .regOR_delay(regOR_delay),
        .regOR_config(regOR_config),
        .en_config_OR(en_config_OR),
        .en_decoder_config_OR(en_decoder_config_OR),
        .initOR_address(initOR_address),
        .level_OR(level_OR),
        .en_initVal_OR(en_initVal_OR),
        .init_initVal_OR(init_initVal_OR),
        .flag_OR(flag_OR),
        `endif
        
        .init0_config(init0_config),
        .en_address(en_address),

        .clk(clk),
        .rst_n(rst_n),
        .activation(activation),
        .rep_valid(rep_valid),
        .repx_valid(repx_valid),
        .trans_valid(trans_valid),
        .address_valid(address_valid)
    );
endmodule