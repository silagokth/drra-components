module agu_RTR_tb #(

    localparam RESOURCE_INSTR_WIDTH = 25,
    localparam INSTR_OPCODE_BITWIDTH = 3,
    localparam ADDRESS_WIDTH = 8,
    localparam REP_ITER_WIDTH = 6,
    localparam REP_DELAY_WIDTH = 7,
    localparam REP_STEP_WIDTH = 4,
    localparam TRANS_DELAY_WIDTH = 18,
    localparam TRANS_LEVEL_WIDTH = 4,
    localparam NUMBER_OR = 3,    // OR: Outter R-Pattern
    localparam NUMBER_MT = 2,    // MT: Middle T-Pattern
    localparam NUMBER_IR = 0     // IR: Inner  R-Pattern
) ();

    logic clk;
    logic rst_n;
    logic activation;
    logic instr_en;
    logic [RESOURCE_INSTR_WIDTH-1:0] instr;
    logic address_valid;
    logic [ADDRESS_WIDTH-1:0] address;

    agu_RTR # (
        .RESOURCE_INSTR_WIDTH(RESOURCE_INSTR_WIDTH),
        .INSTR_OPCODE_BITWIDTH(INSTR_OPCODE_BITWIDTH),
        .ADDRESS_WIDTH(ADDRESS_WIDTH),
        .REP_ITER_WIDTH(REP_ITER_WIDTH),
        .REP_DELAY_WIDTH(REP_DELAY_WIDTH),
        .REP_STEP_WIDTH(REP_STEP_WIDTH),
        .TRANS_DELAY_WIDTH(TRANS_DELAY_WIDTH),
        .TRANS_LEVEL_WIDTH(TRANS_LEVEL_WIDTH),
        .NUMBER_OR(NUMBER_OR),
        .NUMBER_MT(NUMBER_MT),
        .NUMBER_IR(NUMBER_IR)
    ) agu_RTR_inst (
        .clk(clk),
        .rst_n(rst_n),
        .activation(activation),
        .instr_en(instr_en),
        .instr(instr),
        .address_valid(address_valid),
        .address(address)
    );

    always #5 clk = ~clk;

    initial begin
        clk = 1'b0;
        rst_n = 1'b0;
        activation = 1'b0;
        instr_en = 1'b0;

        #17;
        rst_n = 1'b1;

/////////////////////////////////// IR != 0 => RTR, TR, RR, R ///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////// Delay == 0
        // @(negedge clk);
        // instr_en = 1'b1;

        // // trans_level_delay
        // @(negedge clk);
        // instr = 25'b010_0001_000000000000000000;
        // @(negedge clk);
        // instr = 25'b010_0000_000000000000000000;

        // // rep_option_level_step_delay_iter
        // @(negedge clk);
        // instr = 25'b000_00_000_0011_0000000_000010; // op0, L0, S3, D0, iter2 
        // @(negedge clk);
        // instr = 25'b000_00_001_0001_0000000_000011; // op0, L1, S1, D0, iter3
        // @(negedge clk);
        // instr = 25'b000_00_010_0010_0000000_000001; // op0, L2, S2, D0, iter1
        // @(negedge clk);
        // instr = 25'b000_01_000_0010_0000000_000010; // op1, L0, S2, D0, iter2
        // @(negedge clk);
        // instr = 25'b000_01_001_0011_0000000_000011; // op1, L1, S3, D0, iter3
        // @(negedge clk);
        // instr = 25'b000_01_010_0101_0000000_000001; // op1, L2, S5, D0, iter1
        // @(negedge clk);
        // instr = 25'b000_10_000_0111_0000000_000001; // op2, L0, S7, D0, iter1
        // @(negedge clk);
        // instr = 25'b000_10_001_0001_0000000_000010; // op2, L1, S1, D0, iter2
        // @(negedge clk);
        // instr = 25'b000_10_010_0011_0000000_000011; // op2, L2, S3, D0, iter3

        // // rep_option_level_step_delay_iter
        // @(negedge clk);
        // instr = 25'b000_00_011_1010_0000000_000010; // op0, L3, S10, D0, iter2
        // @(negedge clk);
        // instr = 25'b000_00_100_0011_0000000_000001; // op0, L4, S3, D0, iter1
        // @(negedge clk);
        // instr = 25'b000_00_101_0101_0000000_000001; // op0, L5, S5, D0, iter1

////////////////////////////////////////////////////////////////////////////////////// Delay != 0
        // @(negedge clk);
        // instr_en = 1'b1;

        // // trans_level_delay
        // @(negedge clk);
        // instr = 25'b010_0001_000000000000000011;
        // @(negedge clk);
        // instr = 25'b010_0000_000000000000000111;

        // // rep_option_level_step_delay_iter
        // @(negedge clk);
        // instr = 25'b000_00_000_0011_0000010_000010; // op0, L0, S3, D2, iter2 
        // @(negedge clk);
        // instr = 25'b000_00_001_0001_0000001_000011; // op0, L1, S1, D1, iter3
        // @(negedge clk);
        // instr = 25'b000_00_010_0010_0000011_000001; // op0, L2, S2, D3, iter1
        // @(negedge clk);
        // instr = 25'b000_01_000_0010_0000000_000010; // op1, L0, S2, D0, iter2
        // @(negedge clk);
        // instr = 25'b000_01_001_0011_0000000_000011; // op1, L1, S3, D0, iter3
        // @(negedge clk);
        // instr = 25'b000_01_010_0101_0000000_000001; // op1, L2, S5, D0, iter1
        // @(negedge clk);
        // instr = 25'b000_10_000_0111_0000000_000001; // op2, L0, S7, D0, iter1
        // @(negedge clk);
        // instr = 25'b000_10_001_0001_0000000_000010; // op2, L1, S1, D0, iter2
        // @(negedge clk);
        // instr = 25'b000_10_010_0011_0000000_000011; // op2, L2, S3, D0, iter3

        // // rep_option_level_step_delay_iter
        // @(negedge clk);
        // instr = 25'b000_00_011_1010_0000000_000010; // op0, L3, S10, D0, iter2
        // @(negedge clk);
        // instr = 25'b000_00_100_0011_0000101_000001; // op0, L4, S3, D0, iter1
        // @(negedge clk);
        // instr = 25'b000_00_101_0101_0000000_000001; // op0, L5, S5, D0, iter1



////////////////////////////////////// IR == 0 => RT, T, 0 //////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////// Delay == 0
        // @(negedge clk);
        // instr_en = 1'b1;

        // // trans_level_delay
        // @(negedge clk);
        // instr = 25'b010_0001_000000000000000000;
        // @(negedge clk);
        // instr = 25'b010_0000_000000000000000000;
        
        // // rep_option_level_step_delay_iter
        // @(negedge clk);
        // instr = 25'b000_00_000_1010_0000000_000010; // op0, L0, S10, D0, iter2
        // @(negedge clk);
        // instr = 25'b000_00_001_0011_0000000_000001; // op0, L1, S3, D0, iter1
        // @(negedge clk);
        // instr = 25'b000_00_010_0101_0000000_000011; // op0, L2, S5, D0, iter3

////////////////////////////////////////////////////////////////////////////////////// Delay != 0
        @(negedge clk);
        instr_en = 1'b1;

        // trans_level_delay
        @(negedge clk);
        instr = 25'b010_0001_000000000000000011;
        @(negedge clk);
        instr = 25'b010_0000_000000000000000111;

        // rep_option_level_step_delay_iter
        @(negedge clk);
        instr = 25'b000_00_000_1010_0000010_000010; // op0, L0, S10, D0, iter2
        @(negedge clk);
        instr = 25'b000_00_001_0011_0000101_000001; // op0, L1, S3, D0, iter1
        @(negedge clk);
        instr = 25'b000_00_010_0101_0001010_000011; // op0, L2, S5, D0, iter3

////////////////////////////////////////////////////////////////


        @(negedge clk);
        instr_en = 1'b0;
        @(negedge clk);
        @(negedge clk);
        @(negedge clk);
        activation = 1'b1;
        @(negedge clk);
        activation = 1'b0;
        
        #16000;
        $stop;
    end

endmodule