module agu_RTR
  import agu_RTR_pkg::*;
#(
    parameter int ADDRESS_WIDTH     = 16,
    parameter int NUMBER_IR         = 4,
    parameter int NUMBER_MT         = 3,
    parameter int NUMBER_OR         = 4,
    parameter int REP_DELAY_WIDTH   = 6,
    parameter int REP_ITER_WIDTH    = 6,
    parameter int REP_STEP_WIDTH    = 6,
    parameter int TRANS_DELAY_WIDTH = 12
) (
    input logic clk,
    input logic rst_n,
    input logic enable,
    input logic activation,

    input agu_config_class#(
        .NUMBER_IR        (NUMBER_IR),
        .NUMBER_MT        (NUMBER_MT),
        .NUMBER_OR        (NUMBER_OR),
        .REP_DELAY_WIDTH  (REP_DELAY_WIDTH),
        .REP_ITER_WIDTH   (REP_ITER_WIDTH),
        .REP_STEP_WIDTH   (REP_STEP_WIDTH),
        .TRANS_DELAY_WIDTH(TRANS_DELAY_WIDTH)
    )::agu_config_t agu_config,

    output logic [ADDRESS_WIDTH-1:0] addr,
    output logic                     addr_valid,
    output logic                     done
);
  initial begin
    if (NUMBER_IR == 0) $error("agu_RTR: NUMBER_IR cannot be 0");
    if (NUMBER_OR != 0 && NUMBER_MT == 0)
      $error("agu_RTR: NUMBER_MT cannot be 0 if NUMBER_OR is not 0");
  end

  logic activation_reg;
  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      activation_reg <= 0;
    end else if (done) begin
      activation_reg <= 0;
    end else if ((enable && activation) || activation_reg) begin
      activation_reg <= 1;
    end
  end

  generate
    if (NUMBER_OR > 0) begin : gen_or_top
      or_mt_ir #(
          .ADDRESS_WIDTH    (ADDRESS_WIDTH),
          .NUMBER_IR        (NUMBER_IR),
          .NUMBER_MT        (NUMBER_MT),
          .NUMBER_OR        (NUMBER_OR),
          .REP_DELAY_WIDTH  (REP_DELAY_WIDTH),
          .REP_ITER_WIDTH   (REP_ITER_WIDTH),
          .REP_STEP_WIDTH   (REP_STEP_WIDTH),
          .TRANS_DELAY_WIDTH(TRANS_DELAY_WIDTH)
      ) u_or_mt_ir (
          .clk       (clk),
          .rst_n     (rst_n),
          .enable    (activation_reg),
          .or_configs(agu_config.or_configs),
          .mt_configs(agu_config.mt_configs),
          .ir_configs(agu_config.ir_configs),
          .ir_addr   (addr),
          .ir_valid  (addr_valid),
          .ir_done   (done)
      );
    end else if (NUMBER_MT > 0) begin : gen_mt_top
      mt_ir #(
          .ADDRESS_WIDTH    (ADDRESS_WIDTH),
          .NUMBER_IR        (NUMBER_IR),
          .NUMBER_MT        (NUMBER_MT),
          .REP_DELAY_WIDTH  (REP_DELAY_WIDTH),
          .REP_ITER_WIDTH   (REP_ITER_WIDTH),
          .REP_STEP_WIDTH   (REP_STEP_WIDTH),
          .TRANS_DELAY_WIDTH(TRANS_DELAY_WIDTH)
      ) u_mt_ir (
          .clk       (clk),
          .rst_n     (rst_n),
          .enable    (activation_reg),
          .mt_configs(agu_config.mt_configs),
          .ir_configs(agu_config.ir_configs),
          .ir_addr   (addr),
          .ir_valid  (addr_valid),
          .ir_done   (done)
      );
    end else begin : gen_ir_top
      ir #(
          .ADDRESS_WIDTH(ADDRESS_WIDTH),
          .NUMBER_IR    (NUMBER_IR),
          .DELAY_WIDTH  (REP_DELAY_WIDTH),
          .ITER_WIDTH   (REP_ITER_WIDTH),
          .STEP_WIDTH   (REP_STEP_WIDTH)
      ) u_ir (
          .clk       (clk),
          .rst_n     (rst_n),
          .enable    (activation_reg),
          .ir_configs(agu_config.ir_configs),
          .ir_addr   (addr),
          .ir_valid  (addr_valid),
          .ir_done   (done)
      );
    end
  endgenerate

endmodule
