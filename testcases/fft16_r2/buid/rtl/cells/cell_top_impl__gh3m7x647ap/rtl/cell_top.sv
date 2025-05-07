`define cell_top_impl _gh3m7x647ap
`define cell_top_impl_pkg _gh3m7x647ap_pkg

`define iosram_top_impl _xbvdm4fjuts
`define iosram_top_impl_pkg _xbvdm4fjuts_pkg
`define sequencer_impl _bofiw7zs7vj
`define sequencer_impl_pkg _bofiw7zs7vj_pkg
`define swb_impl _dixk93xtnmt
`define swb_impl_pkg _dixk93xtnmt_pkg

package _gh3m7x647ap_pkg;
    parameter BULK_BITWIDTH = 256;
    parameter FSM_PER_SLOT = 4;
    parameter INSTR_ADDR_WIDTH = 6;
    parameter INSTR_DATA_WIDTH = 32;
    parameter INSTR_HOPS_WIDTH = 4;
    parameter IO_ADDR_WIDTH = 16;
    parameter IO_DATA_WIDTH = 256;
    parameter NUM_SLOTS = 16;
    parameter RESOURCE_INSTR_WIDTH = 27;
    parameter WORD_BITWIDTH = 32;
endpackage

module _gh3m7x647ap
import _gh3m7x647ap_pkg::*;
(
    input logic clk,
    input logic rst_n,
    input logic call_in,
    output logic call_out,
    input logic ret_in,
    output logic ret_out,
    output logic io_en_in,
    output logic [IO_ADDR_WIDTH-1:0] io_addr_in,
    input logic [IO_DATA_WIDTH-1:0] io_data_in,
    output logic io_en_out,
    output logic [IO_ADDR_WIDTH-1:0] io_addr_out,
    output logic [IO_DATA_WIDTH-1:0] io_data_out,
    input logic [INSTR_DATA_WIDTH-1:0] instr_data_in,
    input logic [INSTR_ADDR_WIDTH-1:0] instr_addr_in,
    input logic [INSTR_HOPS_WIDTH-1:0] instr_hops_in,
    input logic instr_en_in,
    output logic [INSTR_DATA_WIDTH-1:0] instr_data_out,
    output logic [INSTR_ADDR_WIDTH-1:0] instr_addr_out,
    output logic [INSTR_HOPS_WIDTH-1:0] instr_hops_out,
    output logic instr_en_out,
    input logic [BULK_BITWIDTH-1:0] bulk_intercell_n_in,
    input logic [BULK_BITWIDTH-1:0] bulk_intercell_w_in,
    input logic [BULK_BITWIDTH-1:0] bulk_intercell_e_in,
    input logic [BULK_BITWIDTH-1:0] bulk_intercell_s_in,
    output logic [BULK_BITWIDTH-1:0] bulk_intercell_n_out,
    output logic [BULK_BITWIDTH-1:0] bulk_intercell_w_out,
    output logic [BULK_BITWIDTH-1:0] bulk_intercell_e_out,
    output logic [BULK_BITWIDTH-1:0] bulk_intercell_s_out
);

  logic [NUM_SLOTS-1:0] resource_clk;
  logic [NUM_SLOTS-1:0] resource_rst_n;
  logic [NUM_SLOTS-1:0] instr_valid;
  logic [NUM_SLOTS-1:0][RESOURCE_INSTR_WIDTH-1:0] instr;
  logic [NUM_SLOTS-1:0][FSM_PER_SLOT-1:0] activate;

  logic [NUM_SLOTS-1:0][WORD_BITWIDTH-1:0] word_data_in;
  logic [NUM_SLOTS-1:0][WORD_BITWIDTH-1:0] word_data_out;
  logic [NUM_SLOTS-1:0][BULK_BITWIDTH-1:0] bulk_data_in;
  logic [NUM_SLOTS-1:0][BULK_BITWIDTH-1:0] bulk_data_out;

  logic controller_call;
  logic controller_ret, controller_ret_remember, ret_in_remember;
  assign controller_call = call_in;
  always_ff @(negedge rst_n or posedge clk) begin
    if (~rst_n) begin
      call_out <= 0;
      controller_ret_remember <= 0;
      ret_in_remember <= 0;
    end else begin
      if (controller_ret) begin
        controller_ret_remember <= 1;
      end else if (ret_in) begin
        ret_in_remember <= 1;
      end
      call_out <= call_in;
    end
  end

  assign ret_out = controller_ret_remember & ret_in_remember;

    // Controller
    `sequencer_impl controller_inst
    (
        .clk(clk),
        .rst_n(rst_n),
        .call(controller_call),
        .ret(controller_ret),
        .instr_valid_0(instr_valid[0]),
        .instr_0(instr[0]),
        .activate_0(activate[0]),
        .clk_0(resource_clk[0]),
        .rst_n_0(resource_rst_n[0]),
        .instr_valid_1(instr_valid[1]),
        .instr_1(instr[1]),
        .activate_1(activate[1]),
        .clk_1(resource_clk[1]),
        .rst_n_1(resource_rst_n[1]),
        .instr_valid_2(instr_valid[2]),
        .instr_2(instr[2]),
        .activate_2(activate[2]),
        .clk_2(resource_clk[2]),
        .rst_n_2(resource_rst_n[2]),
        .instr_valid_3(instr_valid[3]),
        .instr_3(instr[3]),
        .activate_3(activate[3]),
        .clk_3(resource_clk[3]),
        .rst_n_3(resource_rst_n[3]),
        .instr_valid_4(instr_valid[4]),
        .instr_4(instr[4]),
        .activate_4(activate[4]),
        .clk_4(resource_clk[4]),
        .rst_n_4(resource_rst_n[4]),
        .instr_valid_5(instr_valid[5]),
        .instr_5(instr[5]),
        .activate_5(activate[5]),
        .clk_5(resource_clk[5]),
        .rst_n_5(resource_rst_n[5]),
        .instr_valid_6(instr_valid[6]),
        .instr_6(instr[6]),
        .activate_6(activate[6]),
        .clk_6(resource_clk[6]),
        .rst_n_6(resource_rst_n[6]),
        .instr_valid_7(instr_valid[7]),
        .instr_7(instr[7]),
        .activate_7(activate[7]),
        .clk_7(resource_clk[7]),
        .rst_n_7(resource_rst_n[7]),
        .instr_valid_8(instr_valid[8]),
        .instr_8(instr[8]),
        .activate_8(activate[8]),
        .clk_8(resource_clk[8]),
        .rst_n_8(resource_rst_n[8]),
        .instr_valid_9(instr_valid[9]),
        .instr_9(instr[9]),
        .activate_9(activate[9]),
        .clk_9(resource_clk[9]),
        .rst_n_9(resource_rst_n[9]),
        .instr_valid_10(instr_valid[10]),
        .instr_10(instr[10]),
        .activate_10(activate[10]),
        .clk_10(resource_clk[10]),
        .rst_n_10(resource_rst_n[10]),
        .instr_valid_11(instr_valid[11]),
        .instr_11(instr[11]),
        .activate_11(activate[11]),
        .clk_11(resource_clk[11]),
        .rst_n_11(resource_rst_n[11]),
        .instr_valid_12(instr_valid[12]),
        .instr_12(instr[12]),
        .activate_12(activate[12]),
        .clk_12(resource_clk[12]),
        .rst_n_12(resource_rst_n[12]),
        .instr_valid_13(instr_valid[13]),
        .instr_13(instr[13]),
        .activate_13(activate[13]),
        .clk_13(resource_clk[13]),
        .rst_n_13(resource_rst_n[13]),
        .instr_valid_14(instr_valid[14]),
        .instr_14(instr[14]),
        .activate_14(activate[14]),
        .clk_14(resource_clk[14]),
        .rst_n_14(resource_rst_n[14]),
        .instr_valid_15(instr_valid[15]),
        .instr_15(instr[15]),
        .activate_15(activate[15]),
        .clk_15(resource_clk[15]),
        .rst_n_15(resource_rst_n[15]),
        .instr_load_data_in(instr_data_in),
        .instr_load_addr_in(instr_addr_in),
        .instr_load_hops_in(instr_hops_in),
        .instr_load_en_in(instr_en_in),
        .instr_load_data_out(instr_data_out),
        .instr_load_addr_out(instr_addr_out),
        .instr_load_hops_out(instr_hops_out),
        .instr_load_en_out(instr_en_out)
    );

    `swb_impl resource_0_inst
    (
        .clk_0(resource_clk[0]),
        .rst_n_0(resource_rst_n[0]),
        .instr_en_0(instr_valid[0]),
        .instr_0(instr[0]),
        .activate_0(activate[0]),
        .word_channels_in(word_data_out),
        .word_channels_out(word_data_in),
        .bulk_intracell_in(bulk_data_out),
        .bulk_intracell_out(bulk_data_in),
        .bulk_intercell_n_in(bulk_intercell_n_in),
        .bulk_intercell_w_in(bulk_intercell_w_in),
        .bulk_intercell_e_in(bulk_intercell_e_in),
        .bulk_intercell_s_in(bulk_intercell_s_in),
        .bulk_intercell_n_out(bulk_intercell_n_out),
        .bulk_intercell_w_out(bulk_intercell_w_out),
        .bulk_intercell_e_out(bulk_intercell_e_out),
        .bulk_intercell_s_out(bulk_intercell_s_out)
      );


    `iosram_top_impl resource_1_inst
    (
        .clk_0(clk),
        .rst_n_0(rst_n),
        .instr_en_0(instr_valid[1]),
        .instr_0(instr),
        .activate_0(activate[1]),
        .word_data_in_0(word_data_in[1]),
        .word_data_out_0(word_data_out[1]),
        .bulk_data_in_0(bulk_data_in[1]),
        .bulk_data_out_0(bulk_data_out[1]),        .clk_1(clk),
        .rst_n_1(rst_n),
        .instr_en_1(instr_valid[2]),
        .instr_1(instr),
        .activate_1(activate[2]),
        .word_data_in_1(word_data_in[2]),
        .word_data_out_1(word_data_out[2]),
        .bulk_data_in_1(bulk_data_in[2]),
        .bulk_data_out_1(bulk_data_out[2]),        .clk_2(clk),
        .rst_n_2(rst_n),
        .instr_en_2(instr_valid[3]),
        .instr_2(instr),
        .activate_2(activate[3]),
        .word_data_in_2(word_data_in[3]),
        .word_data_out_2(word_data_out[3]),
        .bulk_data_in_2(bulk_data_in[3]),
        .bulk_data_out_2(bulk_data_out[3]),        .clk_3(clk),
        .rst_n_3(rst_n),
        .instr_en_3(instr_valid[4]),
        .instr_3(instr),
        .activate_3(activate[4]),
        .word_data_in_3(word_data_in[4]),
        .word_data_out_3(word_data_out[4]),
        .bulk_data_in_3(bulk_data_in[4]),
        .bulk_data_out_3(bulk_data_out[4]),
        .io_en_in(io_en_in),
        .io_addr_in(io_addr_in),
        .io_data_in(io_data_in)
    );

endmodule
