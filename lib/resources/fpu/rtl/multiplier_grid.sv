module multiplier #(
    parameter WIDTH = 3
)(
    input  logic  [WIDTH-1:0]   a_in,
    input  logic  [WIDTH-1:0]   b_in,
    output logic  [2*WIDTH-1:0] product_out
);
    assign product_out = a_in * b_in;
endmodule

module multiplier_grid #(
    parameter WIDTH = 2, 
    parameter GRID_SIZE = 24 / WIDTH
)(
    input  logic [23:0]        multiplicand,
    input  logic [23:0]        multiplier,
    output logic [2*WIDTH-1:0] partial_products [0:GRID_SIZE-1][0:GRID_SIZE-1]
);
    // Multiplier array
    generate
        genvar i, j;
        for (i = 0; i < GRID_SIZE; i++) begin : loop_mul_i
            for (j = 0; j < GRID_SIZE; j++) begin : loop_mul_j
                multiplier #(.WIDTH(WIDTH)) muls (  
                    .a_in(multiplicand[WIDTH*i + WIDTH-1 -: WIDTH]),
                    .b_in(multiplier[WIDTH*j + WIDTH-1 -: WIDTH]),
                    .product_out(partial_products[i][j])
                );
            end
        end
    endgenerate

endmodule
