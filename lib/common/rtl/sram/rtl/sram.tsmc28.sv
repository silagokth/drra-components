module sram #(
    parameter WIDTH = 8,
    parameter DEPTH = 256
) (
    input logic clk,
    input logic enable_a_n,
    input logic enable_b_n,
    input logic write_enable_a_n,
    input logic write_enable_b_n,
    input logic [WIDTH-1:0] data_a,
    input logic [WIDTH-1:0] data_b,
    input logic [$clog2(DEPTH)-1:0] address_a,
    input logic [$clog2(DEPTH)-1:0] address_b,
    output logic [WIDTH-1:0] q_a,
    output logic [WIDTH-1:0] q_b
);

if (WIDTH == 256 && DEPTH == 64) begin : sram_256x64
    TSDN28HPCPUHDB64X128M4MWA sram_macro_0 (
      .RTSEL(2'b00),
      .WTSEL(2'b00),
      .PTSEL(2'b00),
      .AA(address_a),
      .DA(data_a[127:0]),
      .BWEBA({128{1'b0}}),
      .WEBA(write_enable_a_n),
      .CEBA(enable_a_n),
      .QA(q_a[127:0]),
      .AB(address_b),
      .DB(data_b[127:0]),
      .BWEBB({128{1'b0}}),
      .WEBB(write_enable_b_n),
      .CEBB(enable_b_n),
      .QB(q_b[127:0]),
      .AWT(0),
      .CLK(clk)
      );
    TSDN28HPCPUHDB64X128M4MWA sram_macro_1 (
      .RTSEL(2'b00),
      .WTSEL(2'b00),
      .PTSEL(2'b00),
      .AA(address_a),
      .DA(data_a[255:128]),
      .BWEBA({128{1'b0}}),
      .WEBA(write_enable_a_n),
      .CEBA(enable_a_n),
      .QA(q_a[255:128]),
      .AB(address_b),
      .DB(data_b[255:128]),
      .BWEBB({128{1'b0}}),
      .WEBB(write_enable_b_n),
      .CEBB(enable_b_n),
      .QB(q_b[255:128]),
      .AWT(0),
      .CLK(clk)
      );
end else if (WIDTH == 256 && DEPTH == 128) begin : sram_256x128
    TSDN28HPCPUHDB128X128M4MWA sram_macro_0 (
      .RTSEL(2'b00),
      .WTSEL(2'b00),
      .PTSEL(2'b00),
      .AA(address_a),
      .DA(data_a[127:0]),
      .BWEBA({128{1'b0}}),
      .WEBA(write_enable_a_n),
      .CEBA(enable_a_n),
      .QA(q_a[127:0]),
      .AB(address_b),
      .DB(data_b[127:0]),
      .BWEBB({128{1'b0}}),
      .WEBB(write_enable_b_n),
      .CEBB(enable_b_n),
      .QB(q_b[127:0]),
      .AWT(0),
      .CLK(clk)
      );
    TS28HPCUHDB128X128M4MWB sram_macro_1 (
      .RTSEL(2'b00),
      .WTSEL(2'b00),
      .PTSEL(2'b00),
      .AA(address_a),
      .DA(data_a[255:128]),
      .BWEBA({128{1'b0}}),
      .WEBA(write_enable_a_n),
      .CEBA(enable_a_n),
      .QA(q_a[255:128]),
      .AB(address_b),
      .DB(data_b[255:128]),
      .BWEBB({128{1'b0}}),
      .WEBB(write_enable_b_n),
      .CEBB(enable_b_n),
      .QB(q_b[255:128]),
      .AWT(0),
      .CLK(clk)
      );
end else if (WIDTH == 256 && DEPTH == 256) begin : sram_256x256
    TSDN28HPCPUHDB256X128M4MWA sram_macro_0 (
      .RTSEL(2'b00),
      .WTSEL(2'b00),
      .PTSEL(2'b00),
      .AA(address_a),
      .DA(data_a[127:0]),
      .BWEBA({128{1'b0}}),
      .WEBA(write_enable_a_n),
      .CEBA(enable_a_n),
      .QA(q_a[127:0]),
      .AB(address_b),
      .DB(data_b[127:0]),
      .BWEBB({128{1'b0}}),
      .WEBB(write_enable_b_n),
      .CEBB(enable_b_n),
      .QB(q_b[127:0]),
      .AWT(0),
      .CLK(clk)
      );
    TS28HPCUHDB256X128M4MWB sram_macro_1 (
      .RTSEL(2'b00),
      .WTSEL(2'b00),
      .PTSEL(2'b00),
      .AA(address_a),
      .DA(data_a[255:128]),
      .BWEBA({128{1'b0}}),
      .WEBA(write_enable_a_n),
      .CEBA(enable_a_n),
      .QA(q_a[255:128]),
      .AB(address_b),
      .DB(data_b[255:128]),
      .BWEBB({128{1'b0}}),
      .WEBB(write_enable_b_n),
      .CEBB(enable_b_n),
      .QB(q_b[255:128]),
      .AWT(0),
      .CLK(clk)
      );
end else begin
    // Unsupported configuration
    $fatal("Unsupported configuration: WIDTH=%0d, DEPTH=%0d", WIDTH, DEPTH);
end

endmodule
