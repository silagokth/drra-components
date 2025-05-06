module agu_fft #(
    parameter AGU_BITWIDTH = 16,
    parameter DELAY_WIDTH = 5,
    parameter STAGE_WIDTH = 4
) (
    input  logic clk,
    input  logic rst_n,
    input  logic activate,
    input  logic [1:0] radix,     // 0: radix 2, ... - not used
    input  logic even_odd,  // even: 0, odd: 1
    input  logic mode,    // mode 0: twiddle, mode 1: fft data
    input  logic n_bu,    // number of butterfly in the cell -1
    input  logic bu_index,  // index of the butterfly 
    input  logic [AGU_BITWIDTH-1:0] n_points,
    input  logic [DELAY_WIDTH-1:0] delay,
    input  logic load_config,
    output logic address_valid,
    output logic [AGU_BITWIDTH-1:0] address
);

    typedef enum logic [1:0] {
        IDLE,
        ADDR,
        DELAY
    } state_t;

    // FSM
    state_t state, next_state;
    logic next_valid;

    // config regs
    logic mode_reg;
    logic [AGU_BITWIDTH-1:0] n_points_reg;
    logic [DELAY_WIDTH-1:0] delay_reg;
    logic [1:0] n_bu_reg;

    // addresses
    logic [AGU_BITWIDTH-1:0] next_address;
    logic [AGU_BITWIDTH-1:0] address_in_rot;
    logic [AGU_BITWIDTH-1:0] next_address_rot;
    logic [AGU_BITWIDTH-1:0] address_in_twid;
    logic [AGU_BITWIDTH-1:0] next_address_twid;
    
    // counters
    logic [DELAY_WIDTH-1:0] delay_counter;
    logic [DELAY_WIDTH-1:0] delay_counter_next;
    logic [AGU_BITWIDTH-1:0] address_counter;
    logic [AGU_BITWIDTH-1:0] address_counter_next;
    logic [STAGE_WIDTH-1:0] stage_counter;
    logic [STAGE_WIDTH-1:0] stage_counter_next;

    // config loading
    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            n_points_reg <= '0;
            delay_reg <= '0;
            mode_reg <= 0;
            n_bu_reg <= '0;
        end else begin
            if (load_config) begin
                n_points_reg <= n_points;
                delay_reg <= delay;
                mode_reg <= mode;
                n_bu_reg <= n_bu + 1'b1;
            end
        end
    end

    logic [AGU_BITWIDTH-1:0] counter_0_0, counter_0_1, counter_1_0, counter_1_1;
    logic [AGU_BITWIDTH-1:0] twid_counter_0, twid_counter_1;

    assign counter_0_0 = address_counter * 2;
    assign counter_0_1 = address_counter * 2 + 1;
    assign counter_1_0 = address_counter * 2 + n_points_reg/n_bu_reg;
    assign counter_1_1 = address_counter * 2 + n_points_reg/n_bu_reg + 1;

    assign twid_counter_0 = address_counter;
    assign twid_counter_1 = address_counter + n_points_reg/(2*n_bu_reg);

    always_comb begin
        address_in_twid = 0;
        address_in_rot = 0;
        if (n_bu_reg == 1) begin
            address_in_twid = twid_counter_0;
            if (!even_odd) begin
                address_in_rot = counter_0_0;
            end else begin
                address_in_rot = counter_0_1;
            end
        end else begin
            if (bu_index == 0) begin
                address_in_twid = twid_counter_0;
                if (!even_odd) begin
                    address_in_rot = counter_0_0;
                end else begin
                    address_in_rot = counter_0_1;
                end
            end else begin
                address_in_twid = twid_counter_1;
                if (!even_odd) begin
                    address_in_rot = counter_1_0;
                end else begin
                    address_in_rot = counter_1_1;
                end
            end
        end
    end

    mux_rotator #(
        .AGU_BITWIDTH(AGU_BITWIDTH),
        .STAGE_WIDTH(STAGE_WIDTH)
    ) mux_rotator_r2_inst (
        .n_points(n_points_reg),
        .addr_in(address_in_rot),
        .curr_stage(stage_counter),
        .addr_out(next_address_rot)
    );

    twiddle_addr #(
        .AGU_BITWIDTH(AGU_BITWIDTH),
        .STAGE_WIDTH(STAGE_WIDTH)
    ) twiddle_addr_inst (
        .n_points(n_points_reg),
        .addr_in(address_in_twid),
        .curr_stage(stage_counter),
        .addr_out(next_address_twid)
    );

    assign next_address = mode_reg ? next_address_rot : next_address_twid; // mode 1: fft data, mode 0: twiddle

    logic stage_finish, computation_finish;
    logic [STAGE_WIDTH-1:0] stages;

    int j;
    always_comb begin
        stages = 0;
        for (j = AGU_BITWIDTH-1; j >= 0; j--) begin
            if (n_points_reg[j]) begin
                stages = j;
                break;
            end
        end
    end
    
    assign stage_finish = (address_counter >= n_points_reg/(2*n_bu_reg)-1);
    assign computation_finish = (stage_counter == stages-1);   

    // FSM
    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state   <= IDLE;
            address <= 0;
            address_valid <= 0;
        end else begin
            state   <= next_state;
            address <= next_address;
            address_valid <= next_valid;
        end
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            delay_counter <= 0;
            stage_counter <= 0;
            address_counter <= 0;
        end else begin
            delay_counter <= delay_counter_next;
            stage_counter <= stage_counter_next;
            address_counter <= address_counter_next;
        end
    end

    always_ff @(posedge clk or negedge rst_n) begin
        case (state)
            IDLE: begin
                next_valid <= 0;
            end
            ADDR: begin
                next_valid <= 1;
            end
            DELAY: begin
                next_valid <= 1;
            end
            default: begin
                next_valid <= 0;
            end
        endcase
    end

    always_comb begin
        next_state = state;
        delay_counter_next = delay_counter;
        stage_counter_next = stage_counter;
        address_counter_next = address_counter;

        case (state)
            IDLE: begin
                stage_counter_next = 0;
                delay_counter_next = 0;
                address_counter_next = 0;
                if (activate) begin
                    next_state = ADDR;
                end else begin
                    next_state = IDLE;
                end
            end

            ADDR: begin
                if (stage_finish) begin
                    if (computation_finish) begin
                        if (delay_reg > 0) begin
                            next_state = DELAY;
                        end else begin
                            next_state = IDLE;
                            address_counter_next = 0;
                            stage_counter_next = 0;
                        end
                    end else begin
                        if (delay_reg > 0) begin
                            next_state = DELAY;
                        end else begin
                            next_state = ADDR;
                            address_counter_next = 0;
                            stage_counter_next = stage_counter + 1;                 
                        end
                    end
                end else begin
                    if (delay_reg > 0) begin
                        next_state = DELAY;
                    end else begin
                        next_state = ADDR;
                        address_counter_next = address_counter + 1;
                    end
                end
            end
            
            DELAY: begin
                delay_counter_next = delay_counter + 1;
                if (delay_counter_next == delay_reg) begin
                    delay_counter_next = 0;
                    if (stage_finish) begin
                        if (computation_finish) begin
                            next_state = IDLE;
                            address_counter_next = 0;
                            stage_counter_next = 0;
                        end else begin
                            next_state = ADDR;
                            address_counter_next = 0;
                            stage_counter_next = stage_counter + 1;
                        end
                    end else begin
                        next_state = ADDR;
                        address_counter_next = address_counter + 1;
                    end
                end else begin
                    next_state = DELAY;
                    delay_counter_next = delay_counter + 1;
                end
            end
            
            default: begin
                next_state = IDLE;
                stage_counter_next = 0;
                delay_counter_next = 0;
                address_counter_next = 0;
            end
        endcase
    end

endmodule