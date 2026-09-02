module clock_buffer #(
  parameter SLOTS=1
) (
  input logic clk,
  output logic clk_out
) ;

logic [SLOTS:0] clk_buf;
logic [SLOTS:0] dummy;
assign clk_buf[0] = clk;

genvar i;
generate
  for (i = 0; i < SLOTS; i++) begin
    SC8T_CKBUFX16_CSC28L clock_dont_touch_buffer (.CLK(clk_buf[i]), .Z  (clk_buf[i+1]));
    if (i < SLOTS -1) begin
        SC8T_CKBUFX16_CSC28L clock_dont_touch_buffer_load (.CLK(clk_buf[i+1]), .Z(dummy[i]));
    end
  end
endgenerate

assign clk_out = clk_buf[SLOTS];


endmodule
