`include "config.sv"

module controller #(
    parameter INSTR_OPCODE_BITWIDTH,

    parameter REP_ITER_WIDTH,
    parameter REP_DELAY_WIDTH,
    parameter TRANS_DELAY_WIDTH,

    parameter NUMBER_OR,    // OR: Outter R-Pattern
    parameter NUMBER_MT,    // MT: Middle T-Pattern
    parameter NUMBER_IR     // IR: Inner  R-Pattern
) (
    `ifdef INCLUDE_IR_STATES
    input  logic  [REP_ITER_WIDTH-1:0]       regIR_iter   [(NUMBER_MT+1)*NUMBER_IR],
    input  logic  [REP_DELAY_WIDTH-1:0]      regIR_delay  [(NUMBER_MT+1)*NUMBER_IR],
    input  logic                             regIR_config [(NUMBER_MT+1)*NUMBER_IR],
    output logic                             initIR_address,
    output logic [1:0]                       level_IR,
    output logic                             en_initVal_IR    [1:NUMBER_IR-1],
    output logic                             init_initVal_IR  [1:NUMBER_IR-1],
    output logic                             init0_initVal_IR [1:NUMBER_IR-1],
    `endif
    `ifdef INCLUDE_MT_STATES
    input  logic  [TRANS_DELAY_WIDTH-1:0]     regMT_delay  [NUMBER_MT],
    input  logic                              regMT_config [NUMBER_MT],
    output logic                             init0_address,
    `endif
    output logic [1:0]                       level_MT,
    `ifdef INCLUDE_OR_STATES
    input  logic  [REP_ITER_WIDTH-1:0]        regOR_iter   [NUMBER_OR],
    input  logic  [REP_DELAY_WIDTH-1:0]       regOR_delay  [NUMBER_OR],
    input  logic                              regOR_config [NUMBER_OR],
    output logic                             initOR_address,
    output logic [1:0]                       level_OR,
    output logic                             en_initVal_OR   [NUMBER_OR],
    output logic                             init_initVal_OR [NUMBER_OR],
    output logic                             flag_OR,
    `endif
    input  logic                             clk,
    input  logic                             rst_n,
    input  logic                             activation,
    input  logic                             instr_en,
    input  logic [INSTR_OPCODE_BITWIDTH-1:0] opcode,
    output logic                             en_address,
    output logic                             address_valid,
    output logic                             init0_config,
    output logic                             rep_valid,
    output logic                             repx_valid,
    output logic                             trans_valid
);

    `ifdef INCLUDE_IR_STATES
    logic en_iter_IR    [NUMBER_IR];
    logic init_iter_IR  [NUMBER_IR];
    logic co_iter_IR    [NUMBER_IR];
    logic en_delay_IR   [NUMBER_IR];
    logic init_delay_IR [NUMBER_IR];
    logic co_delay_IR   [NUMBER_IR];
    logic en_level_IR;
    logic [1:0] inVal_level_IR;
    `endif
    `ifdef INCLUDE_MT_STATES
    logic en_delay_MT   [NUMBER_MT];
    logic init_delay_MT [NUMBER_MT];
    logic co_delay_MT   [NUMBER_MT];
    logic en_level_MT;
    logic init0_level_MT;
    `endif
    `ifdef INCLUDE_OR_STATES
    logic en_iter_OR    [NUMBER_OR];
    logic init_iter_OR  [NUMBER_OR];
    logic co_iter_OR    [NUMBER_OR];
    logic en_delay_OR   [NUMBER_OR];
    logic init_delay_OR [NUMBER_OR];
    logic co_delay_OR   [NUMBER_OR];
    logic en_level_OR;
    logic [1:0] inVal_level_OR;
    logic en_flag_OR;
    logic flag_OR2;
    `endif

    typedef enum logic [`STATE_WIDTH-1:0] {
        `ifdef INCLUDE_IR_STATES
        GENR_IR0,
        WAIT_IR0,
        GENR_IR1,
        WAIT_IR1,
        `endif
        `ifdef INCLUDE_MT_STATES
        GENR_MT,
        WAIT_MT,
        `endif
        `ifdef INCLUDE_OR_STATES
        GENR_OR,
        WAIT_OR,
        `endif
        IDLE,
        CNFG,
        ACTV,
        GENR_Add0
    } state_t;
    state_t p_state, n_state;

    `include "tasks.sv"

    genvar i;
    `ifdef INCLUDE_IR_STATES
    down_counter #(
        .WIDTH(REP_ITER_WIDTH)
    ) counter_iter_IR_inst (
        .clk(clk),
        .rst_n(rst_n),
        .enable(en_iter_IR[0]),
        .init(init_iter_IR[0]),
        .init_value(regIR_iter[level_MT*NUMBER_IR+0]-1'b1),
        .co(co_iter_IR[0])
    );
    generate
        for (i = 1; i < NUMBER_IR; i++) begin : IR_counter_iter
            down_counter #(
                .WIDTH(REP_ITER_WIDTH)
            ) counter_iter_IR_inst (
                .clk(clk),
                .rst_n(rst_n),
                .enable(en_iter_IR[i]),
                .init(init_iter_IR[i]),
                .init_value(regIR_iter[level_MT*NUMBER_IR+i]),
                .co(co_iter_IR[i])
            );
        end
    endgenerate
    down_counter #(
        .WIDTH(REP_DELAY_WIDTH)
    ) counter_delay_IR_inst (
        .clk(clk),
        .rst_n(rst_n),
        .enable(en_delay_IR[0]),
        .init(init_delay_IR[0]),
        .init_value(flag_OR2 ? regIR_delay[0] : regIR_delay[level_MT*NUMBER_IR+0]),
        .co(co_delay_IR[0])
    );
    generate
        for (i = 1; i < NUMBER_IR; i++) begin : IR_counter_delay
            down_counter #(
                .WIDTH(REP_DELAY_WIDTH)
            ) counter_delay_IR_inst (
                .clk(clk),
                .rst_n(rst_n),
                .enable(en_delay_IR[i]),
                .init(init_delay_IR[i]),
                .init_value(regIR_delay[level_MT*NUMBER_IR+i]),
                .co(co_delay_IR[i])
            );
        end
    endgenerate
    register #(
        .WIDTH(2)
    ) register_level_IR (
        .clk(clk),
        .rst_n(rst_n),
        .enable(en_level_IR),
        .in_value(inVal_level_IR),
        .out_value(level_IR)
    );
    `endif

    `ifdef INCLUDE_MT_STATES
    generate
        for (i = 0; i < NUMBER_MT; i++) begin : MT_counter_delay
            down_counter #(
                .WIDTH(TRANS_DELAY_WIDTH)
            ) counter_delay_MT_inst (
                .clk(clk),
                .rst_n(rst_n),
                .enable(en_delay_MT[i]),
                .init(init_delay_MT[i]),
                .init_value(regMT_delay[i]),
                .co(co_delay_MT[i])
            );
        end
    endgenerate
    up_counter #(
        .WIDTH(2)
    ) counter_level_MT (
        .clk(clk),
        .rst_n(rst_n),
        .enable(en_level_MT),
        .init0(init0_level_MT),
        .count(level_MT)
    );
    `endif 
    
    `ifdef INCLUDE_OR_STATES
    generate
        for (i = 0; i < NUMBER_OR; i++) begin : OR_counter
            down_counter #(
                .WIDTH(REP_ITER_WIDTH)
            ) counter_iter_OR_inst (
                .clk(clk),
                .rst_n(rst_n),
                .enable(en_iter_OR[i]),
                .init(init_iter_OR[i]),
                .init_value(regOR_iter[i]),
                .co(co_iter_OR[i])
            );
            down_counter #(
                .WIDTH(REP_DELAY_WIDTH)
            ) counter_delay_OR_inst (
                .clk(clk),
                .rst_n(rst_n),
                .enable(en_delay_OR[i]),
                .init(init_delay_OR[i]),
                .init_value(regOR_delay[i]),
                .co(co_delay_OR[i])
            );
        end
    endgenerate
    register #(
        .WIDTH(2)
    ) register_level_OR (
        .clk(clk),
        .rst_n(rst_n),
        .enable(en_level_OR),
        .in_value(inVal_level_OR),
        .out_value(level_OR)
    );
    register #(
        .WIDTH(1)
    ) register_flag_OR (
        .clk(clk),
        .rst_n(rst_n),
        .enable(en_flag_OR),
        .in_value(1'b1),
        .out_value(flag_OR)
    );
    `endif


    always_ff @(posedge clk or negedge rst_n) begin : CONTROLLER_FF
        if (!rst_n) begin 
            p_state <= IDLE;
        end else begin
            p_state <= n_state;
        end
    end
    /////////////////////////////////////// FSM PART: INCLUDE_IR_STATES ///////////////////////////////////////
    `ifdef INCLUDE_IR_STATES

    `ifndef INCLUDE_MT_STATES
        assign level_MT = 1'b0;
    `endif

    always_comb begin : CONTROLLER_COMB
        n_state = IDLE;
        init0_config = 1'b0;
        rep_valid = 1'b0;
        repx_valid = 1'b0;
        trans_valid = 1'b0;
        en_address = 1'b0;
        address_valid = 1'b0;

        initIR_address = 1'b0;
        inVal_level_IR = 0;
        en_level_IR = 1'b0;
        for (int i = 0; i < NUMBER_IR; i++) begin
            init_iter_IR[i] = 1'b0;
            init_delay_IR[i] = 1'b0;
            en_iter_IR[i] = 1'b0;
            en_delay_IR[i] = 1'b0;
        end
        for (int i = 1; i < NUMBER_IR; i++) begin
            init0_initVal_IR[i] = 1'b0;
            init_initVal_IR[i] = 1'b0;
            en_initVal_IR[i] = 1'b0;
        end
        `ifdef INCLUDE_MT_STATES
        init0_level_MT = 1'b0;
        en_level_MT = 1'b0;
        init0_address = 1'b0;
        for (int i = 0; i < NUMBER_MT; i++) begin
            init_delay_MT[i] = 1'b0;
            en_delay_MT[i] = 1'b0;
        end
        `endif
        `ifdef INCLUDE_OR_STATES
        initOR_address = 1'b0;
        en_flag_OR = 1'b0;
        flag_OR2 = 1'b0;
        inVal_level_OR = 0;
        en_level_OR = 1'b0;
        for (int i = 0; i < NUMBER_OR; i++) begin
            init_iter_OR[i] = 1'b0;
            init_delay_OR[i] = 1'b0;
            en_iter_OR[i] = 1'b0;
            en_delay_OR[i] = 1'b0;
            init_initVal_OR[i] = 1'b0;
            en_initVal_OR[i] = 1'b0;
        end
        `endif

        case (p_state)
            IDLE: begin
                init0_config = 1'b1;
                if (instr_en) begin
                    n_state = CNFG;
                end else begin
                    n_state = IDLE;
                end
            end
            CNFG: begin
                if (opcode == 0) begin          
                    rep_valid = 1;
                end else if (opcode == 1) begin 
                    repx_valid = 1;
                end else if (opcode == 2) begin 
                    trans_valid = 1;
                end

                if (instr_en) begin
                    n_state = CNFG;
                end else begin
                    n_state = ACTV;
                    for (int i = 0; i < NUMBER_IR; i++) begin
                        init_iter_IR[i] = 1'b1;
                        init_delay_IR[i] = 1'b1;
                    end
                    `ifdef INCLUDE_MT_STATES
                    for (int i = 0; i < NUMBER_MT; i++) begin
                        init_delay_MT[i] = 1'b1;
                    end
                    `endif
                    `ifdef INCLUDE_OR_STATES
                    for (int i = 0; i < NUMBER_OR; i++) begin
                        init_iter_OR[i] = 1'b1;
                        init_delay_OR[i] = 1'b1;
                    end
                    `endif
                end
            end
            ACTV: begin
                if (!activation) begin 
                    n_state = ACTV;
                end else begin
                    n_state = GENR_Add0;
                end
            end

            GENR_Add0: begin
                address_valid = 1'b1;
                if (regIR_config[0]) begin
                    if (!co_delay_IR[0]) begin
                        en_delay_IR[0] = 1'b1;
                        n_state = WAIT_IR0;
                    end else begin
                        n_state = GENR_IR0;
                    end
                end  else begin
                    n_state = IDLE;
                end
            end

            GENR_IR0: begin
                address_valid = 1'b1;
                en_address = 1'b1;
                en_iter_IR[0] = 1'b1;
                evaluate_IR_state (
                    `ifdef INCLUDE_MT_STATES
                    .co_delay_MT(co_delay_MT),
                    .en_delay_MT(en_delay_MT),
                    .en_level_MT(en_level_MT),
                    .init0_level_MT(init0_level_MT),
                    `endif
                    .level_MT(level_MT),
                    `ifdef INCLUDE_OR_STATES
                    .co_iter_OR(co_iter_OR),
                    .co_delay_OR(co_delay_OR),
                    .regOR_config(regOR_config),
                    .en_level_OR(en_level_OR),
                    .inVal_level_OR(inVal_level_OR),
                    .en_initVal_OR(en_initVal_OR),
                    .en_delay_OR(en_delay_OR),
                    .en_flag_OR(en_flag_OR),
                    .flag_OR2(flag_OR2),
                    `endif
                    .co_iter_IR(co_iter_IR),
                    .co_delay_IR(co_delay_IR),
                    .regIR_config(regIR_config),
                    .en_level_IR(en_level_IR),
                    .inVal_level_IR(inVal_level_IR),
                    .en_initVal_IR(en_initVal_IR),
                    .en_delay_IR(en_delay_IR),
                    .init_delay_IR(init_delay_IR),
                    .n_state(n_state)
                );
            end
            WAIT_IR0: begin
                en_delay_IR[0] = 1'b1;
                if (!co_delay_IR[0]) begin
                    n_state = WAIT_IR0;
                end else begin
                    init_delay_IR[0] = 1'b1;
                    n_state = GENR_IR0;
                end
            end

            GENR_IR1: begin
                address_valid = 1'b1;
                initIR_address = 1'b1;
                en_iter_IR[level_IR] = 1'b1;
                en_level_IR = 1'b1;
                inVal_level_IR = 0;
                for (int i = 0; i < level_IR; i++) begin
                    // init_delay_IR[i] = 1'b1;
                    init_iter_IR[i] = 1'b1;
                end
                for (int i = 1; i < level_IR; i++) begin
                    init_initVal_IR[i] = 1'b1;
                end
                if (!co_delay_IR[0]) begin
                    en_delay_IR[0] = 1'b1;
                    n_state = WAIT_IR0;
                end else begin
                    n_state = GENR_IR0;
                end
            end
            WAIT_IR1: begin
                en_delay_IR[level_IR] = 1'b1;
                if (!co_delay_IR[level_IR]) begin
                    n_state = WAIT_IR1;
                end else begin
                    init_delay_IR[level_IR] = 1'b1;
                    n_state = GENR_IR1;
                end
            end

/////////////////////////////////// T-level ///////////////////////////////////    
            `ifdef INCLUDE_MT_STATES
            GENR_MT: begin
                address_valid = 1'b1;
                init0_address = 1'b1;
                en_level_IR = 1'b1;
                inVal_level_IR = 0;
                for (int i = 1; i < NUMBER_IR; i++) begin
                    init_delay_IR[i] = 1'b1;
                end
                for (int i = 0; i < NUMBER_IR; i++) begin
                    init_iter_IR[i] = 1'b1;
                end
                for (int i = 1; i < NUMBER_IR; i++) begin
                    init0_initVal_IR[i] = 1'b1;
                end
                if (!co_delay_IR[0]) begin
                    en_delay_IR[0] = 1'b1;
                    n_state = WAIT_IR0;
                end else begin
                    n_state = GENR_IR0;
                end
            end
            WAIT_MT: begin
                en_delay_MT[level_MT-1] = 1'b1;
                if (!co_delay_MT[level_MT-1]) begin
                    n_state = WAIT_MT;
                end else begin
                    init_delay_MT[level_MT-1] = 1'b1;
                    init_delay_IR[0] = 1'b1;
                    n_state = GENR_MT;
                end
            end
            `endif
/////////////////////////////////// OR-level ///////////////////////////////////    
            `ifdef INCLUDE_OR_STATES
            GENR_OR: begin
                address_valid = 1'b1;
                initOR_address = 1'b1;
                en_iter_OR[level_OR] = 1'b1;
                // flag_OR = 1'b1;
                en_level_IR = 1'b1;
                inVal_level_IR = 0;
                for (int i = 1; i < NUMBER_IR; i++) begin
                    init_delay_IR[i] = 1'b1;
                end
                for (int i = 0; i < NUMBER_IR; i++) begin
                    init_iter_IR[i] = 1'b1;
                end
                `ifdef INCLUDE_MT_STATES
                for (int i = 0; i < NUMBER_MT; i++) begin
                    init_delay_MT[i] = 1'b1;
                end
                `endif
                for (int i = 0; i < level_OR; i++) begin
                    init_delay_OR[i] = 1'b1;
                    init_iter_OR[i] = 1'b1;
                end
                for (int i = 1; i < NUMBER_IR; i++) begin
                    init0_initVal_IR[i] = 1'b1;
                end
                for (int i = 0; i < level_OR; i++) begin
                    init_initVal_OR[i] = 1'b1;
                end
                if (!co_delay_IR[0]) begin
                    en_delay_IR[0] = 1'b1;
                    n_state = WAIT_IR0;
                end else begin
                    n_state = GENR_IR0;
                end
            end
            WAIT_OR: begin
                en_delay_OR[level_OR] = 1'b1;
                if (!co_delay_OR[level_OR]) begin
                    n_state = WAIT_OR;
                end else if (co_delay_OR[level_OR]) begin
                    init_delay_OR[level_OR] = 1'b1;
                    init_delay_IR[0] = 1'b1;
                    n_state = GENR_OR;
                end
            end
            `endif
            default: begin
                n_state = IDLE;
            end
        endcase
    end

    // `elsif !INCLUDE_IR_STATES
    `else
////////////////////////////////////////////////////////////////////////////////
    always_comb begin : CONTROLLER_COMB
        n_state = IDLE;
        init0_config = 1'b0;
        rep_valid = 1'b0;
        repx_valid = 1'b0;
        trans_valid = 1'b0;
        en_address = 1'b0;
        address_valid = 1'b0;
        
        `ifdef INCLUDE_MT_STATES
        init0_level_MT = 1'b0;
        en_level_MT = 1'b0;
        for (int i = 0; i < NUMBER_MT; i++) begin
            init_delay_MT[i] = 1'b0;
            en_delay_MT[i] = 1'b0;
        end
        `endif
        `ifdef INCLUDE_OR_STATES
        initOR_address = 1'b0;
        inVal_level_OR = 0;
        en_level_OR = 1'b0;
        for (int i = 0; i < NUMBER_OR; i++) begin
            init_iter_OR[i] = 1'b0;
            init_delay_OR[i] = 1'b0;
            en_iter_OR[i] = 1'b0;
            en_delay_OR[i] = 1'b0;
            init_initVal_OR[i] = 1'b0;
            en_initVal_OR[i] = 1'b0;
        end
        `endif

        case (p_state)
            IDLE: begin
                init0_config = 1'b1;
                if (instr_en) begin
                    n_state = CNFG;
                end else begin
                    n_state = IDLE;
                end
            end
            CNFG: begin
                if (opcode == 0) begin
                    rep_valid = 1;
                end else if (opcode == 1) begin 
                    repx_valid = 1;
                end else if (opcode == 2) begin 
                    trans_valid = 1;
                end

                if (instr_en) begin
                    n_state = CNFG;
                end else begin
                    n_state = ACTV;
                    `ifdef INCLUDE_MT_STATES
                    for (int i = 0; i < NUMBER_MT; i++) begin
                        init_delay_MT[i] = 1'b1;
                    end
                    `endif
                    `ifdef INCLUDE_OR_STATES
                    for (int i = 0; i < NUMBER_OR; i++) begin
                        init_iter_OR[i] = 1'b1;
                        init_delay_OR[i] = 1'b1;
                    end
                    `endif
                end
            end
            ACTV: begin
                if (!activation) begin 
                    n_state = ACTV;
                end else begin
                    n_state = GENR_Add0;
                end
            end

            GENR_Add0: begin
                address_valid = 1'b1;
                `ifdef INCLUDE_MT_STATES
                if (regMT_config[level_MT]) begin
                    evaluate_MT_state(level_MT, co_delay_MT, en_delay_MT, en_level_MT, n_state);
                end else begin
                    n_state = IDLE;
                end
                `else `ifdef INCLUDE_OR_STATES
                if (regOR_config[0]) begin
                    if (!co_iter_OR[0] && !co_delay_OR[0]) begin
                        en_level_OR = 1'b1;
                        inVal_level_OR = 0;
                        en_initVal_OR[0] = 1'b1;
                        en_delay_OR[0] = 1'b1;
                        n_state = WAIT_OR;
                    end else if (!co_iter_OR[0] && co_delay_OR[0]) begin
                        en_level_OR = 1'b1;
                        inVal_level_OR = 0;
                        en_initVal_OR[0] = 1'b1;
                        n_state = GENR_OR;
                    end else begin
                        n_state = IDLE;
                    end
                end else begin
                    n_state = IDLE;
                end
                `endif 
                `endif
            end

/////////////////////////////////// T-level ///////////////////////////////////
            `ifdef INCLUDE_MT_STATES
            GENR_MT: begin
                address_valid = 1'b1;
                en_address = 1'b1;
                if (regMT_config[level_MT]) begin
                    evaluate_MT_state(level_MT, co_delay_MT, en_delay_MT, en_level_MT, n_state);
                `ifdef INCLUDE_OR_STATES
                end else if (regOR_config[0]) begin
                    init0_level_MT = 1'b1;
                    evaluate_OR_state (
                        .co_iter_OR(co_iter_OR),
                        .co_delay_OR(co_delay_OR),
                        .regOR_config(regOR_config),
                        .en_level_OR(en_level_OR),
                        .inVal_level_OR(inVal_level_OR),
                        .en_initVal_OR(en_initVal_OR),
                        .en_delay_OR(en_delay_OR),
                        .n_state(n_state)
                    );
                `endif
                end else begin
                    n_state = ACTV;
                end
            end
            WAIT_MT: begin
                en_delay_MT[level_MT-1] = 1'b1;
                if (!co_delay_MT[level_MT-1]) begin
                    n_state = WAIT_MT;
                end else begin
                    init_delay_MT[level_MT-1] = 1'b1;
                    n_state = GENR_MT;
                end
            end
            `endif
/////////////////////////////////// OR-level ///////////////////////////////////    
            `ifdef INCLUDE_OR_STATES
            GENR_OR: begin
                address_valid = 1'b1;
                initOR_address = 1'b1;
                en_iter_OR[level_OR] = 1'b1;
                for (int i = 0; i < level_OR; i++) begin
                    init_delay_OR[i] = 1'b1;
                    init_iter_OR[i] = 1'b1;
                    init_initVal_OR[i] = 1'b1;
                end
                `ifdef INCLUDE_MT_STATES
                evaluate_MT_state(level_MT, co_delay_MT, en_delay_MT, en_level_MT, n_state);
                `else
                if (regOR_config[0]) begin
                    evaluate_OR_state (
                        .co_iter_OR(co_iter_OR),
                        .co_delay_OR(co_delay_OR),
                        .regOR_config(regOR_config),
                        .en_level_OR(en_level_OR),
                        .inVal_level_OR(inVal_level_OR),
                        .en_initVal_OR(en_initVal_OR),
                        .en_delay_OR(en_delay_OR),
                        .n_state(n_state)
                    );
                end else begin
                    n_state = ACTV;
                end
                `endif
            end
            WAIT_OR: begin
                en_delay_OR[level_OR] = 1'b1;
                if (!co_delay_OR[level_OR]) begin
                    n_state = WAIT_OR;
                end else if (co_delay_OR[level_OR]) begin
                    init_delay_OR[level_OR] = 1'b1;
                    n_state = GENR_OR;
                end
            end
            `endif
            default: begin
                n_state = IDLE;
            end
        endcase
    end
    `endif

endmodule