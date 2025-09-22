`ifdef INCLUDE_MT_STATES

task automatic evaluate_MT_state(
    input  logic [$clog2(NUMBER_MT+1)-1:0] level_MT,
    input  logic                           co_delay_MT [NUMBER_MT],
    output logic                           en_delay_MT [NUMBER_MT],
    output logic                           en_level_MT,
    output state_t n_state
);
    for (int i = 0; i < NUMBER_MT; i++) begin
        en_delay_MT[i] = 1'b0;
    end
    en_level_MT = 1'b0;
    n_state = IDLE;

    if (!co_delay_MT[level_MT]) begin
        en_delay_MT[level_MT] = 1'b1;
        en_level_MT = 1'b1;
        n_state = WAIT_MT;
    end else begin
        en_level_MT = 1'b1;
        n_state = GENR_MT;
    end
endtask 
`endif

`ifdef INCLUDE_OR_STATES
task automatic evaluate_OR_state(
    input  logic                         co_iter_OR   [NUMBER_OR],
    input  logic                         co_delay_OR  [NUMBER_OR],
    input  logic                         regOR_config [NUMBER_OR],
    output logic                         en_level_OR,
    output logic [$clog2(NUMBER_OR)-1:0] inVal_level_OR,
    output logic                         en_initVal_OR [NUMBER_OR],
    output logic                         en_delay_OR   [NUMBER_OR],
    output state_t n_state
);
    en_level_OR = 1'b0;
    inVal_level_OR = 0;
    for (int i = 0; i < NUMBER_OR; i++) begin
        en_initVal_OR[i] = 1'b0;
        en_delay_OR[i] = 1'b0;
    end
    n_state = IDLE;

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
            end else if (regOR_config[3]) begin
                if (!co_iter_OR[3] && !co_delay_OR[3]) begin
                    en_level_OR = 1'b1;
                    inVal_level_OR = 3;
                    en_initVal_OR[3] = 1'b1;
                    en_delay_OR[3] = 1'b1;
                    n_state = WAIT_OR;
                end else if (!co_iter_OR[3] && co_delay_OR[3]) begin
                    en_level_OR = 1'b1;
                    inVal_level_OR = 3;
                    en_initVal_OR[3] = 1'b1;
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
endtask
`endif 

`ifdef INCLUDE_IR_STATES
task automatic evaluate_IR_state(
    `ifdef INCLUDE_MT_STATES
    input  logic                           co_delay_MT [NUMBER_MT],
    output logic                           en_delay_MT [NUMBER_MT],
    output logic                           init0_level_MT,
    output logic                           en_level_MT,
    `endif  
  
    `ifdef INCLUDE_OR_STATES  
    input  logic                           co_iter_OR   [NUMBER_OR],
    input  logic                           co_delay_OR  [NUMBER_OR],
    input  logic                           regOR_config [NUMBER_OR],
    output logic                           en_level_OR,
    output logic [$clog2(NUMBER_OR)-1:0]   inVal_level_OR,
    output logic                           en_initVal_OR [NUMBER_OR],
    output logic                           en_delay_OR   [NUMBER_OR],
    output logic                           en_flag_OR,
    `endif

    input  logic [$clog2(NUMBER_MT+1)-1:0] level_MT,
    input  logic                           co_iter_IR   [NUMBER_IR],
    input  logic                           co_delay_IR  [NUMBER_IR],
    input  logic                           regIR_config [(NUMBER_MT+1)*NUMBER_IR],
    output logic                           en_level_IR,
    output logic [$clog2(NUMBER_IR)-1:0]   inVal_level_IR,
    output logic                           en_initVal_IR [1:NUMBER_IR-1],
    output logic                           en_delay_IR   [NUMBER_IR],
    output logic                           init_delay_IR [NUMBER_IR],
    output state_t n_state
);
    `ifdef INCLUDE_OR_STATES
        `ifndef INCLUDE_MT_STATES
            logic init0_level_MT;
        `endif
    `endif
    
    n_state = IDLE;

    en_level_IR = 1'b0;
    inVal_level_IR = 0;
    for (int i = 1; i < NUMBER_IR; i++) begin
        en_initVal_IR[i] = 1'b0;
    end
    for (int i = 0; i < NUMBER_IR; i++) begin
        en_delay_IR[i] = 1'b0;
        init_delay_IR[i] = 1'b0;
    end

    `ifdef INCLUDE_MT_STATES
    init0_level_MT = 1'b0;
    en_level_MT = 1'b0;
    for (int i = 0; i < NUMBER_MT; i++) begin
        en_delay_MT[i] = 1'b0;
    end
    `endif

    `ifdef INCLUDE_OR_STATES
    en_level_OR = 1'b0;
    inVal_level_OR = 0;
    en_flag_OR = 1'b0;
    for (int i = 0; i < NUMBER_OR; i++) begin
        en_initVal_OR[i] = 1'b0;
        en_delay_OR[i] = 1'b0;
    end
    `endif


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
            end else if (regIR_config[NUMBER_IR*level_MT+3]) begin
                if (!co_iter_IR[3] && !co_delay_IR[3]) begin
                    en_level_IR = 1'b1;
                    inVal_level_IR = 3;
                    en_initVal_IR[3] = 1'b1;
                    en_delay_IR[3] = 1'b1;
                    n_state = WAIT_IR1;
                end else if (!co_iter_IR[3] && co_delay_IR[3]) begin
                    en_level_IR = 1'b1;
                    inVal_level_IR = 3;
                    en_initVal_IR[3] = 1'b1;
                    n_state = GENR_IR1;
                `ifdef INCLUDE_MT_STATES
                end else if (regMT_config[level_MT]) begin
                    init_delay_IR[0] = 1'b1;
                    evaluate_MT_state(level_MT, co_delay_MT, en_delay_MT, en_level_MT, n_state);
                `endif
                `ifdef INCLUDE_OR_STATES
                end else if (regOR_config[0]) begin
                    init0_level_MT = 1'b1;
                    en_flag_OR = 1'b1;
                    init_delay_IR[0] = 1'b1;
                    evaluate_OR_state(co_iter_OR, co_delay_OR, regOR_config, 
                    en_level_OR, inVal_level_OR, en_initVal_OR, en_delay_OR, n_state);
                `endif
                end else begin
                    n_state = ACTV;
                end
            `ifdef INCLUDE_MT_STATES
            end else if (regMT_config[level_MT]) begin
                evaluate_MT_state(level_MT, co_delay_MT, en_delay_MT, en_level_MT, n_state);
            `endif
            `ifdef INCLUDE_OR_STATES
            end else if (regOR_config[0]) begin
                init0_level_MT = 1'b1;
                en_flag_OR = 1'b1;
                init_delay_IR[0] = 1'b1;
                evaluate_OR_state(co_iter_OR, co_delay_OR, regOR_config, 
                en_level_OR, inVal_level_OR, en_initVal_OR, en_delay_OR, n_state);
            `endif
            end else begin
                n_state = ACTV;
            end
        `ifdef INCLUDE_MT_STATES
        end else if (regMT_config[level_MT]) begin
           evaluate_MT_state(level_MT, co_delay_MT, en_delay_MT, en_level_MT, n_state);
        `endif
        `ifdef INCLUDE_OR_STATES
        end else if (regOR_config[0]) begin
            init0_level_MT = 1'b1;
            init_delay_IR[0] = 1'b1;
            evaluate_OR_state(co_iter_OR, co_delay_OR, regOR_config, 
                en_level_OR, inVal_level_OR, en_initVal_OR, en_delay_OR, n_state);
        `endif
        end else begin
            n_state = ACTV;
        end
    `ifdef INCLUDE_MT_STATES
    end else if (regMT_config[level_MT]) begin
       evaluate_MT_state(level_MT, co_delay_MT, en_delay_MT, en_level_MT, n_state);
    `endif
    `ifdef INCLUDE_OR_STATES
    end else if (regOR_config[0]) begin
        init0_level_MT = 1'b1;
        init_delay_IR[0] = 1'b1;
        evaluate_OR_state(co_iter_OR, co_delay_OR, regOR_config, 
                en_level_OR, inVal_level_OR, en_initVal_OR, en_delay_OR, n_state);
    `endif
    end else begin
        n_state = ACTV;
    end
endtask
`endif