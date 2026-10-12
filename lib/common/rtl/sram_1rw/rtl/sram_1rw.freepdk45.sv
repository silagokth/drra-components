// sram_1rw for FreePDK45, on the OpenRAM macros in macros/freepdk45.
//
// For now only sram_128x32 exists: every 32-bit memory of up to 128 words
// uses it, with the unused upper address bits tied to 0. Anything else falls
// back to flip-flops.
//
// The macro registers its inputs on the rising edge and reads on the falling
// edge, so rdata is valid only in the second half of the cycle after the read,
// and it goes to X after every rising edge whether or not the macro is
// selected. A word must be read again in every cycle it is needed.
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

  if (WIDTH == 32 && DEPTH <= 128) begin : g_sram_128x32
    sram_128x32 macro_inst (
        .clk0 (clk),
        .csb0 (~en),
        .web0 (~we),
        .addr0(7'(addr)),
        .din0 (wdata),
        .dout0(rdata)
    );
`ifdef TARGET_SIMULATION
    // The OpenRAM model prints every access by default.
    defparam macro_inst.VERBOSE = 0;
`endif
  end else begin : g_ff
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
  end

endmodule
