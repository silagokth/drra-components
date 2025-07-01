module adder #(
    parameter WIDTH = 5
)(
    input  logic [WIDTH-1:0] a_in ,
    input  logic [WIDTH-1:0] b_in ,
    output logic [WIDTH:0] sum_out
);
    assign sum_out = a_in + b_in;     
endmodule

module adder_array #(
    parameter WIDTH = 5
)(
    input  logic [WIDTH-1:0] addend [0:3],
    input  logic [WIDTH-1:0] augend [0:3],
    output logic [WIDTH:0]   sums   [0:3]
);

    generate
        for (genvar i = 0; i < 4; i++) begin : adder_array_loop
            adder #(.WIDTH(WIDTH)) adder_inst (
                .a_in(addend[i]),
                .b_in(augend[i]),
                .sum_out(sums[i])
            );
        end
    endgenerate

endmodule
