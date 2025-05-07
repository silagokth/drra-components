module twiddle_addr_v3 #(
    parameter AGU_BITWIDTH = 8, // 256 FFT
    parameter STAGE_WIDTH = 4
) (
    input  logic [AGU_BITWIDTH-1:0] n_points,
    input  logic decimation,    // DIT for radix-2, DIF for radix-4 (other modes not implemented yet)
    input  logic [1:0] radix,
    input  logic n_bu,
    input  logic bu_index,
    input  logic [1:0] port_index,
    input  logic [AGU_BITWIDTH-1:0] addr_in,
    input  logic [STAGE_WIDTH-1:0] curr_stage,
    output logic [AGU_BITWIDTH-1:0] addr_out
);

    logic [STAGE_WIDTH-1:0] stages;
    logic [AGU_BITWIDTH-1:0] addr_out_r2;
    logic [AGU_BITWIDTH-1:0] addr_out_r4;

    int j;
    always_comb begin
        stages = 0;
        for (j = AGU_BITWIDTH-1; j >= 0; j--) begin
            if (n_points[j]) begin
                if (radix == 0) begin
                    stages = j;
                end else if (radix == 1) begin
                    stages = j/2;
                end
                break;
            end
        end
    end

    logic [AGU_BITWIDTH-1:0] addr_twid;
    logic [AGU_BITWIDTH*2-1:0] temp_addr;
    logic [AGU_BITWIDTH-1:0] counter_r4;

    //---- Radix-2 twiddle address generation (DIT)----
    always_comb begin
        if (bu_index == 0) begin
            addr_twid = addr_in;
        end else begin
            addr_twid = addr_in + n_points/2;
        end
    end

    always_comb begin
        if (curr_stage == 0) begin
            temp_addr = 0;
        end else begin
            temp_addr = (addr_twid >> (stages - 1 - curr_stage)) << AGU_BITWIDTH;
        end
    end
    // bit reversal
    genvar i;
    generate
        for (i = 0; i < AGU_BITWIDTH; i++) begin : bitrev_gen
            assign addr_out_r2[i] = temp_addr[stages - 2 - i + AGU_BITWIDTH];
        end
    endgenerate

    //---- Radix-4 twiddle address generation (DIF)----
    always_comb begin
        if (curr_stage == stages - 1) begin
            counter_r4 = 0;
        end else begin
            counter_r4 = addr_in & ((1 << 2*(stages - curr_stage - 1)) - 1);
        end
        addr_out_r4 = (counter_r4 * port_index) << 2*curr_stage;
    end

    always_comb begin
        addr_out = 0;
        case (radix)
            2'b00: addr_out = addr_out_r2;
            2'b01: addr_out = addr_out_r4;
            default: addr_out = 0;
        endcase
    end

endmodule