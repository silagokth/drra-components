module agu_RTR
  import agu_RTR_pkg::*;
#(
    parameter int ADDRESS_WIDTH = 16,
    parameter int NUMBER_IR     = 4,
    parameter int NUMBER_MT     = 0,
    parameter int NUMBER_OR     = 0
) (
    input logic clk,
    input logic rst_n,
    input logic enable,

    // Configurations
    // Use ternary operators to ensure port width is at least 1 bit to avoid compilation errors
    // when NUMBER_OR or NUMBER_MT is 0. These ports are left unconnected in those cases.
    input rep_config_t [(NUMBER_OR > 0 ? NUMBER_OR : 1)-1:0] or_configs,
    input trans_config_t [(NUMBER_MT > 0 ? NUMBER_MT : 1)-1:0] mt_configs,
    input rep_config_t [NUMBER_IR-1:0] ir_configs[NUMBER_MT+1],

    // Outputs
    output logic [ADDRESS_WIDTH-1:0] ir_addr,
    output logic                     ir_valid,
    output logic                     ir_done
);

  // Check if NUMBER_IR is non-zero and NUMBER_MT is valid when NUMBER_OR > 0
  initial begin
    if (NUMBER_IR == 0) $error("agu_RTR: NUMBER_IR cannot be 0");
    if (NUMBER_OR != 0 && NUMBER_MT == 0)
      $error("agu_RTR: NUMBER_MT cannot be 0 if NUMBER_OR is not 0");
  end

  generate
    if (NUMBER_OR > 0) begin : gen_or_top
      or_mt_ir #(
          .ADDRESS_WIDTH(ADDRESS_WIDTH),
          .NUMBER_IR    (NUMBER_IR),
          .NUMBER_MT    (NUMBER_MT),
          .NUMBER_OR    (NUMBER_OR)
      ) u_or_mt_ir (
          .clk       (clk),
          .rst_n     (rst_n),
          .enable    (enable),
          .or_configs(or_configs),
          .mt_configs(mt_configs),
          .ir_configs(ir_configs),
          .ir_addr   (ir_addr),
          .ir_valid  (ir_valid),
          .ir_done   (ir_done)
      );
    end else if (NUMBER_MT > 0) begin : gen_mt_top
      mt_ir #(
          .ADDRESS_WIDTH(ADDRESS_WIDTH),
          .NUMBER_IR    (NUMBER_IR),
          .NUMBER_MT    (NUMBER_MT)
      ) u_mt_ir (
          .clk       (clk),
          .rst_n     (rst_n),
          .enable    (enable),
          .mt_configs(mt_configs),
          .ir_configs(ir_configs),
          .ir_addr   (ir_addr),
          .ir_valid  (ir_valid),
          .ir_done   (ir_done)
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
          .enable    (enable),
          .ir_configs(ir_configs[0]),
          .ir_addr   (ir_addr),
          .ir_valid  (ir_valid),
          .ir_done   (ir_done)
      );
    end
  endgenerate

endmodule
