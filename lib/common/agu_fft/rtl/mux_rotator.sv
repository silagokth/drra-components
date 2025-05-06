module mux_rotator #(
    parameter AGU_BITWIDTH = 8, // 256 FFT
    parameter STAGE_WIDTH = 4
) (
    input  logic [AGU_BITWIDTH-1:0] n_points,
    input  logic [AGU_BITWIDTH-1:0] addr_in,
    input  logic [STAGE_WIDTH-1:0] curr_stage,
    output logic [AGU_BITWIDTH-1:0] addr_out
);

    logic [AGU_BITWIDTH-2:0] select1;
    logic [AGU_BITWIDTH-2:0] select2;
    logic [AGU_BITWIDTH-1:0] mux_signal;
    logic [STAGE_WIDTH-1:0] stages;

    int j;
    always_comb begin
        stages = 0;
        for (j = AGU_BITWIDTH-1; j >= 0; j--) begin
            if (n_points[j]) begin
                stages = j;
                break;
            end
        end
    end

    always_comb begin
        select1 = {AGU_BITWIDTH-1{1'b1}} >> (AGU_BITWIDTH - stages) + curr_stage;
        select2 = {1'b1, {AGU_BITWIDTH-2{1'b0}}} >> (AGU_BITWIDTH - stages) + curr_stage;
    end

    genvar i;
    generate
        for (i = 0; i < AGU_BITWIDTH-1; i++) begin : mux1_gen
            assign mux_signal[i] = select1[i] ? addr_in[i+1] : addr_in[i];
        end
        assign mux_signal[AGU_BITWIDTH-1] = addr_in[AGU_BITWIDTH-1];

        assign addr_out[0] = mux_signal[0];
        for (i = 1; i < AGU_BITWIDTH; i++) begin : mux2_gen
            assign addr_out[i] = select2[i-1] ? addr_in[0] : mux_signal[i];
        end
    endgenerate
    
endmodule