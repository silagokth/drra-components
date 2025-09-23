module core
import pcu_pkg::*;
(
    input logic clk,
    input logic rst_n,

    // External interface as slave
    input logic ext_valid,                  // Memory request valid
    input logic [ADDR_WIDTH-1:0] ext_addr,  // Local address
    input logic [DATA_WIDTH-1:0] ext_wdata, // Write data
    input logic [DATA_WIDTH/8-1:0] ext_wen, // Write enable per byte
    output logic [DATA_WIDTH-1:0] ext_rdata,  // Read data

    // Internal interface as master
    output logic int_valid,                  // Memory request valid
    output logic [ADDR_WIDTH-1:0] int_addr,  // Local address
    output logic [DATA_WIDTH-1:0] int_wdata, // Write data
    output logic [DATA_WIDTH/8-1:0] int_wen, // Write enable per byte
    input logic [DATA_WIDTH-1:0] int_rdata,  // Read data

    // custom signals
    input logic start  

);

    logic trap;

    picorvs32 #(
        .ENABLE_COUNTERS     ('0     ),
		.ENABLE_COUNTERS64   ('0   ),
		.ENABLE_REGS_16_31   ('1   ),
		.ENABLE_REGS_DUALPORT('1),
		.TWO_STAGE_SHIFT     ('1     ),
		.BARREL_SHIFTER      ('1      ),
		.TWO_CYCLE_COMPARE   ('1   ),
		.TWO_CYCLE_ALU       ('1       ),
		.COMPRESSED_ISA      ('0      ),
		.CATCH_MISALIGN      ('0      ),
		.CATCH_ILLINSN       ('0       ),
		.ENABLE_PCPI         ('0         ),
		.ENABLE_MUL          ('0          ),
		.ENABLE_FAST_MUL     ('1    ),
		.ENABLE_DIV          ('0          ),
		.ENABLE_IRQ          ('0          ),
		.ENABLE_IRQ_QREGS    ('0    ),
		.ENABLE_IRQ_TIMER    ('0    ),
		.ENABLE_TRACE        ('0        ),
		.REGS_INIT_ZERO      ('0      ),
		.MASKED_IRQ          ('0          ),
		.LATCHED_IRQ         ('0         ),
		.PROGADDR_RESET      ('0      ),
		.PROGADDR_IRQ        ('0        ),
		.STACKADDR           ('h0000_0100           )
    ) picorv32_inst (
		.clk      (clk),
		.resetn   (rst_n),
        .trap     (trap),
		.mem_valid(mem_valid),
		.mem_addr (mem_addr),
		.mem_wdata(mem_wdata),
		.mem_wstrb(mem_wen),
		.mem_ready(mem_ready),
		.mem_rdata(mem_rdata),
		.irq      ('0)
    );

    // Memory ready signal is always one cycle after valid because our simple memory
    // interface has predictable timing (no wait states, no stalls). It responds in one cycle.
    logic mem_ready_d;
    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            mem_ready_d <= 1'b0;
        end else begin
            mem_ready_d <= mem_valid;
        end
    end
    assign mem_ready = mem_ready_d;

    // Define state machine for start signal handling
    enum logic {
        IDLE,
        BUSY
    } state;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state <= IDLE;
        end else begin
            case (state)
                IDLE: begin
                    state <= IDLE;
                    if (start) begin
                        state <= BUSY;
                    end
                end
                BUSY: begin
                    state <= BUSY;
                    if (trap) begin
                        state <= IDLE;
                    end
                end
            endcase
        end
    end

    // Via the external interface, status register can be read.
    // If the core is BUSY, bit 0 of the status register is 1.
    // If the core is IDLE, bit 0 of the status register is 0.
    always_comb begin
        ext_rdata = '0;
        if (ext_valid) begin
            if (ext_addr == 32'h0000_0000) begin
                ext_rdata = {{DATA_WIDTH-1{1'b0}}, (state == BUSY)}; // Status register
            end
        end
    end


endmodule



