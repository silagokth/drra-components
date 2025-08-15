module twiddle_addr_r4 #(
    parameter AGU_BITWIDTH = 8, // 256 FFT
    parameter STAGE_WIDTH = 4
) (
    input  logic [AGU_BITWIDTH-1:0] n_points,
    input  logic [1:0] radix,   // DIT for radix-2, DIF for radix-4 (other modes not implemented yet)
    input  logic n_bu,
    input  logic bu_index,
    input  logic [1:0] port_index,
    input  logic [AGU_BITWIDTH-1:0] addr_in,
    input  logic [STAGE_WIDTH-1:0] curr_stage,
    output logic [AGU_BITWIDTH-1:0] addr_out
);

    logic [STAGE_WIDTH-1:0] stages;
    logic [AGU_BITWIDTH-2:0] addr_out_r2; // radix-2 uses N/2 addresses
    logic [AGU_BITWIDTH-1:0] addr_out_r4;   // radix-4 uses 3N/4 addresses

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

    logic [AGU_BITWIDTH-2:0] addr_twid;
    logic [AGU_BITWIDTH-1:0] counter_r4;

    //---- Radix-2 twiddle address generation (DIF) ----
    always_comb begin
        if (bu_index == 0) begin
            addr_twid = addr_in;
        end else begin
            addr_twid = addr_in + n_points/4;
        end
        if (curr_stage == 0) begin
            addr_out_r2 = addr_twid;
        end else begin
            addr_out_r2 = (addr_twid << curr_stage) & ((1 << (stages - 1)) - 1);
        end
    end

    //---- Radix-4 twiddle address generation (DIF) ----
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