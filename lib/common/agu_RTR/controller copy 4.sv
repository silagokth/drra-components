module controller #(
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
    input  logic [INSTR_OPCODE_BITWIDTH-1:0] opcode,
    input  wire [REP_ITER_WIDTH-1:0]    regIR_iter   [(NUMBER_MT+1)*NUMBER_IR-1:0],
    input  wire [REP_DELAY_WIDTH-1:0]   regIR_delay  [(NUMBER_MT+1)*NUMBER_IR-1:0],
    input  wire                         regIR_config [(NUMBER_MT+1)*NUMBER_IR-1:0],
    input  wire [TRANS_DELAY_WIDTH-1:0] regMT_delay  [NUMBER_MT-1:0],
    input  wire                         regMT_config [NUMBER_MT-1:0],
    input  wire [REP_ITER_WIDTH-1:0]    regOR_iter   [NUMBER_OR-1:0],
    input  wire [REP_DELAY_WIDTH-1:0]   regOR_delay  [NUMBER_OR-1:0],
    input  wire                         regOR_config [NUMBER_OR-1:0],
    output logic rep_valid,
    output logic repx_valid,
    output logic trans_valid,
    output logic en_address,
    output logic init0_address,
    output logic initIR_address,
    output logic initOR_address,
    output logic address_valid,
    output logic [1:0] level_IR,
    output logic [1:0] level_MT,
    output logic [1:0] level_OR,
    output logic en_initVal_IR [NUMBER_IR-1:1],
    output logic init_initVal_IR [NUMBER_IR-1:1],
    output logic init0_initVal_IR [NUMBER_IR-1:1],
    output logic en_initVal_OR [NUMBER_OR-1:0],
    output logic init_initVal_OR [NUMBER_OR-1:0],
    output logic flag_OR
);

    // assign rep_valid   = instr_en && (opcode == 0);
    // assign repx_valid  = instr_en && (opcode == 1);
    // assign trans_valid = instr_en && (opcode == 2);

    ////////////////////////// counters
    
    logic [REP_ITER_WIDTH-1:0]  count_iter_IR      [NUMBER_IR-1:0];
    logic [REP_ITER_WIDTH-1:0]  init_value_iter_IR [NUMBER_IR-1:0];
    logic en_iter_IR    [NUMBER_IR-1:0];
    logic init_iter_IR  [NUMBER_IR-1:0];
    logic co_iter_IR    [NUMBER_IR-1:0];

    logic [REP_DELAY_WIDTH-1:0] count_delay_IR      [NUMBER_IR-1:0];
    logic [REP_DELAY_WIDTH-1:0] init_value_delay_IR [NUMBER_IR-1:0];
    logic en_delay_IR   [NUMBER_IR-1:0];
    logic init_delay_IR [NUMBER_IR-1:0];
    logic co_delay_IR   [NUMBER_IR-1:0];

    logic [TRANS_DELAY_WIDTH-1:0] count_delay_MT      [NUMBER_MT-1:0];
    logic [TRANS_DELAY_WIDTH-1:0] init_value_delay_MT [NUMBER_MT-1:0];
    logic en_delay_MT   [NUMBER_MT-1:0];
    logic init_delay_MT [NUMBER_MT-1:0];
    logic co_delay_MT   [NUMBER_MT-1:0];

    logic [REP_ITER_WIDTH-1:0]  count_iter_OR      [NUMBER_OR-1:0];
    logic [REP_ITER_WIDTH-1:0]  init_value_iter_OR [NUMBER_OR-1:0];
    logic en_iter_OR    [NUMBER_OR-1:0];
    logic init_iter_OR  [NUMBER_OR-1:0];
    logic co_iter_OR    [NUMBER_OR-1:0];

    logic [REP_DELAY_WIDTH-1:0] count_delay_OR      [NUMBER_OR-1:0];
    logic [REP_DELAY_WIDTH-1:0] init_value_delay_OR [NUMBER_OR-1:0];
    logic en_delay_OR   [NUMBER_OR-1:0];
    logic init_delay_OR [NUMBER_OR-1:0];
    logic co_delay_OR   [NUMBER_OR-1:0];

    logic en_level_OR;
    logic en_level_MT;
    logic en_level_IR;
    logic init0_level_OR;
    logic init0_level_MT;
    logic [1:0] inVal_level_IR;
    


    //TODO: Remove output of these counters as we do not use them :  .count(count_iter_IR[0])
    down_counter #(
        .WIDTH(REP_ITER_WIDTH)
    ) IR_counter_iter_inst (
        .clk(clk),
        .rst_n(rst_n),
        .enable(en_iter_IR[0]),
        .init(init_iter_IR[0]),
        .init_value(regIR_iter[level_MT*NUMBER_IR+0]-1'b1),
        .co(co_iter_IR[0]),
        .count(count_iter_IR[0])
    );

    genvar i;
    for (i = 1; i < NUMBER_IR; i++) begin : IR_counter_iter
        down_counter #(
            .WIDTH(REP_ITER_WIDTH)
        ) IR_counter_iter_inst (
            .clk(clk),
            .rst_n(rst_n),
            .enable(en_iter_IR[i]),
            .init(init_iter_IR[i]),
            .init_value(regIR_iter[level_MT*NUMBER_IR+i]),
            .co(co_iter_IR[i]),
            .count(count_iter_IR[i])
        );
    end
    for (i = 0; i < NUMBER_IR; i++) begin : IR_counter
        down_counter #(
            .WIDTH(REP_DELAY_WIDTH)
        ) IR_counter_delay_inst (
            .clk(clk),
            .rst_n(rst_n),
            .enable(en_delay_IR[i]),
            .init(init_delay_IR[i]),
            .init_value(regIR_delay[level_MT*NUMBER_IR+i]),
            .count(count_delay_IR[i]),
            .co(co_delay_IR[i])
        );
    end

    for (i = 0; i < NUMBER_MT; i++) begin : MT_counter
        down_counter #(
            .WIDTH(TRANS_DELAY_WIDTH)
        ) MT_counter_delay_inst (
            .clk(clk),
            .rst_n(rst_n),
            .enable(en_delay_MT[i]),
            .init(init_delay_MT[i]),
            .init_value(regMT_delay[i]), //init_value_delay_MT
            .count(count_delay_MT[i]),
            .co(co_delay_MT[i])
        );
    end

    for (i = 0; i < NUMBER_OR; i++) begin : OR_counter
        down_counter #(
            .WIDTH(REP_ITER_WIDTH)
        ) OR_counter_iter_inst (
            .clk(clk),
            .rst_n(rst_n),
            .enable(en_iter_OR[i]),
            .init(init_iter_OR[i]),
            .init_value(regOR_iter[i]),  //init_value_iter_OR
            .co(co_iter_OR[i]),
            .count(count_iter_OR[i])
        );
        down_counter #(
            .WIDTH(REP_DELAY_WIDTH)
        ) OR_counter_delay_inst (
            .clk(clk),
            .rst_n(rst_n),
            .enable(en_delay_OR[i]),
            .init(init_delay_OR[i]),
            .init_value(regOR_delay[i]),  //init_value_delay_OR
            .count(count_delay_OR[i]),
            .co(co_delay_OR[i])
        );
    end


    register #(
        .WIDTH(2)
    ) IR_level_register (
        .clk(clk),
        .rst_n(rst_n),
        .enable(en_level_IR),
        .in_value(inVal_level_IR),
        .out_value(level_IR)
    );

    up_counter #(
        .WIDTH(2)
    ) MT_level_counter (
        .clk(clk),
        .rst_n(rst_n),
        .enable(en_level_MT),
        .init0(init0_level_MT),
        .count(level_MT)
    );

    up_counter #(
        .WIDTH(2)
    ) OR_level_counter (
        .clk(clk),
        .rst_n(rst_n),
        .enable(en_level_OR),
        .init0(init0_level_OR),
        .count(level_OR)
    );


    /////////////////////////////////////// FSM PART ///////////////////////////////////////
    typedef enum logic [3:0] {
        IDLE,
        CNFG,
        ACTV,
        // ADDR0,
        GENR_IR0,
        WAIT_IR0,
        GENR_IR1,
        WAIT_IR1,
        GENR_MT,
        WAIT_MT,
        GENR_OR,
        WAIT_OR
    } state_t;

    state_t p_state, n_state;

    always_ff @(posedge clk or negedge rst_n) begin : CONTROLLER_FF
        if (!rst_n) begin 
            p_state <= IDLE;
        end else begin
            p_state <= n_state;
        end
    end

    always_comb begin : CONTROLLER_COMB
        n_state = IDLE;
        rep_valid = 1'b0;
        repx_valid = 1'b0;
        trans_valid = 1'b0;
        en_address = 1'b0;
        init0_address = 1'b0;
        initIR_address = 1'b0;
        initOR_address = 1'b0;
        address_valid = 1'b0;
        for (int i = 0; i < NUMBER_IR; i++) begin
            init_iter_IR[i] = 1'b0;
            init_delay_IR[i] = 1'b0;
            en_iter_IR[i] = 1'b0;
            en_delay_IR[i] = 1'b0;
        end
        for (int i = 0; i < NUMBER_MT; i++) begin
            init_delay_MT[i] = 1'b0;
            en_delay_MT[i] = 1'b0;
        end
        for (int i = 0; i < NUMBER_OR; i++) begin
            init_iter_OR[i] = 1'b0;
            init_delay_OR[i] = 1'b0;
            en_iter_OR[i] = 1'b0;
            en_delay_OR[i] = 1'b0;
        end
        for (int i = 1; i < NUMBER_IR; i++) begin
            init0_initVal_IR[i] = 1'b0;
            init_initVal_IR[i] = 1'b0;
            en_initVal_IR[i] = 1'b0;
        end
        for (int i = 0; i < NUMBER_OR; i++) begin
            init_initVal_OR[i] = 1'b0;
            en_initVal_OR[i] = 1'b0;
        end
        inVal_level_IR = 0;
        en_level_IR = 1'b0;
        en_level_MT = 1'b0;
        en_level_OR = 1'b0;
        init0_level_MT = 1'b0;
        init0_level_OR = 1'b0;

        case (p_state)
            IDLE: begin
                // TODO: Reset all config registers
                if (instr_en) begin
                    n_state = CNFG;
                end else begin
                    n_state = IDLE;
                end
            end
            CNFG: begin
                if (opcode == 0) begin          // REP
                    rep_valid = 1;
                end else if (opcode == 1) begin // REPX
                    repx_valid = 1;
                end else if (opcode == 2) begin // TRANS
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
                    for (int i = 0; i < NUMBER_OR; i++) begin
                        init_iter_OR[i] = 1'b1;
                        init_delay_OR[i] = 1'b1;
                    end
                    flag_OR = 1'b0;
                end
            end
            ACTV: begin
                if (!activation) begin 
                    n_state = ACTV;
                end else if (regIR_config[0]) begin
                    if (!co_delay_IR[0]) begin
                        n_state = WAIT_IR0;
                    end else begin
                        n_state = GENR_IR0;
                    end
                end else if (regMT_config[0]) begin
                    //TODO: consider RT pattern instead of RTR
                    n_state = IDLE;
                end
            end
            GENR_IR0: begin
                address_valid = 1'b1;
                en_address = 1'b1;
                en_iter_IR[0] = 1'b1;
                if (!co_iter_IR[0] && !co_delay_IR[0]) begin
                    n_state = WAIT_IR0;
                end else if (!co_iter_IR[0] && co_delay_IR[0]) begin
                    n_state = GENR_IR0;
                end else if (regIR_config[NUMBER_IR*level_MT+1]) begin
                    if (!co_iter_IR[1] && !co_delay_IR[1]) begin
                        n_state = WAIT_IR1;
                    end else if (!co_iter_IR[1] && co_delay_IR[1]) begin
                        en_level_IR = 1'b1;
                        inVal_level_IR = 1;
                        en_initVal_IR[1] = 1'b1;
                        n_state = GENR_IR1;
                    end else if (regIR_config[NUMBER_IR*level_MT+2]) begin
                        if (!co_iter_IR[2] && !co_delay_IR[2]) begin
                            n_state = WAIT_IR1;
                        end else if (!co_iter_IR[2] && co_delay_IR[2]) begin
                            n_state = GENR_IR1;
                            en_level_IR = 1'b1;
                            inVal_level_IR = 2;
                            en_initVal_IR[2] = 1'b1;
                        end else if (regMT_config[level_MT]) begin
                            if (!co_delay_MT[level_MT]) begin
                                n_state = WAIT_MT;
                            end else begin
                                n_state = GENR_MT;
                                en_level_MT = 1'b1;
                            end
                        end else if (regOR_config[level_OR]) begin
                            init0_level_MT = 1'b1;
                            if (!co_iter_OR[level_OR] && !co_delay_OR[level_OR]) begin
                                n_state = WAIT_OR;
                            end else if (!co_iter_OR[level_OR] && co_delay_OR[level_OR]) begin
                                n_state = GENR_OR;
                                en_initVal_OR[level_OR] = 1'b1;
                            end else begin
                                n_state = GENR_OR;
                                en_level_OR = 1'b1;
                                en_initVal_OR[level_OR + 1] = 1'b1;
                            end
                        end else begin
                            n_state = ACTV;
                        end
                    end else if (regMT_config[level_MT]) begin
                       if (!co_delay_MT[level_MT]) begin
                            n_state = WAIT_MT;
                        end else begin
                            n_state = GENR_MT;
                            en_level_MT = 1'b1;
                        end
                    end else if (regOR_config[level_OR]) begin
                        init0_level_MT = 1'b1;
                        if (!co_iter_OR[level_OR] && !co_delay_OR[level_OR]) begin
                            n_state = WAIT_OR;
                        end else if (!co_iter_OR[level_OR] && co_delay_OR[level_OR]) begin
                            n_state = GENR_OR;
                            en_initVal_OR[level_OR] = 1'b1;
                        end else begin
                            n_state = GENR_OR;
                            en_level_OR = 1'b1;
                            en_initVal_OR[level_OR + 1] = 1'b1;
                        end
                    end else begin
                        n_state = ACTV;
                    end
                end else if (regMT_config[level_MT]) begin
                    if (!co_delay_MT[level_MT]) begin
                            n_state = WAIT_MT;
                        end else begin
                            n_state = GENR_MT;
                            en_level_MT = 1'b1;
                        end
                end else if (regOR_config[level_OR]) begin
                    init0_level_MT = 1'b1;
                    if (!co_iter_OR[level_OR] && !co_delay_OR[level_OR]) begin
                        n_state = WAIT_OR;
                    end else if (!co_iter_OR[level_OR] && co_delay_OR[level_OR]) begin
                        n_state = GENR_OR;
                        en_initVal_OR[level_OR] = 1'b1;
                    end else begin
                        n_state = GENR_OR;
                        en_level_OR = 1'b1;
                        en_initVal_OR[level_OR + 1] = 1'b1;
                    end
                end else begin
                    n_state = ACTV;
                end
            end
            WAIT_IR0: begin
                en_delay_IR[0] = 1'b1;
                if (!co_delay_IR[0]) begin
                    n_state = WAIT_IR0;
                end else if (co_delay_IR[0]) begin
                    init_delay_IR[0] = 1'b1;
                    n_state = GENR_IR0;
                end
            end
///////////////////////////////////
            GENR_IR1: begin
                address_valid = 1'b1;
                initIR_address = 1'b1;
                en_iter_IR[level_IR] = 1'b1;
                if (!co_delay_IR[0]) begin
                    n_state = WAIT_IR0;
                end else begin
                    en_level_IR = 1'b1;
                    inVal_level_IR = 0;
                    n_state = GENR_IR0;
                    for (int i = 0; i < level_IR; i++) begin
                        init_delay_IR[i] = 1'b1;
                        init_iter_IR[i] = 1'b1;
                    end
                    for (int i = 1; i < level_IR; i++) begin
                        init_initVal_IR[i] = 1'b1;
                    end
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
            GENR_MT: begin
                address_valid = 1'b1;
                init0_address = 1'b1;
                if (!co_delay_IR[0]) begin
                    n_state = WAIT_IR0;
                end else begin
                    en_level_IR = 1'b1;
                    inVal_level_IR = 0;
                    n_state = GENR_IR0;
                    for (int i = 0; i < NUMBER_IR; i++) begin
                        init_delay_IR[i] = 1'b1;
                        init_iter_IR[i] = 1'b1;
                    end
                    for (int i = 1; i < NUMBER_IR; i++) begin
                        init0_initVal_IR[i] = 1'b1;
                    end
                end
            end
            WAIT_MT: begin
                en_delay_MT[en_level_MT] = 1'b1;
                if (!co_delay_MT[level_MT]) begin
                    n_state = WAIT_MT;
                end else if (co_delay_MT[level_MT]) begin
                    init_delay_MT[level_MT] = 1'b1;
                    n_state = GENR_MT;
                end
            end
/////////////////////////////////// OR-level ///////////////////////////////////    
            GENR_OR: begin
                address_valid = 1'b1;
                initOR_address = 1'b1;
                en_iter_OR[level_OR] = 1'b1;
                flag_OR = 1'b1;
                if (!co_delay_IR[0]) begin
                    n_state = WAIT_IR0;
                end else begin
                    en_level_IR = 1'b1;
                    inVal_level_IR = 0;
                    n_state = GENR_IR0;
                    for (int i = 0; i < NUMBER_IR; i++) begin
                        init_delay_IR[i] = 1'b1;
                        init_iter_IR[i] = 1'b1;
                    end
                    for (int i = 0; i < NUMBER_MT; i++) begin
                        init_delay_MT[i] = 1'b1;
                    end
                    for (int i = 0; i < level_OR; i++) begin
                        init_delay_OR[i] = 1'b1;
                        init_iter_OR[i] = 1'b1;
                    end
                    for (int i = 1; i < NUMBER_IR; i++) begin
                        init0_initVal_IR[i] = 1'b1;
                    end
                    for (int i = 1; i < level_OR; i++) begin
                        init_initVal_OR[i] = 1'b1;
                    end
                end
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
        endcase
    end


endmodule