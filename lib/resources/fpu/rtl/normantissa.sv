module normantissa_comb (
    input  logic [7:0]  exp_fp32,
    input  logic [4:0]  exp_fp16  [0:1],
    input  logic [4:0]  exp_fp8   [0:3],
    input  logic [24:0] mant_fp32      ,
    input  logic [11:0] mant_fp16 [0:1],
    input  logic [3:0]  mant_fp8  [0:3],

    output logic [7:0]  norm_exp_fp32  ,
    output logic [4:0]  norm_exp_fp16  [0:1],
    output logic [4:0]  norm_exp_fp8   [0:3],
    output logic [23:0] norm_mant_fp32      ,
    output logic [10:0] norm_mant_fp16 [0:1],
    output logic [2:0]  norm_mant_fp8  [0:3]
);

always_comb begin
    if (mant_fp32 == 0) begin
        norm_mant_fp32 = '0;
        norm_exp_fp32  = '0;    
    end else begin 
        if (mant_fp32[24]) begin
            norm_mant_fp32 = mant_fp32[24:1];
            norm_exp_fp32  = exp_fp32 + 1;     
        end else begin
            norm_mant_fp32 = mant_fp32[23:0];
            norm_exp_fp32  = exp_fp32; 
        end
    end
    for (int i = 0; i < 2; i++) begin
        if (mant_fp16[i] == 0) begin
            norm_mant_fp16[i] = '0;
            norm_exp_fp16 [i] = '0;
        end else begin
            if (mant_fp16[i][11]) begin
                norm_mant_fp16[i] = mant_fp16[i][11:1];     
                norm_exp_fp16 [i] = exp_fp16 [i] + 1;
            end else begin
                norm_mant_fp16[i] = mant_fp16[i][10:0];     
                norm_exp_fp16 [i] = exp_fp16 [i];
            end
        end
    end
    for (int i = 0; i < 4; i++) begin
        if (mant_fp8[i] == 0) begin
            norm_mant_fp8[i] = '0;
            norm_exp_fp8 [i] = '0;
        end else begin
            if (mant_fp8[i][3]) begin
                norm_mant_fp8[i] = mant_fp8[i][3:1];     
                norm_exp_fp8 [i] = exp_fp8 [i] + 1;
            end else begin
                norm_mant_fp8[i] = mant_fp8[i][2:0];     
                norm_exp_fp8 [i] = exp_fp8 [i];
            end
        end
    end
end

endmodule


