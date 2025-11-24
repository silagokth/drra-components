module shift_exp_comb (
    input  logic [7:0]  shift_count_fp32      , 
    input  logic [4:0]  shift_count_fp16 [0:1], 
    input  logic [4:0]  shift_count_fp8  [0:3],
    input  logic [7:0]  exp_fp32              ,
    input  logic [4:0]  exp_fp16         [0:1],
    input  logic [4:0]  exp_fp8          [0:3],
    input  logic [23:0] mant_fp32             ,
    input  logic [10:0] mant_fp16        [0:1],
    input  logic [2:0]  mant_fp8         [0:3],
    output logic [7:0]  f_exp_fp32            ,
    output logic [4:0]  f_exp_fp16       [0:1],
    output logic [4:0]  f_exp_fp8        [0:3],
    output logic [23:0] f_mant_fp32           ,
    output logic [10:0] f_mant_fp16      [0:1],
    output logic [2:0]  f_mant_fp8       [0:3]
);

always_comb begin
    f_exp_fp32  = exp_fp32   - shift_count_fp32;
    f_mant_fp32 = mant_fp32 << shift_count_fp32;

    for (int i = 0; i < 2; i++) begin
        f_exp_fp16 [i] = exp_fp16 [i]  - shift_count_fp16[i];
        f_mant_fp16[i] = mant_fp16[i] << shift_count_fp16[i];
    end
    
    for (int i = 0; i < 4; i++) begin
        f_exp_fp8 [i]  = exp_fp8  [i]  - shift_count_fp8 [i];
        f_mant_fp8[i]  = mant_fp8 [i] << shift_count_fp8 [i];
    end
end

endmodule
