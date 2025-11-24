module lzd_comb (
    input  logic [23:0] norm_mant_fp32        ,
    input  logic [10:0] norm_mant_fp16   [0:1],
    input  logic [2:0]  norm_mant_fp8    [0:3],
    input  logic [7:0]  norm_exp_fp32         ,
    input  logic [4:0]  norm_exp_fp16    [0:1],
    input  logic [4:0]  norm_exp_fp8     [0:3],
    output logic [7:0]  shift_count_fp32      , 
    output logic [4:0]  shift_count_fp16 [0:1], 
    output logic [4:0]  shift_count_fp8  [0:3]
);

// Function to count leading zeros in a vector of WIDTH (MANTISSA_W+1).
function automatic int count_leading_zeros(input logic [23:0] data, input int width);
    int unsigned count = 0;
    
    // Iterate from MSB based on the provided width
    for (int i = width - 1; i >= 0; i--) begin
        if (data[i] == 1'b0)
            count++;
        else
            break;
    end
    return count;
endfunction

logic [4:0] lz_count_fp32;
logic [3:0] lz_count_fp16 [0:1];
logic [1:0] lz_count_fp8  [0:3];

always_comb begin
    // Compute the number of leading zeros from normalized_mant.
    lz_count_fp32 = count_leading_zeros(norm_mant_fp32, 24);
    for (int i = 0; i < 2; i++) begin
        lz_count_fp16[i] = count_leading_zeros(norm_mant_fp16[i], 11);
    end
    for (int i = 0; i < 4; i++) begin
        lz_count_fp8[i] = count_leading_zeros(norm_mant_fp8[i], 3);
    end
end

// Determine the effective shift: the minimum between lz_count and the available exponent.
always_comb begin
    logic msb_gt_fp32, gt_fp32;
    logic msb_gt_fp16 [0:1], gt_fp16 [0:1];
    logic msb_gt_fp8  [0:3], gt_fp8  [0:3];

    msb_gt_fp32 = norm_exp_fp32[7] | norm_exp_fp32[6] | norm_exp_fp32[5];
    gt_fp32     = (norm_exp_fp32[4:0] > lz_count_fp32);
    
    shift_count_fp32 = (msb_gt_fp32 | gt_fp32) ? lz_count_fp32 : norm_exp_fp32;

    for (int i = 0; i < 2; i++) begin
        msb_gt_fp16[i] = norm_exp_fp16[i][4];
        gt_fp16[i]     = (norm_exp_fp16[i][3:0] > lz_count_fp16[i]);
        shift_count_fp16[i] = (msb_gt_fp16[i] | gt_fp16[i]) ? lz_count_fp16[i] : norm_exp_fp16[i];
    end

    for (int i = 0; i < 4; i++) begin
        msb_gt_fp8[i] = norm_exp_fp8[i][4] | norm_exp_fp8[i][3] | norm_exp_fp8[i][2];
        gt_fp8[i] = (norm_exp_fp8[i][1:0] > lz_count_fp8[i]);
        shift_count_fp8[i] = (msb_gt_fp8[i] | gt_fp8[i]) ? lz_count_fp8[i] : norm_exp_fp8[i][1:0];
    end
end

endmodule
