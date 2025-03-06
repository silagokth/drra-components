// Simulation model for the dual port SRAM
module sram #(
    parameter WIDTH = 256,
    parameter DEPTH = 64
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

  logic [DEPTH-1:0][WIDTH-1:0] memory;

  always_ff @(posedge clk) begin
    if (!enable_a_n && !write_enable_a_n) begin
      memory[address_a] <= data_a;
    end
  end

  always_comb begin
    q_a = {WIDTH{1'b0}};
    q_b = {WIDTH{1'b0}};
    if (!enable_b_n) begin
      q_b = memory[address_b];
    end
  end

endmodule
