module comparator_form #(
    parameter WIDTH = 5
) (
    input  logic [WIDTH-1:0] a,
    input  logic [WIDTH-1:0] b,
    output logic [WIDTH-1:0] gout,
    output logic [WIDTH-1:0] lout,
    output logic a_greater,
    output logic b_greater
);
    always_comb begin
        if (a > b) begin
            a_greater = 1'b1;
            b_greater = 1'b0;
            gout       = a;
            lout       = b;
        end else if (b > a) begin
            a_greater = 1'b0;
            b_greater = 1'b1;
            gout       = b;
            lout       = a;
        end else begin
            a_greater = 1'b0;
            b_greater = 1'b0;
            gout       = a;
            lout       = b;    
        end
    end
endmodule

module formatter (
    input  logic [ 1:0] src_format,     
    input  logic [31:0] op_a,
    input  logic [31:0] op_b,
    output logic        align_flag_fp32 ,
    output logic        align_flag [0:3],
    output logic [6:0]  exp_diff   [0:3]
);

logic [8:0] twoscomp_lout_fp32;
logic [4:0] to_comp_a [0:3], to_comp_b [0:3], gout [0:3], lout [0:3];
logic [5:0] twoscomp_lout [0:3];
logic [5:0] minuend[0:3], subtrahend[0:3];
logic [6:0] diff[0:3];
logic [7:0] gout_fp32, lout_fp32;
logic a_greater [0:3], b_greater [0:3];
logic is_equal;

// Comperators for the exponents, later they'll be substracted
always_comb begin
    case(src_format) 
        2'b00 : begin
            to_comp_a[0] = {2'b0, op_a[25:23]};
            to_comp_b[0] = {2'b0, op_b[25:23]};
            to_comp_a[1] = op_a[30:26];
            to_comp_b[1] = op_b[30:26];
            for (int i = 2; i < 4; i++) begin           // we don't care
                to_comp_a[i] = op_a[i*8 + 2 +: 5];
                to_comp_b[i] = op_b[i*8 + 2 +: 5];
            end
        end
        2'b01 : begin
            for (int i = 0; i < 2; i++) begin
                to_comp_a[i] = op_a[i*16 + 10 +: 5];
                to_comp_b[i] = op_b[i*16 + 10 +: 5];
            end
            for (int i = 2; i < 4; i++) begin           // we don't care
                to_comp_a[i] = op_a[i*8 + 2 +: 5];
                to_comp_b[i] = op_b[i*8 + 2 +: 5];
            end
        end
        2'b10 : begin
            for (int i = 0; i < 4; i++) begin
                to_comp_a[i] = op_a[i*8 + 2 +: 5];
                to_comp_b[i] = op_b[i*8 + 2 +: 5];
            end
        end
        default : begin
            for (int i = 0; i < 4; i++) begin
                to_comp_a[i] = '0;
                to_comp_b[i] = '0;
            end
        end
    endcase
end

generate
    for (genvar i = 0; i < 4; i++) begin : comp_loop
        comparator_form #(.WIDTH(5)) comp_inst (
            .a(to_comp_a[i]),
            .b(to_comp_b[i]),
            .a_greater(a_greater[i]),
            .b_greater(b_greater[i]),
            .gout(gout[i]),
            .lout(lout[i])
        );
    end
endgenerate

always_comb begin
    is_equal = ~(a_greater[1] | b_greater[1]);      // for FP32, compare the 5 MSBs first
    if (is_equal) begin
        if (b_greater[0]) begin
            gout_fp32 = op_b[30:23];
            lout_fp32 = op_a[30:23];
            align_flag_fp32 = 1'b0;
        end else begin
            gout_fp32 = op_a[30:23];
            lout_fp32 = op_b[30:23];
            align_flag_fp32 = 1'b1;
        end
    end else if (a_greater[1]) begin
        gout_fp32 = op_a[30:23];
        lout_fp32 = op_b[30:23];
        align_flag_fp32 = 1'b1;
    end else begin
        gout_fp32 = op_b[30:23];
        lout_fp32 = op_a[30:23]; 
        align_flag_fp32 = 1'b0;
    end
end

always_comb begin
    twoscomp_lout_fp32 = {1'b0, ~lout_fp32} + 1;
    for (int i = 0; i < 4; i++) begin
        twoscomp_lout[i] = {1'b0, ~lout[i]} + 1;
    end
    if (src_format == 2'b00) begin
        minuend[0]    = gout_fp32[5:0];        
        subtrahend[0] = twoscomp_lout_fp32[5:0];
        minuend[1]    = {4'b0, gout_fp32[7:6]};
        subtrahend[1] = {3'b0, twoscomp_lout_fp32[8:6]};

        for (int i = 2; i < 4; i++) begin       // we don't care
            minuend[i]    = {1'b0, gout[i]};
            subtrahend[i] = twoscomp_lout[i];
        end
    end else begin  
        for (int i = 0; i < 4; i++) begin
            minuend[i]    = {1'b0, gout[i]};
            subtrahend[i] = twoscomp_lout[i];
        end
    end
end

adder_array #(.WIDTH(6)) adder_exp (
    .addend(minuend),
    .augend(subtrahend),
    .sums(diff)
);

always_comb begin
    for (int i = 0; i < 4; i++) begin
        align_flag[i] = a_greater[i];
    end
end

assign exp_diff = diff;

endmodule 


