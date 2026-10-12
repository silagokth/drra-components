// Flip-flop implementation of sram_1rw, used wherever no macro fits.
//
// It keeps the timing of a synchronous single-port macro, so a design behaves
// the same cycle for cycle on either: a read issued in one cycle returns its
// word in the next, and rdata is only defined in the cycle after a read. The
// contents are not reset.
module sram_1rw_ff #(
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

  logic [WIDTH-1:0] mem[DEPTH];

  always_ff @(posedge clk) begin
    if (en) begin
      if (we) begin
        mem[addr] <= wdata;
      end else begin
        rdata <= mem[addr];
      end
    end
  end

endmodule
