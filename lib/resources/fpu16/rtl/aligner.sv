module aligner_comb (
    input  logic [ 1:0] src_format, 
    input  logic [31:0] op_a,
    input  logic [31:0] op_b,
    input  logic [6:0]  exp_diff   [0:3],
    input  logic        align_flag_fp32 ,
    input  logic        align_flag [0:3],
    output logic [23:0] aligned_mant_a_fp32,
    output logic [23:0] aligned_mant_b_fp32,
    output logic [7:0]  exp_fp32,
    output logic [10:0] aligned_mant_a_fp16 [0:1],
    output logic [10:0] aligned_mant_b_fp16 [0:1],
    output logic [4:0]  exp_fp16 [0:1],
    output logic [2:0]  aligned_mant_a_fp8  [0:3],
    output logic [2:0]  aligned_mant_b_fp8  [0:3],
    output logic [4:0]  exp_fp8  [0:3]  
);

logic [3:0] texp_diff_fp32;         // max value of addition = 11 + 111 = 1010
logic [9:0] exp_diff_fp32;

always_comb begin
    texp_diff_fp32 = exp_diff[1][3:0] + exp_diff[0][6];     // max value: 1010 + 1 = 1011
    exp_diff_fp32  = {texp_diff_fp32, exp_diff[0][5:0]};
end

always_comb begin
    if (align_flag_fp32) begin
        exp_fp32            = op_a[30:23];   
        aligned_mant_a_fp32 = {1'b1, op_a[22:0]};
        aligned_mant_b_fp32 = {1'b1, op_b[22:0]} >> exp_diff_fp32[7:0];
    end else begin
        exp_fp32            = op_b[30:23]; 
        aligned_mant_a_fp32 = {1'b1, op_a[22:0]} >> exp_diff_fp32[7:0];
        aligned_mant_b_fp32 = {1'b1, op_b[22:0]};
    end
    for (int i = 0; i < 2; i++) begin
        if (align_flag[i]) begin
            exp_fp16[i]            = op_a[16*i + 10 +: 5];
            aligned_mant_a_fp16[i] = {1'b1, op_a[16*i +: 10]};
            aligned_mant_b_fp16[i] = {1'b1, op_b[16*i +: 10]} >> exp_diff[i][4:0];
        end else begin
            exp_fp16[i]            = op_b[16*i + 10 +: 5];
            aligned_mant_a_fp16[i] = {1'b1, op_a[16*i +: 10]} >> exp_diff[i][4:0];
            aligned_mant_b_fp16[i] = {1'b1, op_b[16*i +: 10]};
        end
    end
    for (int i = 0; i < 4; i++) begin
        if (align_flag[i]) begin
            exp_fp8[i]            = op_a[8*i + 2 +: 5];
            aligned_mant_a_fp8[i] = {1'b1, op_a[8*i +: 2]};
            aligned_mant_b_fp8[i] = {1'b1, op_b[8*i +: 2]}    >> exp_diff[i][4:0];
        end else begin
            exp_fp8[i]            = op_b[8*i + 2 +: 5];
            aligned_mant_a_fp8[i] = {1'b1, op_a[8*i +: 2]}    >> exp_diff[i][4:0];
            aligned_mant_b_fp8[i] = {1'b1, op_b[8*i +: 2]};
        end
    end
end

endmodule
