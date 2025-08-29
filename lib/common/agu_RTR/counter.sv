module down_counter #(
    parameter WIDTH
) (
    input  logic clk,
    input  logic rst_n,
    input  logic enable,
    input  logic init,
    input  logic [WIDTH-1:0] init_value,
    output logic co
    );

    logic [WIDTH-1:0] count;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            count <= 0;
        end else if (init) begin
            count <= init_value;
        end else if (enable) begin
            count <= count - 1;
        end
    end

    assign co = ~(|count);
endmodule

module up_counter #(
    parameter WIDTH
) (
    input  logic clk,
    input  logic rst_n,
    input  logic enable,
    input  logic init0,
    output logic [WIDTH-1:0] count
    );
  
    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            count <= 0;
        end else if (init0) begin
            count <= 0;
        end else if (enable) begin
            count <= count + 1;
        end
    end
endmodule

module step_counter #(
    parameter COUNT_WIDTH,
    parameter STEP_WIDTH
) (
    input  logic clk,
    input  logic rst_n,
    input  logic enable,
    input  logic init0,
    input  logic init,
    input  logic [COUNT_WIDTH-1:0] init_value,
    input  logic [STEP_WIDTH-1:0] step,
    output logic [COUNT_WIDTH-1:0] count
    );
  
    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            count <= 0;
        end else if (init0) begin
            count <= 0;
        end else if (init) begin
            count <= init_value;
        end else if (enable) begin
            count <= count + step;
        end
    end
endmodule