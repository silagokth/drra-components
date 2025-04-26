module multiplier #(
    parameter BITWIDTH = 8
) (
    input logic signed [BITWIDTH-1:0] in1,
    input logic signed [BITWIDTH-1:0] in2,
    output logic signed [BITWIDTH-1:0] out,
    output logic overflow,
    output logic underflow,
    input logic saturate
);

  localparam logic signed [BITWIDTH-1:0] MAX_RESULT = (1 << (BITWIDTH - 1)) - 1;
  localparam logic signed [BITWIDTH-1:0] MIN_RESULT = -MAX_RESULT - 1;
  logic signed [BITWIDTH*2-1:0] temp_mult;

  always_comb begin
    out = 0;
    overflow = 0;
    underflow = 0;
    temp_mult = in1 * in2;
    // saturation logic
    if (saturate) begin
      if (temp_mult > MAX_RESULT) begin
        out = MAX_RESULT;
        overflow = 1;
      end else if (temp_mult < MIN_RESULT) begin
        out = MIN_RESULT;
        underflow = 1;
      end else begin
        out = temp_mult[BITWIDTH-1:0];
      end
    end else begin
      out = temp_mult[BITWIDTH-1:0];
    end
  end
endmodule

