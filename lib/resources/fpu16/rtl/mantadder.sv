module adder_r #(
    parameter WIDTH = 3
)(
    input  logic [WIDTH-1:0] a_in ,
    input  logic [WIDTH-1:0] b_in ,
    output logic [WIDTH  :0] sum_out 
);
    assign sum_out = a_in + b_in;
endmodule

module mantadder (
    input  logic [ 1:0] src_format,   
    input  logic sign_a_fp32,
    input  logic sign_b_fp32,
    input  logic sign_a_fp16 [0:1],
    input  logic sign_b_fp16 [0:1],
    input  logic sign_a_fp8  [0:3],
    input  logic sign_b_fp8  [0:3],
    input  logic [23:0] aligned_mant_a_fp32,
    input  logic [23:0] aligned_mant_b_fp32,
    input  logic [10:0] aligned_mant_a_fp16 [0:1],
    input  logic [10:0] aligned_mant_b_fp16 [0:1],
    input  logic [2:0]  aligned_mant_a_fp8  [0:3],
    input  logic [2:0]  aligned_mant_b_fp8  [0:3],
    output logic        sign_fp32,
    output logic        sign_fp16 [0:1],
    output logic        sign_fp8  [0:3],
    output logic [24:0] mant_fp32      ,
    output logic [11:0] mant_fp16 [0:1],
    output logic [3:0]  mant_fp8  [0:3]
);

logic [27:0] addend_fp32, augend_fp32;
logic [11:0] addend_fp16 [0:1], augend_fp16 [0:1];
logic [3:0]  addend_fp8  [0:3], augend_fp8  [0:3];
logic [3:0]  addend  [0:6], augend  [0:6];
logic [4:0]  sums    [0:6];

always_comb begin
    if (sign_a_fp32 == sign_b_fp32) begin
        addend_fp32 = {4'b0, aligned_mant_a_fp32};     
        augend_fp32 = {4'b0, aligned_mant_b_fp32};
        sign_fp32   = sign_a_fp32;
    end else begin
        if (aligned_mant_a_fp32 >= aligned_mant_b_fp32) begin
            addend_fp32 = {4'b0,  aligned_mant_a_fp32}; 
            augend_fp32 = {4'b0, ~aligned_mant_b_fp32} + 1'b1;     //2s complement
            sign_fp32   = sign_a_fp32;                                
        end else begin
            addend_fp32 = {4'b0,  aligned_mant_b_fp32};
            augend_fp32 = {4'b0, ~aligned_mant_a_fp32} + 1'b1;
            sign_fp32   = sign_b_fp32;
        end
    end
end

always_comb begin
    for (int i = 0; i < 2; i++) begin
        if (sign_a_fp16[i] == sign_b_fp16[i]) begin
            addend_fp16[i] = {1'b0, aligned_mant_a_fp16[i]}; 
            augend_fp16[i] = {1'b0, aligned_mant_b_fp16[i]}; 
            sign_fp16  [i] = sign_a_fp16[i];
        end else begin
            if (aligned_mant_a_fp16[i] >= aligned_mant_b_fp16[i]) begin
                addend_fp16[i] = {1'b0,  aligned_mant_a_fp16[i]}; 
                augend_fp16[i] = {1'b0, ~aligned_mant_b_fp16[i]} + 1; 
                sign_fp16  [i] = sign_a_fp16[i];
            end else begin
                addend_fp16[i] = {1'b0, ~aligned_mant_a_fp16[i]} + 1; 
                augend_fp16[i] = {1'b0,  aligned_mant_b_fp16[i]}; 
                sign_fp16  [i] = sign_b_fp16[i];
            end
        end
    end
end

always_comb begin
    for (int i = 0; i < 4; i++) begin
        if (sign_a_fp8[i] == sign_b_fp8[i]) begin
            addend_fp8[i] = {1'b0, aligned_mant_a_fp8[i]};
            augend_fp8[i] = {1'b0, aligned_mant_b_fp8[i]};
            sign_fp8  [i] = sign_a_fp8[i];
        end else begin
            if (aligned_mant_a_fp8[i] >= aligned_mant_b_fp8[i]) begin
                addend_fp8[i] = {1'b0,  aligned_mant_a_fp8[i]};
                augend_fp8[i] = {1'b0, ~aligned_mant_b_fp8[i]} + 1;
                sign_fp8  [i] = sign_a_fp8[i];
            end else begin
                addend_fp8[i] = {1'b0, ~aligned_mant_a_fp8[i]} + 1;
                augend_fp8[i] = {1'b0,  aligned_mant_b_fp8[i]};
                sign_fp8  [i] = sign_b_fp8[i];
            end
        end
    end
end

always_comb begin
    case (src_format) 
        2'b00 : begin
            for (int i = 0; i < 7; i++) begin
                addend[i] = addend_fp32[4*i +: 4];
                augend[i] = augend_fp32[4*i +: 4];
            end
        end
        2'b01 : begin
            for (int i = 0; i < 2; i++) begin
                for (int j = 0; j < 3; j++) begin
                    addend[3*i + j] = addend_fp16[i][4*j +: 4];
                    augend[3*i + j] = augend_fp16[i][4*j +: 4];
                end
            end    
            for (int i = 6; i < 7; i++) begin           // we don't care
                addend[i] = addend_fp32[4*i +: 4];
                augend[i] = augend_fp32[4*i +: 4];
            end
        end
        2'b10 : begin
            for (int i = 0; i < 4; i++) begin
                addend[i] = addend_fp8[i];    
                augend[i] = augend_fp8[i];    
            end
            for (int i = 4; i < 7; i++) begin           // we don't care
                addend[i] = addend_fp32[4*i +: 4];
                augend[i] = augend_fp32[4*i +: 4];
            end
        end
        default: begin
            for (int i = 0; i < 7; i++) begin
                addend[i] = '0;
                augend[i] = '0;
            end    
        end
    endcase
end

generate
    for (genvar i = 0; i < 7; i++) begin : adder_array_loop
        adder_r #(.WIDTH(4)) adder_inst (
            .a_in(addend[i]),
            .b_in(augend[i]),
            .sum_out(sums[i])
        );
    end
endgenerate

tree_mantadder u_tree (
    .sums(sums),
    .mant_fp32(mant_fp32),
    .mant_fp16(mant_fp16),
    .mant_fp8(mant_fp8)
);

endmodule
