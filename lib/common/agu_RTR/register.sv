module register #(
    parameter WIDTH
) (
    input  logic clk,
    input  logic rst_n,
    input  logic enable,
    input  logic [WIDTH-1:0] in_value,
    output logic [WIDTH-1:0] out_value
    );
  
    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            out_value <= 0;
        end else if (enable) begin
            out_value <= in_value;
        end
    end
endmodule