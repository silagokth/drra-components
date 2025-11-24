module tree_mantadder 
(
    input  logic [4:0]  sums      [0:6],
    output logic [24:0] mant_fp32      ,
    output logic [11:0] mant_fp16 [0:1],
    output logic [3:0]  mant_fp8  [0:3]
);

logic [4:0]  temp_sum [0:4];   // intermediate fixed sums
logic [29:0] res_fp32;
logic [12:0] res1_fp16, res2_fp16;
logic [16:0] mid_sum;
logic [17:0] upper_sum;

always_comb begin
    // First sum is direct, no carry-in
    temp_sum[0] = sums[1] + sums[0][4];
    temp_sum[1] = sums[2] + temp_sum[0][4]; 
    res1_fp16 = {temp_sum[1], temp_sum[0][3:0], sums[0][3:0]};      // sum_1_fp16 = sum[0] + sum[1] + sum[2] 
    
    temp_sum[2] = sums[4] + sums[3][4]; 
    temp_sum[3] = sums[5] + temp_sum[2][4];

    res2_fp16 = {temp_sum[3], temp_sum[2][3:0], sums[3][3:0]};      // sum_2_fp16 = sum[3] + sum[4] + sum[5] 

    temp_sum[4] = sums[6] + temp_sum[3][4];
    mid_sum = {temp_sum[4], temp_sum[3][3:0], temp_sum[2][3:0], sums[3][3:0]};
    upper_sum = mid_sum + res1_fp16[12];                            // add the carry out from the lower sum

    res_fp32 = {upper_sum, res1_fp16[11:0]};
end

always_comb begin
    mant_fp32    = res_fp32 [24:0];             // it was padded, so the actual result is 25 bits
    mant_fp16[0] = res1_fp16[11:0];
    mant_fp16[1] = res2_fp16[11:0];
    for (int i = 0; i < 4; i++) begin
        mant_fp8[i] = sums[i][3:0];
    end
end

endmodule
