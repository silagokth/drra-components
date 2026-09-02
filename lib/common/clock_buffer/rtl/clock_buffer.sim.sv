module clock_buffer (
  parameter SLOTS=1
) (
  input logic clk,
  output logic clk_out
) ;

  assign clk_out = clk;

endmodule