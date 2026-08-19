// config_controller — a resource's configuration register file.
//
// NUM_REGISTERS registers of REG_WIDTH bits. A CONF instruction writes one of
// them, addressed by its own register-index field, and every register is
// exposed on `registers`.
//
// Deliberately dumb: it stores bits and nothing else. Which register is live,
// and what any of them mean, is the resource's logic to decide — so this module
// needs nothing from the ISA and is compiled once and shared.
module config_controller #(
    parameter int NUM_REGISTERS = 4,
    parameter int REG_WIDTH     = 16,

    // $clog2(1) is 0, which would make a zero-width port; clamp to 1 so a
    // single-register file still elaborates.
    localparam int ADDR_WIDTH = (NUM_REGISTERS > 1) ? $clog2(NUM_REGISTERS) : 1
) (
    input logic clk,
    input logic rst_n,

    // CONF write port — one whole register per instruction.
    input logic                  write_valid,
    input logic [ADDR_WIDTH-1:0] write_addr,
    input logic [REG_WIDTH-1:0]  write_data,

    output logic [NUM_REGISTERS-1:0][REG_WIDTH-1:0] registers
);

  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      registers <= '0;
    end else if (write_valid) begin
      registers[write_addr] <= write_data;
    end
  end

endmodule
