// sram_1rw for a technology without macros: always flip-flops.
module sram_1rw #(
    parameter int WIDTH = 32,
    parameter int DEPTH = 128
) (
    input  logic                     clk,
    input  logic                     en,     // access this cycle
    input  logic                     we,     // write when en, otherwise read
    input  logic [$clog2(DEPTH)-1:0] addr,
    input  logic [        WIDTH-1:0] wdata,
    output logic [        WIDTH-1:0] rdata
);

  sram_1rw_ff #(
      .WIDTH(WIDTH),
      .DEPTH(DEPTH)
  ) ff_inst (
      .clk  (clk),
      .en   (en),
      .we   (we),
      .addr (addr),
      .wdata(wdata),
      .rdata(rdata)
  );

endmodule
