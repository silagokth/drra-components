module driver2sub (
    input  logic [1:0]  src_format,     // Format of input operands
    input  logic [8:0]  t_exp_fp32_s2,
    input  logic [5:0]  t_exp_s2 [0:3],
    output logic [5:0]  minuend [0:3],
    output logic [5:0]  subtrahend [0:3]
);

//2s complement of the biases 
localparam logic [8:0] BIAS_VEC_FP32 = {9'b110000001};    
localparam logic [5:0] BIAS_VEC_FP   = {6'b110001}; 

always_comb begin
    if (src_format == 2'b00) begin              // FP32
        minuend[0]    = t_exp_fp32_s2[5:0];
        subtrahend[0] = BIAS_VEC_FP32[5:0];
        minuend[1]    = {3'b0, t_exp_fp32_s2[8:6]};
        subtrahend[1] = {3'b0, BIAS_VEC_FP32[8:6]};
        for (int i = 2; i < 4; i++) begin
            minuend[i]    = t_exp_s2[i];
            subtrahend[i] = BIAS_VEC_FP;
        end
    end else begin
        for (int i = 0; i < 4; i++) begin
            minuend[i]    = t_exp_s2[i];
            subtrahend[i] = BIAS_VEC_FP;
        end
    end
end

endmodule
