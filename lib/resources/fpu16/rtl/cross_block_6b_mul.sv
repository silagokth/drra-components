module cross_block_6b_mul (
    input  logic [11:0] tR_partial_products [0:1][0:1],
    input  logic [11:0] bL_partial_products [0:1][0:1],
    output logic [24:0] cb_sum
);

// 111111 x 111111 = 111110_000001 -> max value of partial product

logic [12:0] sum_s1 [0:1];
logic [23:0] cb_psum_s1 [0:1];

// STAGE 1
always_comb begin
    // Cross-block partials (top-right and bottom-left) --- Using 12 as a base for the shifts
    // to use cheaper adders. The final result will be shifted by 12 left 
    sum_s1[0]     =  tR_partial_products[0][1] + tR_partial_products[1][0];      // 13b, max value = 1_111100_000010 - shift 6 
    cb_psum_s1[0] = {tR_partial_products[1][1],  tR_partial_products[0][0]};     // shifts: 12, 0 --- 24b, max value = 111110_000001_111110_000001

    sum_s1[1] =      bL_partial_products[0][1] + bL_partial_products[1][0];      // 13b, max value = 1_111100_000010 - shift 6 
    cb_psum_s1[1] = {bL_partial_products[1][1],  bL_partial_products[0][0]};     // shifts: 12, 0
end

// STAGE 2 
logic [13:0] cb_sum_s2;
logic [24:0] cb_psum_s2;

always_comb begin
    cb_sum_s2  = sum_s1[0] + sum_s1[1];                     // 14b, max value = 11_111000_000100 - shift 6    
    cb_psum_s2 = cb_psum_s1[0] + cb_psum_s1[1];             // 25b, max value = 1_111100_000011_111100_000010  
end

// STAGE 3 
logic [18:0] t_cb_sum;

always_comb begin
    t_cb_sum = cb_sum_s2 + cb_psum_s2[24:6];                                     
    cb_sum   = {t_cb_sum, cb_psum_s2[5:0]};                 // 25b, max value = 1_111111_111100_000000_000010 - shift 12
end

endmodule

