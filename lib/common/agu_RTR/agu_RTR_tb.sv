module agu_RTR_tb #(

    localparam ADDRESS_WIDTH = 8,

    localparam REP_LEVEL_WIDTH = 4,
    localparam REP_DELAY_WIDTH = 6,
    localparam REP_ITER_WIDTH = 6,
    localparam REP_STEP_WIDTH = 6,
    localparam TRANS_LEVEL_WIDTH = 4,
    localparam TRANS_DELAY_WIDTH = 12,
    
    localparam NUMBER_OR = 4,    // OR: Outter R-Pattern
    localparam NUMBER_MT = 3,    // MT: Middle T-Pattern
    localparam NUMBER_IR = 4     // IR: Inner  R-Pattern
) ();

    logic                         clk;
    logic                         rst_n;
    logic                         activation;
    logic                         rep_valid;
    logic                         repx_valid;
    logic                         trans_valid;
    logic [REP_LEVEL_WIDTH-1:0]   rep_level;
    logic [REP_DELAY_WIDTH-1:0]   rep_delay;
    logic [REP_ITER_WIDTH-1:0]    rep_iter;
    logic [REP_STEP_WIDTH-1:0]    rep_step;
    logic [TRANS_LEVEL_WIDTH-1:0] trans_level;
    logic [TRANS_DELAY_WIDTH-1:0] trans_delay;
    logic                         address_valid;
    logic [ADDRESS_WIDTH-1:0]     address;

    agu_RTR # (
        .ADDRESS_WIDTH(ADDRESS_WIDTH),
        .REP_LEVEL_WIDTH(REP_LEVEL_WIDTH),
        .REP_DELAY_WIDTH(REP_DELAY_WIDTH),
        .REP_ITER_WIDTH(REP_ITER_WIDTH),
        .REP_STEP_WIDTH(REP_STEP_WIDTH),
        .TRANS_LEVEL_WIDTH(TRANS_LEVEL_WIDTH),
        .TRANS_DELAY_WIDTH(TRANS_DELAY_WIDTH),
        .NUMBER_OR(NUMBER_OR),
        .NUMBER_MT(NUMBER_MT),
        .NUMBER_IR(NUMBER_IR)
    ) agu_RTR_inst (
        .clk(clk),
        .rst_n(rst_n),
        .activation(activation),
        .rep_valid(rep_valid),
        .repx_valid(repx_valid),
        .trans_valid(trans_valid),
        .rep_level(rep_level),
        .rep_delay(rep_delay),
        .rep_iter(rep_iter),
        .rep_step(rep_step),
        .trans_level(trans_level),
        .trans_delay(trans_delay),
        .address_valid(address_valid),
        .address(address)
    );

    always #5 clk = ~clk;

    initial begin
        clk = 1'b0;
        rst_n = 1'b0;
        activation = 1'b0;
        rep_valid = 1'b0;
        repx_valid = 1'b0;
        trans_valid = 1'b0;
        rep_level = 0;
        rep_delay = 0;
        rep_iter = 0;
        rep_step = 0;
        trans_level = 0;
        trans_delay = 0;

        #17;
        rst_n = 1'b1;

/////////////////////////////////// IR != 0 => RTR, TR, RR, R ///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////// Delay == 0
        // // Must be issued one cycle before the first rep instruction, to go to the CNFG state
        // @(posedge clk);
        // rep_valid = 1'b1;
        
        // // IR0
        // @(posedge clk);
        // rep_level = 0; rep_step = 3;  rep_delay = 0; rep_iter = 2; // L0, S3,  D0, iter2
        // @(posedge clk);  
        // rep_level = 1; rep_step = 1;  rep_delay = 0; rep_iter = 3; // L1, S1,  D0, iter3
        // @(posedge clk);  
        // rep_level = 2; rep_step = 2;  rep_delay = 0; rep_iter = 1; // L2, S2,  D0, iter1
        // @(posedge clk);
        // rep_level = 3; rep_step = 10; rep_delay = 0; rep_iter = 2; // L3, S10, D0, iter2

        // // MT0
        // @(posedge clk);
        // rep_valid = 1'b0;
        // trans_valid = 1'b1;
        // trans_level = 0; trans_delay = 0;

        // // IR1
        // @(posedge clk);
        // rep_valid = 1'b1;
        // trans_valid = 1'b0;
        // rep_level = 0; rep_step = 2; rep_delay = 0; rep_iter = 2; // L0, S2, D0, iter2
        // @(posedge clk);
        // rep_level = 1; rep_step = 3; rep_delay = 0; rep_iter = 3; // L1, S3, D0, iter3
        // @(posedge clk);
        // rep_level = 2; rep_step = 5; rep_delay = 0; rep_iter = 1; // L2, S5, D0, iter1
        // @(posedge clk);
        // rep_level = 3; rep_step = 4; rep_delay = 0; rep_iter = 1; // L3, S4, D0, iter1

        // // MT1
        // @(posedge clk);
        // rep_valid = 1'b0;
        // trans_valid = 1'b1;
        // trans_level = 1; trans_delay = 0;

        // // IR2
        // @(posedge clk);
        // rep_valid = 1'b1;
        // trans_valid = 1'b0;
        // rep_level = 0; rep_step = 7; rep_delay = 0; rep_iter = 1; // L0, S7, D0, iter1
        // @(posedge clk);
        // rep_level = 1; rep_step = 1; rep_delay = 0; rep_iter = 2; // L1, S1, D0, iter2
        // @(posedge clk);
        // rep_level = 2; rep_step = 3; rep_delay = 0; rep_iter = 3; // L2, S3, D0, iter3
        // @(posedge clk);
        // rep_level = 3; rep_step = 5; rep_delay = 0; rep_iter = 2; // L3, S5, D0, iter2

        // // MT2
        // @(posedge clk);
        // rep_valid = 1'b0;
        // trans_valid = 1'b1;
        // trans_level = 2; trans_delay = 0;

        // // IR3
        // @(posedge clk);
        // rep_valid = 1'b1;
        // trans_valid = 1'b0;
        // rep_level = 0; rep_step = 1; rep_delay = 0; rep_iter = 5; // L0, S1, D0, iter5
        // @(posedge clk);
        // rep_level = 1; rep_step = 6; rep_delay = 0; rep_iter = 1; // L1, S6, D0, iter1
        // @(posedge clk);
        // rep_level = 2; rep_step = 0; rep_delay = 0; rep_iter = 2; // L2, S0, D0, iter2
        // @(posedge clk);
        // rep_level = 3; rep_step = 5; rep_delay = 0; rep_iter = 1; // L3, S5, D0, iter1
        
        // // OR
        // @(posedge clk);
        // rep_level = 4; rep_step = 10; rep_delay = 0; rep_iter = 2; // L4, S10, D0, iter2
        // @(posedge clk);
        // rep_level = 5; rep_step = 3;  rep_delay = 0; rep_iter = 1; // L5, S3, D0, iter1
        // @(posedge clk);
        // rep_level = 6; rep_step = 5;  rep_delay = 0; rep_iter = 1; // L6, S5, D0, iter1
        // @(posedge clk);
        // rep_level = 7; rep_step = 1;  rep_delay = 0; rep_iter = 1; // L7, S1, D0, iter1

////////////////////////////////////////////////////////////////////////////////////// Delay != 0
        // Must be issued one cycle before the first rep instruction, to go to the CNFG state
        // @(posedge clk);
        // rep_valid = 1'b1;
        
        // // IR0
        // @(posedge clk);
        // rep_level = 0; rep_step = 3;  rep_delay = 2; rep_iter = 2; // L0, S3,  D2, iter2
        // @(posedge clk);  
        // rep_level = 1; rep_step = 1;  rep_delay = 1; rep_iter = 3; // L1, S1,  D1, iter3
        // @(posedge clk);  
        // rep_level = 2; rep_step = 2;  rep_delay = 3; rep_iter = 1; // L2, S2,  D3, iter1
        // @(posedge clk);
        // rep_level = 3; rep_step = 10; rep_delay = 0; rep_iter = 2; // L3, S10, D0, iter2

        // // MT0
        // @(posedge clk);
        // rep_valid = 1'b0;
        // trans_valid = 1'b1;
        // trans_level = 0; trans_delay = 7;

        // // IR1
        // @(posedge clk);
        // rep_valid = 1'b1;
        // trans_valid = 1'b0;
        // rep_level = 0; rep_step = 2; rep_delay = 0; rep_iter = 2; // L0, S2, D0, iter2
        // @(posedge clk);
        // rep_level = 1; rep_step = 3; rep_delay = 0; rep_iter = 3; // L1, S3, D0, iter3
        // @(posedge clk);
        // rep_level = 2; rep_step = 5; rep_delay = 0; rep_iter = 1; // L2, S5, D0, iter1
        // @(posedge clk);
        // rep_level = 3; rep_step = 4; rep_delay = 3; rep_iter = 1; // L3, S4, D3, iter1

        // // MT1
        // @(posedge clk);
        // rep_valid = 1'b0;
        // trans_valid = 1'b1;
        // trans_level = 1; trans_delay = 3;

        // // IR2
        // @(posedge clk);
        // rep_valid = 1'b1;
        // trans_valid = 1'b0;
        // rep_level = 0; rep_step = 7; rep_delay = 0; rep_iter = 1; // L0, S7, D0, iter1
        // @(posedge clk);
        // rep_level = 1; rep_step = 1; rep_delay = 0; rep_iter = 2; // L1, S1, D0, iter2
        // @(posedge clk);
        // rep_level = 2; rep_step = 3; rep_delay = 0; rep_iter = 3; // L2, S3, D0, iter3
        // @(posedge clk);
        // rep_level = 3; rep_step = 5; rep_delay = 0; rep_iter = 2; // L3, S5, D0, iter2

        // // MT2
        // @(posedge clk);
        // rep_valid = 1'b0;
        // trans_valid = 1'b1;
        // trans_level = 2; trans_delay = 0;

        // // IR3
        // @(posedge clk);
        // rep_valid = 1'b1;
        // trans_valid = 1'b0;
        // rep_level = 0; rep_step = 1; rep_delay = 1; rep_iter = 5; // L0, S1, D1, iter5
        // @(posedge clk);
        // rep_level = 1; rep_step = 6; rep_delay = 3; rep_iter = 1; // L1, S6, D3, iter1
        // @(posedge clk);
        // rep_level = 2; rep_step = 0; rep_delay = 7; rep_iter = 2; // L2, S0, D7, iter2
        // @(posedge clk);
        // rep_level = 3; rep_step = 5; rep_delay = 9; rep_iter = 1; // L3, S5, D9, iter1
        
        // // OR
        // @(posedge clk);
        // rep_level = 4; rep_step = 10; rep_delay = 0; rep_iter = 2; // L4, S10, D0, iter2
        // @(posedge clk);
        // rep_level = 5; rep_step = 3;  rep_delay = 5; rep_iter = 1; // L5, S3,  D5, iter1
        // @(posedge clk); 
        // rep_level = 6; rep_step = 5;  rep_delay = 0; rep_iter = 1; // L6, S5,  D0, iter1
        // @(posedge clk); 
        // rep_level = 7; rep_step = 1;  rep_delay = 9; rep_iter = 1; // L7, S1,  D9, iter1


////////////////////////////////////// IR == 0 => RT, T, 0 //////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////// Delay == 0
        // Must be issued one cycle before the first rep instruction, to go to the CNFG state
        // @(posedge clk);
        // trans_valid = 1'b1;

        // // MT0
        // @(posedge clk);
        // trans_level = 0; trans_delay = 0;

        // // MT1
        // @(posedge clk);
        // trans_level = 1; trans_delay = 0;

        // // MT2
        // @(posedge clk);
        // trans_level = 2; trans_delay = 0;

        // // OR
        // @(posedge clk);
        // rep_valid = 1'b1;
        // trans_valid = 1'b0;
        // rep_level = 0; rep_step = 10; rep_delay = 0; rep_iter = 2; // L0, S10, D0, iter2
        // @(posedge clk);
        // rep_level = 1; rep_step = 3;  rep_delay = 0; rep_iter = 1; // L1, S3,  D0, iter1
        // @(posedge clk); 
        // rep_level = 2; rep_step = 5;  rep_delay = 0; rep_iter = 3; // L2, S5,  D0, iter3
        // @(posedge clk); 
        // rep_level = 3; rep_step = 1;  rep_delay = 0; rep_iter = 1; // L3, S1,  D0, iter1

////////////////////////////////////////////////////////////////////////////////////// Delay != 0
        // @(posedge clk);
        // instr_en = 1'b1;

        // // trans_level_delay
        // @(posedge clk);
        // instr = 25'b010_0001_000000000000000011;
        // @(posedge clk);
        // instr = 25'b010_0000_000000000000000111;

        // // rep_option_level_step_delay_iter
        // @(posedge clk);
        // instr = 25'b000_00_000_1010_0000010_000010; // op0, L0, S10, D0, iter2
        // @(posedge clk);
        // instr = 25'b000_00_001_0011_0000101_000001; // op0, L1, S3, D0, iter1
        // @(posedge clk);
        // instr = 25'b000_00_010_0101_0001010_000011; // op0, L2, S5, D0, iter3

////////////////////////////////////////////////////////////////


        @(posedge clk);
        rep_valid = 1'b0;
        trans_valid = 1'b0;
        @(posedge clk);
        @(posedge clk);
        @(posedge clk);
        activation = 1'b1;
        @(posedge clk);
        activation = 1'b0;
        
        #64000;  //WO delay
        $stop;
    end

endmodule