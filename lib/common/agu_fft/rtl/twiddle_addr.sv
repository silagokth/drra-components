module twiddle_addr #(
    parameter AGU_BITWIDTH = 8, // 256 FFT
    parameter STAGE_WIDTH = 4
) (
    input  logic [AGU_BITWIDTH-1:0] n_points,
    input  logic [AGU_BITWIDTH-1:0] addr_in,
    input  logic [STAGE_WIDTH-1:0] curr_stage,
    output logic [AGU_BITWIDTH-1:0] addr_out
);

    logic [AGU_BITWIDTH*2-1:0] temp_addr;
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
        if (curr_stage == 0) begin
            temp_addr = 0;
        end else begin
            temp_addr = (addr_in >> (stages - 1 - curr_stage)) << AGU_BITWIDTH;
        end
    end

    genvar i;
    generate
        for (i = 0; i < AGU_BITWIDTH; i++) begin : bitrev_gen
            assign addr_out[i] = temp_addr[stages - 2 - i + AGU_BITWIDTH];
        end
    endgenerate

endmodule