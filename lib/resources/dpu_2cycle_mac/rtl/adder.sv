// vesyla_template_start module_head
module {{name}}_{{fingerprint}}_adder
import {{name}}_{{fingerprint}}_pkg::*;
// vesyla_template_end module_head
(
    input logic signed [BITWIDTH-1:0] in1,
    input logic signed [BITWIDTH-1:0] in2,
    output logic signed [BITWIDTH-1:0] out,
    output logic overflow,
    output logic underflow,
    input logic saturate
);

  localparam logic signed [BITWIDTH-1:0] MAX_RESULT = (1 << (BITWIDTH - 1)) - 1;
  localparam logic signed [BITWIDTH-1:0] MIN_RESULT = -MAX_RESULT - 1;
  logic signed [BITWIDTH:0] temp_add;

  always_comb begin
    overflow = 0;
    underflow = 0;
    out = 0;
    temp_add = in1 + in2;
    // saturation logic
    if (saturate) begin
      if (temp_add > MAX_RESULT) begin
        out = MAX_RESULT;
        overflow = 1;
      end else if (temp_add < MIN_RESULT) begin
        out = MIN_RESULT;
        underflow = 1;
      end else begin
        out = temp_add[BITWIDTH-1:0];
      end
    end else begin
      out = temp_add[BITWIDTH-1:0];
    end
  end
endmodule
