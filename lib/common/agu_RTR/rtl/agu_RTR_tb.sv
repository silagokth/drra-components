module agu_RTR_tb #(

    localparam ADDRESS_WIDTH = 8,

    localparam REP_LEVEL_WIDTH = 4,
    localparam REP_DELAY_WIDTH = 6,
    localparam REP_ITER_WIDTH = 6,
    localparam REP_STEP_WIDTH = 6,
    localparam TRANS_LEVEL_WIDTH = 4,
    localparam TRANS_DELAY_WIDTH = 12,

    localparam NUMBER_OR = 2,  // OR: Outter R-Pattern
    localparam NUMBER_MT = 1,  // MT: Middle T-Pattern
    localparam NUMBER_IR = 2   // IR: Inner  R-Pattern
) ();

  logic                         clk;
  logic                         rst_n;
  logic                         activation;
  logic                         rep_valid;
  logic                         repx_valid;
  logic [  REP_DELAY_WIDTH-1:0] rep_delay_IR  [(NUMBER_MT+1)*NUMBER_IR];
  logic [   REP_ITER_WIDTH-1:0] rep_iter_IR   [(NUMBER_MT+1)*NUMBER_IR];
  logic [   REP_STEP_WIDTH-1:0] rep_step_IR   [(NUMBER_MT+1)*NUMBER_IR];
  logic                         rep_config_IR [(NUMBER_MT+1)*NUMBER_IR];
  logic                         trans_valid;
  logic [TRANS_DELAY_WIDTH-1:0] trans_delay   [              NUMBER_MT];
  logic                         trans_config  [              NUMBER_MT];
  logic [  REP_DELAY_WIDTH-1:0] rep_delay_OR  [              NUMBER_OR];
  logic [   REP_ITER_WIDTH-1:0] rep_iter_OR   [              NUMBER_OR];
  logic [   REP_STEP_WIDTH-1:0] rep_step_OR   [              NUMBER_OR];
  logic                         rep_config_OR [              NUMBER_OR];
  logic                         address_valid;
  logic [    ADDRESS_WIDTH-1:0] address;

  agu_RTR #(
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
      .rep_delay_IR(rep_delay_IR),
      .rep_iter_IR(rep_iter_IR),
      .rep_step_IR(rep_step_IR),
      .rep_config_IR(rep_config_IR),
      .trans_valid(trans_valid),
      .trans_delay(trans_delay),
      .trans_config(trans_config),
      .rep_delay_OR(rep_delay_OR),
      .rep_iter_OR(rep_iter_OR),
      .rep_step_OR(rep_step_OR),
      .rep_config_OR(rep_config_OR),
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
    for (int i = 0; i < (NUMBER_MT + 1) * NUMBER_IR; i++) begin
      rep_delay_IR[i]  = 0;
      rep_iter_IR[i]   = 0;
      rep_step_IR[i]   = 0;
      rep_config_IR[i] = 1'b0;
    end
    for (int i = 0; i < NUMBER_MT; i++) begin
      trans_delay[i]  = 0;
      trans_config[i] = 1'b0;
    end
    for (int i = 0; i < NUMBER_OR; i++) begin
      rep_delay_OR[i]  = 0;
      rep_iter_OR[i]   = 0;
      rep_step_OR[i]   = 0;
      rep_config_OR[i] = 1'b0;
    end

    #17;
    rst_n = 1'b1;


    /////////////////////////////////// IR != 0 => RTR, TR, RR, R ///////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////// Delay == 0

    // IR0
    @(posedge clk);
    rep_valid = 1'b1;
    rep_step_IR[0] = 1;
    rep_delay_IR[0] = 0;
    rep_iter_IR[0] = 6'h3f;
    rep_config_IR[0] = 1'b1;  // L0, S3,  D0, iter2

    @(posedge clk);
    rep_valid = 1'b0;
    rep_step_IR[0] = 0;
    rep_delay_IR[0] = 0;
    rep_iter_IR[0] = '0;
    rep_config_IR[0] = 1'b0;  // L0, S3,  D0, iter2

    // IR1
    @(posedge clk);
    @(posedge clk);
    @(posedge clk);
    rep_valid = 1'b1;
    rep_step_IR[1] = 0;
    rep_delay_IR[1] = 0;
    rep_iter_IR[1] = 7;
    rep_config_IR[1] = 1'b1;  // L1, S1,  D0, iter3

    @(posedge clk);
    rep_valid = 1'b0;
    rep_step_IR[0] = 0;
    rep_delay_IR[0] = 0;
    rep_iter_IR[0] = '0;
    rep_config_IR[0] = 1'b0;  // L0, S3,  D0, iter2

    @(posedge clk);
    @(posedge clk);
    activation = 1'b1;
    @(posedge clk);
    activation = 1'b0;

    #5600;  //WO delay
    $stop;
  end

endmodule
