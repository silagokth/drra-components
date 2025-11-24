module adder_tree #(
    parameter WIDTH = 6, 
    parameter GRID_SIZE = 24 / WIDTH
)(
    input  logic [2*WIDTH-1:0] partial_products [0:GRID_SIZE-1][0:GRID_SIZE-1],
    output logic [21:0] sum_1_fp16,
    output logic [21:0] sum_2_fp16,  
    output logic [47:0] sum_fp32
);

logic [2*WIDTH-1:0] tL_partial_products[0:GRID_SIZE/2-1][0:GRID_SIZE/2-1];
logic [2*WIDTH-1:0] bR_partial_products[0:GRID_SIZE/2-1][0:GRID_SIZE/2-1];  // Reindexed for [0:3][0:3] convenience
logic [23:0] tl_sum, br_sum;   

// Assign from global 8x8 or 6x6 grid
always_comb begin
    for (int i = 0; i < GRID_SIZE/2; i++) begin
        for (int j = 0; j < GRID_SIZE/2; j++) begin
            tL_partial_products[i][j] = partial_products[i][j];
            bR_partial_products[i][j] = partial_products[i+GRID_SIZE/2][j+GRID_SIZE/2];
        end
    end
end

topL_botR_block_6b_mul tl_br_block_inst (
    .tL_partial_products(tL_partial_products),
    .bR_partial_products(bR_partial_products),
    .sum_1_fp16(tl_sum),
    .sum_2_fp16(br_sum)
);

logic [2*WIDTH-1:0] tR_partial_products[0:GRID_SIZE/2-1][0:GRID_SIZE/2-1];
logic [2*WIDTH-1:0] bL_partial_products[0:GRID_SIZE/2-1][0:GRID_SIZE/2-1];  
logic [24:0] cb_sum;        //not sure about that

// Extract the top-right block from the global grid
always_comb begin
    for (int k = 0; k < GRID_SIZE/2; k++) begin
        for (int l = GRID_SIZE/2; l < GRID_SIZE; l++) begin
            tR_partial_products[k][l-GRID_SIZE/2] = partial_products[k][l]; // top-right
        end
    end
end
// Extract the bottom-left block from the global grid
always_comb begin
    for (int k = GRID_SIZE/2; k < GRID_SIZE; k++) begin
        for (int l = 0; l < GRID_SIZE/2; l++) begin
            bL_partial_products[k-GRID_SIZE/2][l] = partial_products[k][l]; // bottom-left
        end
    end
end

cross_block_6b_mul cb_inst (
    .tR_partial_products(tR_partial_products),
    .bL_partial_products(bL_partial_products),
    .cb_sum(cb_sum)
);
 
// STAGE 6
logic [47:0] tl_br_sum, t_total_sum, total_sum;

// Using 12 as a base for the shifts to use cheaper adders. 
// The final result should be shifted 12 left. 
always_comb begin       
    tl_br_sum   = {br_sum, tl_sum};                     // 48b, max value = 111111_111110_000000_000001_111111_111110_000000_000001
    t_total_sum = cb_sum + tl_br_sum[47:12];             
    total_sum   = {t_total_sum, tl_br_sum[11:0]};       // 48b, max value = 111111_111111_111111_111110_000000_000000_000000_000001
end

// Assign outputs
assign sum_1_fp16 = tl_sum[21:0];
assign sum_2_fp16 = br_sum[21:0];
assign sum_fp32   = total_sum;
 
endmodule
