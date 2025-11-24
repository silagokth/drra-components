module topL_botR_block_6b_mul (
    input  logic [11:0] tL_partial_products [0:1][0:1],
    input  logic [11:0] bR_partial_products [0:1][0:1],
    output logic [23:0] sum_1_fp16,
    output logic [23:0] sum_2_fp16 
);

// 111111 x 111111 = 111110_000001 -> max value of partial product

// STAGE 1
logic [12:0] sum_s1  [0:1];
logic [23:0] psum_s1 [0:1];

always_comb begin
    // Top Left --- 24b, max value = 111110_000001_111110_000001
    sum_s1[0]  =  tL_partial_products[0][1] + tL_partial_products[1][0];       // 13b max value = 1_111100_000010 - shift 6    
    psum_s1[0] = {tL_partial_products[1][1],  tL_partial_products[0][0]};      // shifts: 12,  0

    // Bottom Right --- Using 24 as a base for the shifts to use cheaper adders
    // since each shift of the partial products in FP16 is calculated as 6*(i-2 + j-2). 
    // The final result will be shifted by 24 left 
    sum_s1[1]  =  bR_partial_products[0][1] + bR_partial_products[1][0];       // 13b max value = 1_111100_000010 - shift 6    
    psum_s1[1] = {bR_partial_products[1][1],  bR_partial_products[0][0]};      // shifts: 12,  0
end

// STAGE 2 
logic [17:0] tl_tsum, br_tsum;
logic [23:0] tl_sum, br_sum;   

always_comb begin
    tl_tsum = sum_s1[0] + psum_s1[0][23:6];                 
    tl_sum = {tl_tsum, psum_s1[0][5:0]};                     // 24b, max value = 111111_111110_000000_000010
end

always_comb begin
    br_tsum = sum_s1[1] + psum_s1[1][23:6];                 
    br_sum = {br_tsum, psum_s1[1][5:0]};                     // 24b, max value = 111111_111110_000000_000010
end

assign sum_1_fp16 = tl_sum;
assign sum_2_fp16 = br_sum;

endmodule

