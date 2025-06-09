module mux_rotator_r4 #(
    parameter AGU_BITWIDTH = 8, // 256 FFT
    parameter STAGE_WIDTH = 4
) (
    input  logic [AGU_BITWIDTH-1:0] n_points,
    input  logic [1:0] radix,
    input  logic n_bu,
    input  logic bu_index,
    input  logic [1:0] port_index,
    input  logic [AGU_BITWIDTH-1:0] addr_in,
    input  logic [STAGE_WIDTH-1:0] curr_stage,
    output logic [AGU_BITWIDTH-1:0] addr_out
);

    logic [AGU_BITWIDTH-1:0] select1;
    logic [AGU_BITWIDTH-1:0] select2;
    logic [AGU_BITWIDTH-1:0] mux_signal;
    logic [AGU_BITWIDTH-1:0] addr_rot;
    logic [AGU_BITWIDTH-1:0] rot_out;
    
    logic [AGU_BITWIDTH/2-1:0][1:0] mux_signal_r4;
    logic [AGU_BITWIDTH-1:0] rot_out_r4;

    logic [STAGE_WIDTH-1:0] stages;

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

    always_comb begin
        addr_rot = 0;
        if (radix == 0) begin
            if (n_bu == 0) begin
                if (port_index == 0) begin
                    addr_rot = addr_in;
                end else begin
                    addr_rot = addr_in + n_points/2;
                end
            end else begin
                if (bu_index == 0) begin
                    if (port_index == 0) begin
                        addr_rot = addr_in;
                    end else begin
                        addr_rot = addr_in + n_points/2;
                    end
                end else begin
                    if (port_index == 0) begin
                        addr_rot = addr_in + n_points/4;
                    end else begin
                        addr_rot = addr_in + 3*n_points/4;
                    end
                end
            end
        end else if (radix == 1) begin
            if (port_index == 0) begin
                addr_rot = addr_in;
            end else if (port_index == 1) begin
                addr_rot = addr_in + n_points/4;
            end else if (port_index == 2) begin
                addr_rot = addr_in + n_points/2;
            end else if (port_index == 3) begin
                addr_rot = addr_in + 3*n_points/4;
            end
        end
    end

    always_comb begin
        if (curr_stage == 0) begin
            select1 = '0;
            select2 = '0;
        end else begin
            select1 = select1 | 1 << stages - curr_stage - 1;
            select2 = 1 << stages - curr_stage - 1;
        end
    end

    genvar i;
    generate
        for (i = 1; i < AGU_BITWIDTH; i++) begin
            assign mux_signal[i] = select1[i-1] ? addr_rot[i-1] : addr_rot[i];
        end
        assign mux_signal[0] = addr_rot[0];

        for (i = 0; i < AGU_BITWIDTH; i++) begin
            assign rot_out[i] = select2[i] ? addr_rot[stages-1] : mux_signal[i];
        end

        // for radix-4 rotate groups of 2 bits
        for (i = 1; i < AGU_BITWIDTH/2; i++) begin
            assign mux_signal_r4[i] = select1[i-1] ? addr_rot[i*2-2 +:2] : addr_rot[i*2 +:2];
        end
        assign mux_signal_r4[0] = addr_rot[1:0];

        for (i = 0; i < AGU_BITWIDTH/2; i++) begin
            assign rot_out_r4[i*2 +:2] = select2[i] ? addr_rot[(stages-1)*2 +:2] : mux_signal_r4[i];
        end      
    endgenerate

    always_comb begin
        addr_out = 0;
        case (radix)
            2'b00: addr_out = rot_out;
            2'b01: addr_out = rot_out_r4;
            default: addr_out = 0;
        endcase
    end
    
endmodule