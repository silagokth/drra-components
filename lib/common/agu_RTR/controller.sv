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

    logic en_iter_IR    [NUMBER_IR-1:0];
    logic init_iter_IR  [NUMBER_IR-1:0];
    logic co_iter_IR    [NUMBER_IR-1:0];
    logic en_delay_IR   [NUMBER_IR-1:0];
    logic init_delay_IR [NUMBER_IR-1:0];
    logic co_delay_IR   [NUMBER_IR-1:0];

    logic en_delay_MT   [NUMBER_MT-1:0];
    logic init_delay_MT [NUMBER_MT-1:0];
    logic co_delay_MT   [NUMBER_MT-1:0];

    logic en_iter_OR    [NUMBER_OR-1:0];
    logic init_iter_OR  [NUMBER_OR-1:0];
    logic co_iter_OR    [NUMBER_OR-1:0];
    logic en_delay_OR   [NUMBER_OR-1:0];
    logic init_delay_OR [NUMBER_OR-1:0];
    logic co_delay_OR   [NUMBER_OR-1:0];

    logic en_level_OR;
    logic en_level_MT;
    logic en_level_IR;
    logic init0_level_MT;
    logic [1:0] inVal_level_IR;
    logic [1:0] inVal_level_OR;
    logic flag_OR2;


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
    genvar i;
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


    register #(
        .WIDTH(2)
    ) register_level_IR (
        .clk(clk),
        .rst_n(rst_n),
        .enable(en_level_IR),
        .in_value(inVal_level_IR),
        .out_value(level_IR)
    );

    up_counter #(
        .WIDTH(2)
    ) counter_level_MT (
        .clk(clk),
        .rst_n(rst_n),
        .enable(en_level_MT),
        .init0(init0_level_MT),
        .count(level_MT)
    );

    register #(
        .WIDTH(2)
    ) register_level_OR (
        .clk(clk),
        .rst_n(rst_n),
        .enable(en_level_OR),
        .in_value(inVal_level_OR),
        .out_value(level_OR)
    );


    /////////////////////////////////////// FSM PART ///////////////////////////////////////
    typedef enum logic [3:0] {
        IDLE,
        CNFG,
        ACTV,
        GENR_Add0,
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
        flag_OR2 = 1'b0;
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
                    for (int i = 0; i < NUMBER_MT; i++) begin
                        init_delay_MT[i] = 1'b1;
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
                end  else if (regMT_config[0]) begin
                    //TODO: consider RT pattern instead of RTR 
                    n_state = IDLE;
                end
            end

            GENR_IR0: begin
                address_valid = 1'b1;
                en_address = 1'b1;
                en_iter_IR[0] = 1'b1;
                if (!co_iter_IR[0] && !co_delay_IR[0]) begin
                    en_delay_IR[0] = 1'b1;
                    n_state = WAIT_IR0;
                end else if (!co_iter_IR[0] && co_delay_IR[0]) begin
                    n_state = GENR_IR0;
                end else if (regIR_config[NUMBER_IR*level_MT+1]) begin
                    if (!co_iter_IR[1] && !co_delay_IR[1]) begin
                        en_level_IR = 1'b1;
                        inVal_level_IR = 1;
                        en_initVal_IR[1] = 1'b1;
                        en_delay_IR[1] = 1'b1;
                        n_state = WAIT_IR1;
                    end else if (!co_iter_IR[1] && co_delay_IR[1]) begin
                        en_level_IR = 1'b1;
                        inVal_level_IR = 1;
                        en_initVal_IR[1] = 1'b1;
                        n_state = GENR_IR1;
                    end else if (regIR_config[NUMBER_IR*level_MT+2]) begin
                        if (!co_iter_IR[2] && !co_delay_IR[2]) begin
                            en_level_IR = 1'b1;
                            inVal_level_IR = 2;
                            en_initVal_IR[2] = 1'b1;
                            en_delay_IR[2] = 1'b1;
                            n_state = WAIT_IR1;
                        end else if (!co_iter_IR[2] && co_delay_IR[2]) begin
                            en_level_IR = 1'b1;
                            inVal_level_IR = 2;
                            en_initVal_IR[2] = 1'b1;
                            n_state = GENR_IR1;
                        end else if (regMT_config[level_MT]) begin
                            if (!co_delay_MT[level_MT]) begin
                                en_delay_MT[level_MT] = 1'b1;
                                en_level_MT = 1'b1;
                                n_state = WAIT_MT;
                            end else begin
                                en_level_MT = 1'b1;
                                n_state = GENR_MT;
                            end
                        end else if (regOR_config[0]) begin
                            init0_level_MT = 1'b1;
                            flag_OR = 1'b1;
                            flag_OR2 = 1'b1;
                            init_delay_IR[0] = 1'b1;
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
                            end else if (regOR_config[1]) begin
                                if (!co_iter_OR[1] && !co_delay_OR[1]) begin
                                    en_level_OR = 1'b1;
                                    inVal_level_OR = 1;
                                    en_initVal_OR[1] = 1'b1;
                                    en_delay_OR[1] = 1'b1;
                                    n_state = WAIT_OR;
                                end else if (!co_iter_OR[1] && co_delay_OR[1]) begin
                                    en_level_OR = 1'b1;
                                    inVal_level_OR = 1;
                                    en_initVal_OR[1] = 1'b1;
                                    n_state = GENR_OR;
                                end else if (regOR_config[2]) begin
                                    if (!co_iter_OR[2] && !co_delay_OR[2]) begin
                                        en_level_OR = 1'b1;
                                        inVal_level_OR = 2;
                                        en_initVal_OR[2] = 1'b1;
                                        en_delay_OR[2] = 1'b1;
                                        n_state = WAIT_OR;
                                    end else if (!co_iter_OR[2] && co_delay_OR[2]) begin
                                        en_level_OR = 1'b1;
                                        inVal_level_OR = 2;
                                        en_initVal_OR[2] = 1'b1;
                                        n_state = GENR_OR;
                                    end else begin
                                        n_state = ACTV;
                                    end
                                end else begin
                                    n_state = ACTV;
                                end
                            end else begin
                                n_state = ACTV;
                            end
                        end else begin
                            n_state = ACTV;
                        end
                    end else if (regMT_config[level_MT]) begin
                       if (!co_delay_MT[level_MT]) begin
                            en_delay_MT[level_MT] = 1'b1;
                            en_level_MT = 1'b1;
                            n_state = WAIT_MT;
                        end else begin
                            en_level_MT = 1'b1;
                            n_state = GENR_MT;
                        end
                    end else if (regOR_config[0]) begin
                        init0_level_MT = 1'b1;
                        init_delay_IR[0] = 1'b1;
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
                        end else if (regOR_config[1]) begin
                            if (!co_iter_OR[1] && !co_delay_OR[1]) begin
                                en_level_OR = 1'b1;
                                inVal_level_OR = 1;
                                en_initVal_OR[1] = 1'b1;
                                en_delay_OR[1] = 1'b1;
                                n_state = WAIT_OR;
                            end else if (!co_iter_OR[1] && co_delay_OR[1]) begin
                                en_level_OR = 1'b1;
                                inVal_level_OR = 1;
                                en_initVal_OR[1] = 1'b1;
                                n_state = GENR_OR;
                            end else if (regOR_config[2]) begin
                                if (!co_iter_OR[2] && !co_delay_OR[2]) begin
                                    en_level_OR = 1'b1;
                                    inVal_level_OR = 2;
                                    en_initVal_OR[2] = 1'b1;
                                    en_delay_OR[2] = 1'b1;
                                    n_state = WAIT_OR;
                                end else if (!co_iter_OR[2] && co_delay_OR[2]) begin
                                    en_level_OR = 1'b1;
                                    inVal_level_OR = 2;
                                    en_initVal_OR[2] = 1'b1;
                                    n_state = GENR_OR;
                                end else begin
                                    n_state = ACTV;
                                end
                            end else begin
                                n_state = ACTV;
                            end
                        end else begin
                            n_state = ACTV;
                        end
                    end else begin
                        n_state = ACTV;
                    end
                end else if (regMT_config[level_MT]) begin
                    if (!co_delay_MT[level_MT]) begin
                            en_delay_MT[level_MT] = 1'b1;
                            en_level_MT = 1'b1;
                            n_state = WAIT_MT;
                        end else begin
                            en_level_MT = 1'b1;
                            n_state = GENR_MT;
                        end
                end else if (regOR_config[0]) begin
                    init0_level_MT = 1'b1;
                    init_delay_IR[0] = 1'b1;
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
                    end else if (regOR_config[1]) begin
                        if (!co_iter_OR[1] && !co_delay_OR[1]) begin
                            en_level_OR = 1'b1;
                            inVal_level_OR = 1;
                            en_initVal_OR[1] = 1'b1;
                            en_delay_OR[1] = 1'b1;
                            n_state = WAIT_OR;
                        end else if (!co_iter_OR[1] && co_delay_OR[1]) begin
                            en_level_OR = 1'b1;
                            inVal_level_OR = 1;
                            en_initVal_OR[1] = 1'b1;
                            n_state = GENR_OR;
                        end else if (regOR_config[2]) begin
                            if (!co_iter_OR[2] && !co_delay_OR[2]) begin
                                en_level_OR = 1'b1;
                                inVal_level_OR = 2;
                                en_initVal_OR[2] = 1'b1;
                                en_delay_OR[2] = 1'b1;
                                n_state = WAIT_OR;
                            end else if (!co_iter_OR[2] && co_delay_OR[2]) begin
                                en_level_OR = 1'b1;
                                inVal_level_OR = 2;
                                en_initVal_OR[2] = 1'b1;
                                n_state = GENR_OR;
                            end else begin
                                n_state = ACTV;
                            end
                        end else begin
                            n_state = ACTV;
                        end
                    end else begin
                        n_state = ACTV;
                    end
                end else begin
                    n_state = ACTV;
                end
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
///////////////////////////////////
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
/////////////////////////////////// OR-level ///////////////////////////////////    
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
        endcase
    end


endmodule