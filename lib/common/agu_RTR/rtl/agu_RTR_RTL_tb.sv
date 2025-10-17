// vesyla_template_start defines
`define {{name}} {{name}}_{{fingerprint}}
`define {{name}}_pkg {{name}}_{{fingerprint}}_pkg
// vesyla_template_end defines

// vesyla_template_start module_head
{% if not already_defined %}
module {{name}}_{{fingerprint}}
import {{name}}_{{fingerprint}}_pkg::*;
// vesyla_template_end module_head
(
    input  logic clk_0,
    input  logic rst_n_0,
    input  logic instr_en_0,
    input  logic [RESOURCE_INSTR_WIDTH-1:0] instr_0,
    input  logic [3:0] activate_0,
    input  logic [WORD_BITWIDTH-1:0] word_data_in_0,
    output logic [WORD_BITWIDTH-1:0] word_data_out_0,
    input  logic [BULK_BITWIDTH-1:0] bulk_data_in_0,
    output logic [BULK_BITWIDTH-1:0] bulk_data_out_0,
    input  logic clk_1,
    input  logic rst_n_1,
    input  logic instr_en_1,
    input  logic [RESOURCE_INSTR_WIDTH-1:0] instr_1,
    input  logic [3:0] activate_1,
    input  logic [WORD_BITWIDTH-1:0] word_data_in_1,
    output logic [WORD_BITWIDTH-1:0] word_data_out_1,
    input  logic [BULK_BITWIDTH-1:0] bulk_data_in_1,
    output logic [BULK_BITWIDTH-1:0] bulk_data_out_1
);

    logic clk, rst_n, instruction_valid, activate;
    logic [INSTRUCTION_PAYLOAD_WIDTH-1:0] instruction;
    assign clk = clk_0;
    assign rst_n = rst_n_0;
    assign instruction_valid = instr_en_0;
    assign activate = activate_0[0];
    assign instruction = instr_0;
    // TODO: check how to drive this reset_fsm
    logic reset_fsm;

    // useless output
    assign word_data_out_1 = 0;
    assign bulk_data_out_0 = 0;
    assign bulk_data_out_1 = 0;

    // register inputs
    logic [WORD_BITWIDTH-1:0] in0, in1, out0;
    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            in0 <= 0;
            in1 <= 0;
            reset_fsm <= 1;
        end else begin
            in0 <= word_data_in_0;
            in1 <= word_data_in_1;
            reset_fsm <= 0;
        end

        for (int i = 0; i < NUMBER_MT; i++) begin
            trans_delay[i]  = 0;
            trans_config[i] = 1'b0;
        end
        for (int i = 0; i < NUMBER_OR; i++) begin
            rep_delay_OR[i] = 0;
            rep_iter_OR[i]  = 0;
            rep_step_OR[i]  = 0;
            rep_config_OR[i]= 1'b0;
        end

        #17;
        rst_n = 1'b1;


/////////////////////////////////// IR != 0 => RTR, TR, RR, R ///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////// Delay == 0


        // // IR0
        // rep_valid = 1'b1;
        // rep_step_IR[0] = 3;  rep_delay_IR[0] = 0; rep_iter_IR[0] = 2; rep_config_IR[0] = 1'b1; // L0, S3,  D0, iter2
        // rep_step_IR[1] = 1;  rep_delay_IR[1] = 0; rep_iter_IR[1] = 3; rep_config_IR[1] = 1'b1; // L1, S1,  D0, iter3
        // rep_step_IR[2] = 2;  rep_delay_IR[2] = 0; rep_iter_IR[2] = 1; rep_config_IR[2] = 1'b1; // L2, S2,  D0, iter1
        // rep_step_IR[3] = 10; rep_delay_IR[3] = 0; rep_iter_IR[3] = 2; rep_config_IR[3] = 1'b1; // L3, S10, D0, iter2
    
        // // MT0
        // trans_valid = 1'b1;
        // trans_delay[0] = 0; trans_config[0] = 1'b1;

        // // IR1
        // rep_step_IR[4] = 2; rep_delay_IR[4] = 0; rep_iter_IR[4] = 2; rep_config_IR[4] = 1'b1; // L0, S2, D0, iter2
        // rep_step_IR[5] = 3; rep_delay_IR[5] = 0; rep_iter_IR[5] = 3; rep_config_IR[5] = 1'b1; // L1, S3, D0, iter3
        // rep_step_IR[6] = 5; rep_delay_IR[6] = 0; rep_iter_IR[6] = 1; rep_config_IR[6] = 1'b1; // L2, S5, D0, iter1
        // rep_step_IR[7] = 4; rep_delay_IR[7] = 0; rep_iter_IR[7] = 1; rep_config_IR[7] = 1'b1; // L3, S4, D0, iter1

        // // MT1
        // trans_delay[1] = 0; trans_config[1] = 1'b1;

        // // IR2
        // rep_step_IR[8]  = 7; rep_delay_IR[8]  = 0; rep_iter_IR[8]  = 1; rep_config_IR[8]  = 1'b1; // L0, S7, D0, iter1
        // rep_step_IR[9]  = 1; rep_delay_IR[9]  = 0; rep_iter_IR[9]  = 2; rep_config_IR[9]  = 1'b1; // L1, S1, D0, iter2
        // rep_step_IR[10] = 3; rep_delay_IR[10] = 0; rep_iter_IR[10] = 3; rep_config_IR[10] = 1'b1; // L2, S3, D0, iter3
        // rep_step_IR[11] = 5; rep_delay_IR[11] = 0; rep_iter_IR[11] = 2; rep_config_IR[11] = 1'b1; // L3, S5, D0, iter2

        // // MT2
        // trans_delay[2] = 0; trans_config[2] = 1'b1;

        // // IR3
        // rep_step_IR[12] = 1; rep_delay_IR[12] = 0; rep_iter_IR[12] = 5; rep_config_IR[12] = 1'b1; // L0, S1, D0, iter5
        // rep_step_IR[13] = 6; rep_delay_IR[13] = 0; rep_iter_IR[13] = 1; rep_config_IR[13] = 1'b1; // L1, S6, D0, iter1
        // rep_step_IR[14] = 0; rep_delay_IR[14] = 0; rep_iter_IR[14] = 2; rep_config_IR[14] = 1'b1; // L2, S0, D0, iter2
        // rep_step_IR[15] = 5; rep_delay_IR[15] = 0; rep_iter_IR[15] = 1; rep_config_IR[15] = 1'b1; // L3, S5, D0, iter1

        // // OR
        // rep_step_OR[0] = 10; rep_delay_OR[0] = 0; rep_iter_OR[0] = 2; rep_config_OR[0] = 1'b1; // L4, S10, D0, iter2
        // rep_step_OR[1] = 3;  rep_delay_OR[1] = 0; rep_iter_OR[1] = 1; rep_config_OR[1] = 1'b1; // L5, S3, D0, iter1
        // rep_step_OR[2] = 5;  rep_delay_OR[2] = 0; rep_iter_OR[2] = 1; rep_config_OR[2] = 1'b1; // L6, S5, D0, iter1
        // rep_step_OR[3] = 1;  rep_delay_OR[3] = 0; rep_iter_OR[3] = 1; rep_config_OR[3] = 1'b1; // L7, S1, D0, iter1

////////////////////////////////////////////////////////////////////////////////////// Delay != 0


        // IR0
        rep_valid = 1'b1;
        rep_step_IR[0] = 3;  rep_delay_IR[0] = 2; rep_iter_IR[0] = 2; rep_config_IR[0] = 1'b1; // L0, S3,  D0, iter2
        rep_step_IR[1] = 1;  rep_delay_IR[1] = 1; rep_iter_IR[1] = 3; rep_config_IR[1] = 1'b1; // L1, S1,  D0, iter3
        rep_step_IR[2] = 2;  rep_delay_IR[2] = 3; rep_iter_IR[2] = 1; rep_config_IR[2] = 1'b1; // L2, S2,  D0, iter1
        rep_step_IR[3] = 10; rep_delay_IR[3] = 0; rep_iter_IR[3] = 2; rep_config_IR[3] = 1'b1; // L3, S10, D0, iter2
    
        // MT0
        trans_valid = 1'b1;
        trans_delay[0] = 7; trans_config[0] = 1'b1;

        // IR1
        rep_step_IR[4] = 2; rep_delay_IR[4] = 0; rep_iter_IR[4] = 2; rep_config_IR[4] = 1'b1; // L0, S2, D0, iter2
        rep_step_IR[5] = 3; rep_delay_IR[5] = 0; rep_iter_IR[5] = 3; rep_config_IR[5] = 1'b1; // L1, S3, D0, iter3
        rep_step_IR[6] = 5; rep_delay_IR[6] = 0; rep_iter_IR[6] = 1; rep_config_IR[6] = 1'b1; // L2, S5, D0, iter1
        rep_step_IR[7] = 4; rep_delay_IR[7] = 3; rep_iter_IR[7] = 1; rep_config_IR[7] = 1'b1; // L3, S4, D0, iter1

        // MT1
        trans_delay[1] = 3; trans_config[1] = 1'b1;

        // IR2
        rep_step_IR[8]  = 7; rep_delay_IR[8]  = 0; rep_iter_IR[8]  = 1; rep_config_IR[8]  = 1'b1; // L0, S7, D0, iter1
        rep_step_IR[9]  = 1; rep_delay_IR[9]  = 0; rep_iter_IR[9]  = 2; rep_config_IR[9]  = 1'b1; // L1, S1, D0, iter2
        rep_step_IR[10] = 3; rep_delay_IR[10] = 0; rep_iter_IR[10] = 3; rep_config_IR[10] = 1'b1; // L2, S3, D0, iter3
        rep_step_IR[11] = 5; rep_delay_IR[11] = 0; rep_iter_IR[11] = 2; rep_config_IR[11] = 1'b1; // L3, S5, D0, iter2

        // MT2
        trans_delay[2] = 0; trans_config[2] = 1'b1;

        // IR3
        rep_step_IR[12] = 1; rep_delay_IR[12] = 1; rep_iter_IR[12] = 5; rep_config_IR[12] = 1'b1; // L0, S1, D0, iter5
        rep_step_IR[13] = 6; rep_delay_IR[13] = 3; rep_iter_IR[13] = 1; rep_config_IR[13] = 1'b1; // L1, S6, D0, iter1
        rep_step_IR[14] = 0; rep_delay_IR[14] = 7; rep_iter_IR[14] = 2; rep_config_IR[14] = 1'b1; // L2, S0, D0, iter2
        rep_step_IR[15] = 5; rep_delay_IR[15] = 9; rep_iter_IR[15] = 1; rep_config_IR[15] = 1'b1; // L3, S5, D0, iter1

        // OR
        rep_step_OR[0] = 10; rep_delay_OR[0] = 0; rep_iter_OR[0] = 2; rep_config_OR[0] = 1'b1; // L4, S10, D0, iter2
        rep_step_OR[1] = 3;  rep_delay_OR[1] = 5; rep_iter_OR[1] = 1; rep_config_OR[1] = 1'b1; // L5, S3, D0, iter1
        rep_step_OR[2] = 5;  rep_delay_OR[2] = 0; rep_iter_OR[2] = 1; rep_config_OR[2] = 1'b1; // L6, S5, D0, iter1
        rep_step_OR[3] = 1;  rep_delay_OR[3] = 9; rep_iter_OR[3] = 1; rep_config_OR[3] = 1'b1; // L7, S1, D0, iter1


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
        activation = 1'b1;
        @(posedge clk);
        @(negedge clk);
        activation = 1'b0;
        
        #64000;  //WO delay
        $stop;
    end

  logic [$clog2(FSM_MAX_STATES)-1:0][DPU_MODE_WIDTH-1:0] mode_memory;
  logic [$clog2(FSM_MAX_STATES)-1:0][DPU_IMMEDIATE_WIDTH-1:0] immediate_memory;
  logic [$clog2(FSM_MAX_STATES)-1:0] fsm_option;
  logic [$clog2(FSM_MAX_STATES)-1:0] fsm_max_init_state;
  logic [FSM_MAX_STATES-2:0][FSM_DELAY_WIDTH-1:0] fsm_delays;
  logic [DPU_IMMEDIATE_WIDTH-1:0] immediate;
  logic [DPU_MODE_WIDTH-1:0] mode;
  logic [OPCODE_WIDTH-1:0] opcode;
  logic dpu_valid;
  logic fsm_valid;
  logic reset_accumulator;

  dpu_t dpu;
  fsm_t fsm;

  fsm #(
      .FSM_MAX_STATES(FSM_MAX_STATES),
      .FSM_DELAY_WIDTH(FSM_DELAY_WIDTH)
  ) fsm_inst (
      .clk(clk),
      .rst_n(rst_n),
      .activate(|activate),
      .fsm_delays(fsm_delays),
      .max_init_state(fsm_max_init_state),
      .reset_fsm(reset_fsm),
      .state(fsm_option)
  );

  assign opcode = instruction[OPCODE_H:OPCODE_L];
  assign dpu_valid = instruction_valid && (opcode == OPCODE_DPU);
  assign fsm_valid = instruction_valid && (opcode == OPCODE_FSM);
  assign dpu = dpu_valid ? unpack_dpu(
          instruction[INSTRUCTION_PAYLOAD_WIDTH-1:0]
      ) :
      '{default: 0};
  assign fsm = fsm_valid ? unpack_fsm(
          instruction[INSTRUCTION_PAYLOAD_WIDTH-1:0]
      ) :
      '{default: 0};

  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      immediate_memory <= '0;
      mode_memory <= '0;
      fsm_max_init_state <= '0;
    end else begin
      if (dpu_valid) begin
        mode_memory[dpu._option] <= dpu._mode;
        immediate_memory[dpu._option] <= dpu._immediate;
        fsm_max_init_state <= dpu._option;
      end
    end
  end

  // TODO: we need to make the number of delays parametric somehow
  assign fsm_delays[0] = fsm._delay_0;
  assign fsm_delays[1] = fsm._delay_1;
  assign fsm_delays[2] = fsm._delay_2;

  assign mode = mode_memory[fsm_option];
  assign immediate = immediate_memory[fsm_option];

  logic signed [BITWIDTH-1:0] acc0, acc0_next;
  logic signed [BITWIDTH-1:0] adder_in0;
  logic signed [BITWIDTH-1:0] adder_in1;
  logic signed [BITWIDTH-1:0] adder_out;
  logic signed [BITWIDTH-1:0] mult_in0;
  logic signed [BITWIDTH-1:0] mult_in1;
  logic signed [BITWIDTH-1:0] mult_out;

  // signed saturateion
  localparam logic signed [BITWIDTH-1:0] MAX_RESULT = 2 ** (BITWIDTH - 1) - 1;
  localparam logic signed [BITWIDTH-1:0] MIN_RESULT = -2 ** (BITWIDTH - 1);


  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      acc0 <= '0;
    end else begin
      if (reset_accumulator) begin
        acc0 <= '0;
      end else begin
        acc0 <= acc0_next;
      end
    end
  end

  always_comb begin
    out0 = 0;
    adder_in0 = 0;
    adder_in1 = 0;
    mult_in0 = 0;
    mult_in1 = 0;
    acc0_next = 0;

    case (mode)
      DPU_MODE_ADD: begin
        adder_in0 = in0;
        adder_in1 = in1;
        out0 = adder_out;
      end
      DPU_MODE_MAC: begin
        mult_in0 = in0;
        mult_in1 = in1;
        adder_in0 = mult_out;
        adder_in1 = acc0;
        acc0_next = adder_out;
        out0 = adder_out;
      end
      DPU_MODE_MUL: begin
        mult_in0 = in0;
        mult_in1 = in1;
        out0 = mult_out;
      end
    endcase
  end

  // in future we will use ChipWare blocks for pipelined and such 
  {{name}}_{{fingerprint}}_adder adder_inst (
      .in1(adder_in0),
      .in2(adder_in1),
      .saturate(1'b1),
      .out(adder_out)
  );

  {{name}}_{{fingerprint}}_multiplier mult_inst (
      .in1(mult_in0),
      .in2(mult_in1),
      .saturate(1'b1),
      .out(mult_out)
  );

  assign word_data_out_0 = out0;

endmodule

// vesyla_template_start module_tail
{% endif %}
// vesyla_template_end module_tail
//module agu_RTR_tb #(
//
//    localparam ADDRESS_WIDTH = 8,
//
//    localparam REP_LEVEL_WIDTH = 4,
//    localparam REP_DELAY_WIDTH = 6,
//    localparam REP_ITER_WIDTH = 6,
//    localparam REP_STEP_WIDTH = 6,
//    localparam TRANS_LEVEL_WIDTH = 4,
//    localparam TRANS_DELAY_WIDTH = 12,
//    
//    localparam NUMBER_OR = 4,    // OR: Outter R-Pattern
//    localparam NUMBER_MT = 3,    // MT: Middle T-Pattern
//    localparam NUMBER_IR = 4     // IR: Inner  R-Pattern
//) ();
//
//    logic                         clk;
//    logic                         rst_n;
//    logic                         activation;
//    logic                         rep_valid;
//    logic                         repx_valid;
//    logic [REP_DELAY_WIDTH-1:0]   rep_delay_IR  [(NUMBER_MT+1)*NUMBER_IR];
//    logic [REP_ITER_WIDTH-1:0]    rep_iter_IR   [(NUMBER_MT+1)*NUMBER_IR];
//    logic [REP_STEP_WIDTH-1:0]    rep_step_IR   [(NUMBER_MT+1)*NUMBER_IR];
//    logic                         rep_config_IR [(NUMBER_MT+1)*NUMBER_IR];
//    logic                         trans_valid; 
//    logic [TRANS_DELAY_WIDTH-1:0] trans_delay   [NUMBER_MT];
//    logic                         trans_config  [NUMBER_MT];
//    logic [REP_DELAY_WIDTH-1:0]   rep_delay_OR  [NUMBER_OR];
//    logic [REP_ITER_WIDTH-1:0]    rep_iter_OR   [NUMBER_OR];
//    logic [REP_STEP_WIDTH-1:0]    rep_step_OR   [NUMBER_OR];
//    logic                         rep_config_OR [NUMBER_OR];
//
//    logic                         address_valid;
//    logic [ADDRESS_WIDTH-1:0]     address;
//
//    agu_RTR # (
//        .ADDRESS_WIDTH(ADDRESS_WIDTH),
//        .REP_LEVEL_WIDTH(REP_LEVEL_WIDTH),
//        .REP_DELAY_WIDTH(REP_DELAY_WIDTH),
//        .REP_ITER_WIDTH(REP_ITER_WIDTH),
//        .REP_STEP_WIDTH(REP_STEP_WIDTH),
//        .TRANS_LEVEL_WIDTH(TRANS_LEVEL_WIDTH),
//        .TRANS_DELAY_WIDTH(TRANS_DELAY_WIDTH),
//        .NUMBER_OR(NUMBER_OR),
//        .NUMBER_MT(NUMBER_MT),
//        .NUMBER_IR(NUMBER_IR)
//    ) agu_RTR_inst (
//        .clk(clk),
//        .rst_n(rst_n),
//        .activation(activation),
//        .rep_valid(rep_valid),
//        .repx_valid(repx_valid),
//        .rep_delay_IR(rep_delay_IR),
//        .rep_iter_IR(rep_iter_IR),
//        .rep_step_IR(rep_step_IR),
//        .rep_config_IR(rep_config_IR),
//        .trans_valid(trans_valid),
//        .trans_delay(trans_delay),
//        .trans_config(trans_config),
//        .rep_delay_OR(rep_delay_OR),
//        .rep_iter_OR(rep_iter_OR),
//        .rep_step_OR(rep_step_OR),
//        .rep_config_OR(rep_config_OR),
//        .address_valid(address_valid),
//        .address(address)
//    );
//
//    always #5 clk = ~clk;
//
//    initial begin
//        clk = 1'b0;
//        rst_n = 1'b0;
//        activation = 1'b0;
//        rep_valid = 1'b0;
//        repx_valid = 1'b0;
//        trans_valid = 1'b0;
//        for (int i = 0; i < (NUMBER_MT+1)*NUMBER_IR; i++) begin
//            rep_delay_IR[i] = 0;
//            rep_iter_IR[i]  = 0;
//            rep_step_IR[i]  = 0;
//            rep_config_IR[i]= 1'b0;
//        end
//        for (int i = 0; i < NUMBER_MT; i++) begin
//            trans_delay[i]  = 0;
//            trans_config[i] = 1'b0;
//        end
//        for (int i = 0; i < NUMBER_OR; i++) begin
//            rep_delay_OR[i] = 0;
//            rep_iter_OR[i]  = 0;
//            rep_step_OR[i]  = 0;
//            rep_config_OR[i]= 1'b0;
//        end
//
//        #17;
//        rst_n = 1'b1;
//
//
///////////////////////////////////// IR != 0 => RTR, TR, RR, R ///////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////// Delay == 0
//
//
//        // IR0
//        rep_valid = 1'b1;
//        rep_step_IR[0] = 3;  rep_delay_IR[0] = 0; rep_iter_IR[0] = 2; rep_config_IR[0] = 1'b1; // L0, S3,  D0, iter2
//        rep_step_IR[1] = 1;  rep_delay_IR[1] = 0; rep_iter_IR[1] = 3; rep_config_IR[1] = 1'b1; // L1, S1,  D0, iter3
//        rep_step_IR[2] = 2;  rep_delay_IR[2] = 0; rep_iter_IR[2] = 1; rep_config_IR[2] = 1'b1; // L2, S2,  D0, iter1
//        rep_step_IR[3] = 10; rep_delay_IR[3] = 0; rep_iter_IR[3] = 2; rep_config_IR[3] = 1'b1; // L3, S10, D0, iter2
//
//        // MT0
//        trans_valid = 1'b1;
//        trans_delay[0] = 0; trans_config[0] = 1'b1;
//
//        // IR1
//        rep_step_IR[4] = 2; rep_delay_IR[4] = 0; rep_iter_IR[4] = 2; rep_config_IR[4] = 1'b1; // L0, S2, D0, iter2
//        rep_step_IR[5] = 3; rep_delay_IR[5] = 0; rep_iter_IR[5] = 3; rep_config_IR[5] = 1'b1; // L1, S3, D0, iter3
//        rep_step_IR[6] = 5; rep_delay_IR[6] = 0; rep_iter_IR[6] = 1; rep_config_IR[6] = 1'b1; // L2, S5, D0, iter1
//        rep_step_IR[7] = 4; rep_delay_IR[7] = 0; rep_iter_IR[7] = 1; rep_config_IR[7] = 1'b1; // L3, S4, D0, iter1
//
//        // MT1
//        trans_delay[1] = 0; trans_config[1] = 1'b1;
//
//        // IR2
//        rep_step_IR[8]  = 7; rep_delay_IR[8]  = 0; rep_iter_IR[8]  = 1; rep_config_IR[8]  = 1'b1; // L0, S7, D0, iter1
//        rep_step_IR[9]  = 1; rep_delay_IR[9]  = 0; rep_iter_IR[9]  = 2; rep_config_IR[9]  = 1'b1; // L1, S1, D0, iter2
//        rep_step_IR[10] = 3; rep_delay_IR[10] = 0; rep_iter_IR[10] = 3; rep_config_IR[10] = 1'b1; // L2, S3, D0, iter3
//        rep_step_IR[11] = 5; rep_delay_IR[11] = 0; rep_iter_IR[11] = 2; rep_config_IR[11] = 1'b1; // L3, S5, D0, iter2
//
//        // MT2
//        trans_delay[2] = 0; trans_config[2] = 1'b1;
//
//        // IR3
//        rep_step_IR[12] = 1; rep_delay_IR[12] = 0; rep_iter_IR[12] = 5; rep_config_IR[12] = 1'b1; // L0, S1, D0, iter5
//        rep_step_IR[13] = 6; rep_delay_IR[13] = 0; rep_iter_IR[13] = 1; rep_config_IR[13] = 1'b1; // L1, S6, D0, iter1
//        rep_step_IR[14] = 0; rep_delay_IR[14] = 0; rep_iter_IR[14] = 2; rep_config_IR[14] = 1'b1; // L2, S0, D0, iter2
//        rep_step_IR[15] = 5; rep_delay_IR[15] = 0; rep_iter_IR[15] = 1; rep_config_IR[15] = 1'b1; // L3, S5, D0, iter1
//
//        // OR
//        rep_step_OR[0] = 10; rep_delay_OR[0] = 0; rep_iter_OR[0] = 2; rep_config_OR[0] = 1'b1; // L4, S10, D0, iter2
//        rep_step_OR[1] = 3;  rep_delay_OR[1] = 0; rep_iter_OR[1] = 1; rep_config_OR[1] = 1'b1; // L5, S3, D0, iter1
//        rep_step_OR[2] = 5;  rep_delay_OR[2] = 0; rep_iter_OR[2] = 1; rep_config_OR[2] = 1'b1; // L6, S5, D0, iter1
//        rep_step_OR[3] = 1;  rep_delay_OR[3] = 0; rep_iter_OR[3] = 1; rep_config_OR[3] = 1'b1; // L7, S1, D0, iter1
//
//////////////////////////////////////////////////////////////////////////////////////// Delay != 0
//        // // Must be issued one cycle before the first rep instruction, to go to the CNFG state
//        // @(posedge clk);
//        // rep_valid = 1'b1;
//
//        // // IR0
//        // @(posedge clk);
//        // rep_level = 0; rep_step = 3;  rep_delay = 2; rep_iter = 2; // L0, S3,  D2, iter2
//        // @(posedge clk);  
//        // rep_level = 1; rep_step = 1;  rep_delay = 1; rep_iter = 3; // L1, S1,  D1, iter3
//        // @(posedge clk);  
//        // rep_level = 2; rep_step = 2;  rep_delay = 3; rep_iter = 1; // L2, S2,  D3, iter1
//        // @(posedge clk);
//        // rep_level = 3; rep_step = 10; rep_delay = 0; rep_iter = 2; // L3, S10, D0, iter2
//
//        // // MT0
//        // @(posedge clk);
//        // rep_valid = 1'b0;
//        // trans_valid = 1'b1;
//        // trans_level = 0; trans_delay = 7;
//
//        // // IR1
//        // @(posedge clk);
//        // rep_valid = 1'b1;
//        // trans_valid = 1'b0;
//        // rep_level = 0; rep_step = 2; rep_delay = 0; rep_iter = 2; // L0, S2, D0, iter2
//        // @(posedge clk);
//        // rep_level = 1; rep_step = 3; rep_delay = 0; rep_iter = 3; // L1, S3, D0, iter3
//        // @(posedge clk);
//        // rep_level = 2; rep_step = 5; rep_delay = 0; rep_iter = 1; // L2, S5, D0, iter1
//        // @(posedge clk);
//        // rep_level = 3; rep_step = 4; rep_delay = 3; rep_iter = 1; // L3, S4, D3, iter1
//
//        // // MT1
//        // @(posedge clk);
//        // rep_valid = 1'b0;
//        // trans_valid = 1'b1;
//        // trans_level = 1; trans_delay = 3;
//
//        // // IR2
//        // @(posedge clk);
//        // rep_valid = 1'b1;
//        // trans_valid = 1'b0;
//        // rep_level = 0; rep_step = 7; rep_delay = 0; rep_iter = 1; // L0, S7, D0, iter1
//        // @(posedge clk);
//        // rep_level = 1; rep_step = 1; rep_delay = 0; rep_iter = 2; // L1, S1, D0, iter2
//        // @(posedge clk);
//        // rep_level = 2; rep_step = 3; rep_delay = 0; rep_iter = 3; // L2, S3, D0, iter3
//        // @(posedge clk);
//        // rep_level = 3; rep_step = 5; rep_delay = 0; rep_iter = 2; // L3, S5, D0, iter2
//
//        // // MT2
//        // @(posedge clk);
//        // rep_valid = 1'b0;
//        // trans_valid = 1'b1;
//        // trans_level = 2; trans_delay = 0;
//
//        // // IR3
//        // @(posedge clk);
//        // rep_valid = 1'b1;
//        // trans_valid = 1'b0;
//        // rep_level = 0; rep_step = 1; rep_delay = 1; rep_iter = 5; // L0, S1, D1, iter5
//        // @(posedge clk);
//        // rep_level = 1; rep_step = 6; rep_delay = 3; rep_iter = 1; // L1, S6, D3, iter1
//        // @(posedge clk);
//        // rep_level = 2; rep_step = 0; rep_delay = 7; rep_iter = 2; // L2, S0, D7, iter2
//        // @(posedge clk);
//        // rep_level = 3; rep_step = 5; rep_delay = 9; rep_iter = 1; // L3, S5, D9, iter1
// 
//        // // OR
//        // @(posedge clk);
//        // rep_level = 4; rep_step = 10; rep_delay = 0; rep_iter = 2; // L4, S10, D0, iter2
//        // @(posedge clk);
//        // rep_level = 5; rep_step = 3;  rep_delay = 5; rep_iter = 1; // L5, S3,  D5, iter1
//        // @(posedge clk); 
//        // rep_level = 6; rep_step = 5;  rep_delay = 0; rep_iter = 1; // L6, S5,  D0, iter1
//        // @(posedge clk); 
//        // rep_level = 7; rep_step = 1;  rep_delay = 9; rep_iter = 1; // L7, S1,  D9, iter1
//
//
//////////////////////////////////////// IR == 0 => RT, T, 0 //////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////// Delay == 0
//        // Must be issued one cycle before the first rep instruction, to go to the CNFG state
//        // @(posedge clk);
//        // trans_valid = 1'b1;
//
//        // // MT0
//        // @(posedge clk);
//        // trans_level = 0; trans_delay = 0;
//
//        // // MT1
//        // @(posedge clk);
//        // trans_level = 1; trans_delay = 0;
//
//        // // MT2
//        // @(posedge clk);
//        // trans_level = 2; trans_delay = 0;
//
//        // // OR
//        // @(posedge clk);
//        // rep_valid = 1'b1;
//        // trans_valid = 1'b0;
//        // rep_level = 0; rep_step = 10; rep_delay = 0; rep_iter = 2; // L0, S10, D0, iter2
//        // @(posedge clk);
//        // rep_level = 1; rep_step = 3;  rep_delay = 0; rep_iter = 1; // L1, S3,  D0, iter1
//        // @(posedge clk); 
//        // rep_level = 2; rep_step = 5;  rep_delay = 0; rep_iter = 3; // L2, S5,  D0, iter3
//        // @(posedge clk); 
//        // rep_level = 3; rep_step = 1;  rep_delay = 0; rep_iter = 1; // L3, S1,  D0, iter1
//
//////////////////////////////////////////////////////////////////////////////////////// Delay != 0
//        // @(posedge clk);
//        // instr_en = 1'b1;
//
//        // // trans_level_delay
//        // @(posedge clk);
//        // instr = 25'b010_0001_000000000000000011;
//        // @(posedge clk);
//        // instr = 25'b010_0000_000000000000000111;
//
//        // // rep_option_level_step_delay_iter
//        // @(posedge clk);
//        // instr = 25'b000_00_000_1010_0000010_000010; // op0, L0, S10, D0, iter2
//        // @(posedge clk);
//        // instr = 25'b000_00_001_0011_0000101_000001; // op0, L1, S3, D0, iter1
//        // @(posedge clk);
//        // instr = 25'b000_00_010_0101_0001010_000011; // op0, L2, S5, D0, iter3
//
//////////////////////////////////////////////////////////////////
//
//
//        @(posedge clk);
//        activation = 1'b1;
//        @(posedge clk);
//        activation = 1'b0;
//
//        #64000;  //WO delay
//        $stop;
//    end
//
//endmodule
