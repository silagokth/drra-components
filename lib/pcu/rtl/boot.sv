module boot
import pcu_pkg::*;
(
    input logic clk,
    input logic rst_n,

    // External interface
    input logic ext_sel,                    // Chip select
    input logic [ADDR_WIDTH-1:0] ext_addr,  // Local address
    input logic [DATA_WIDTH-1:0] ext_wdata, // Write data
    input logic [DATA_WIDTH/8-1:0] ext_wen, // Write enable per byte
    output logic [DATA_WIDTH-1:0] ext_rdata, // Read data

    // Custom signals
    input logic start_in,
    output logic start_out
);

logic [DATA_WIDTH-1:0] delay_reg;
logic [DATA_WIDTH-1:0] counter;

enum logic {
    IDLE,
    BUSY
} state;

always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
        delay_reg <= '0;
    end else begin
        if (ext_sel && |ext_wen && ext_addr == 0) begin
            for (int i = 0; i < DATA_WIDTH/8; i++) begin
                if (ext_wen[i]) begin
                    delay_reg[i*8 +: 8] <= ext_wdata[i*8 +: 8];
                end
            end
        end
    end
end

// Combinational read - always responds immediately
always_comb begin
    if (ext_sel) begin
        case (ext_addr)
            0: ext_rdata = delay_reg;           // Delay register
            1: ext_rdata = {{DATA_WIDTH-1{1'b0}}, (state == BUSY)}; // Status register
            default: ext_rdata = '0;            // Unused addresses
        endcase
    end else begin
        ext_rdata = '0;                        // Not selected
    end
end

always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
        state <= IDLE;
        counter <= '0;
        start_out <= 1'b0;
    end else begin
        // Default: clear start_out (pulse behavior)
        start_out <= 1'b0;
        
        case (state)
            IDLE: begin
                if (start_in) begin
                    state <= BUSY;
                    counter <= delay_reg;
                end
            end
            BUSY: begin
                if (counter == 1) begin
                    counter <= '0;
                    state <= IDLE;
                    start_out <= 1'b1;  // Generate 1-cycle pulse
                end else begin
                    counter <= counter - 1;
                end
            end
        endcase
    end
end

// SystemVerilog Assertions for Error Detection
`ifdef ASSERT_ON
    // Assertion 1: No start_in while module is busy
    property no_start_while_busy;
        @(posedge clk) disable iff (!rst_n)
        (state == BUSY) |-> !start_in;
    endproperty
    assert_no_start_while_busy: assert property(no_start_while_busy)
        else $error("[BOOT] ERROR: start_in asserted while module is BUSY at time %0t", $time);

    // Assertion 2: delay_reg should not be zero when starting
    property valid_delay_on_start;
        @(posedge clk) disable iff (!rst_n)
        (state == IDLE && start_in) |-> (delay_reg != 0);
    endproperty
    assert_valid_delay: assert property(valid_delay_on_start)
        else $error("[BOOT] ERROR: Attempting to start with zero delay at time %0t", $time);

    // Assertion 3: Counter should never underflow
    property counter_no_underflow;
        @(posedge clk) disable iff (!rst_n)
        (state == BUSY && counter > 1) |-> ##1 (counter == $past(counter) - 1);
    endproperty
    assert_counter_valid: assert property(counter_no_underflow)
        else $error("[BOOT] ERROR: Counter underflow detected at time %0t", $time);

    // Assertion 4: State transitions are valid
    property valid_state_transitions;
        @(posedge clk) disable iff (!rst_n)
        (state == IDLE && start_in && delay_reg != 0) |-> ##1 (state == BUSY) ||
        (state == BUSY && counter > 1) |-> ##1 (state == BUSY) ||
        (state == BUSY && counter == 1) |-> ##1 (state == IDLE);
    endproperty
    assert_valid_transitions: assert property(valid_state_transitions)
        else $error("[BOOT] ERROR: Invalid state transition at time %0t", $time);

    // Assertion 5: start_out is only pulsed for one cycle
    property start_out_pulse;
        @(posedge clk) disable iff (!rst_n)
        start_out |-> ##1 !start_out;
    endproperty
    assert_start_out_pulse: assert property(start_out_pulse)
        else $error("[BOOT] ERROR: start_out not properly pulsed at time %0t", $time);

    // Assertion 6: start_out only occurs when transitioning from BUSY to IDLE
    property start_out_timing;
        @(posedge clk) disable iff (!rst_n)
        start_out |-> ($past(state) == BUSY && state == IDLE);
    endproperty
    assert_start_out_timing: assert property(start_out_timing)
        else $error("[BOOT] ERROR: start_out asserted at wrong time at time %0t", $time);

    // Assertion 7: Counter only decrements in BUSY state
    property counter_decrement_only_when_busy;
        @(posedge clk) disable iff (!rst_n)
        (state == IDLE) |-> (counter == 0);
    endproperty
    assert_counter_idle: assert property(counter_decrement_only_when_busy)
        else $error("[BOOT] ERROR: Counter non-zero in IDLE state at time %0t", $time);

    // Assertion 8: Register writes only affect delay_reg when addr == 0
    property register_write_protection;
        @(posedge clk) disable iff (!rst_n)
        (ext_sel && |ext_wen && ext_addr != 0) |-> ##1 (delay_reg == $past(delay_reg));
    endproperty
    assert_write_protection: assert property(register_write_protection)
        else $error("[BOOT] ERROR: Unexpected register write at addr %0d at time %0t", ext_addr, $time);

    // Coverage: Track important scenarios
    covergroup boot_coverage @(posedge clk);
        cp_state: coverpoint state {
            bins idle = {IDLE};
            bins busy = {BUSY};
        }
        cp_start_in: coverpoint start_in {
            bins start_asserted = {1};
            bins start_deasserted = {0};
        }
        cp_delay_values: coverpoint delay_reg {
            bins zero = {0};
            bins small = {[1:10]};
            bins medium = {[11:100]};
            bins large = {[101:$]};
        }
        // Cross coverage
        cx_state_start: cross cp_state, cp_start_in;
    endgroup
    
    boot_coverage cov_inst = new();

    // Functional coverage for sequences
    sequence start_to_complete;
        (state == IDLE && start_in && delay_reg != 0) ##1 
        (state == BUSY) [*1:$] ##1 
        (state == IDLE && start_out);
    endsequence
    
    cover_start_complete: cover property(@(posedge clk) disable iff (!rst_n) start_to_complete);

    // Monitor warnings (non-fatal)
    always @(posedge clk) begin
        if (rst_n) begin
            if (state == BUSY && start_in)
                $warning("[BOOT] WARNING: start_in ignored - module is busy at time %0t", $time);
            if (state == IDLE && start_in && delay_reg == 0)
                $warning("[BOOT] WARNING: start_in ignored - delay_reg is zero at time %0t", $time);
        end
    end
`endif

endmodule : boot