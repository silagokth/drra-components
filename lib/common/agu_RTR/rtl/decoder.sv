module decoder #(
    parameter WIDTH
) (
    input  logic                enable,
    input  logic [WIDTH-1:0]    in_data,
    output logic [2**WIDTH-1:0] out_data
);

    always_comb begin
        out_data = '0;
        if (enable)
            out_data[in_data[WIDTH-1:0]] = 1'b1;
    end

endmodule