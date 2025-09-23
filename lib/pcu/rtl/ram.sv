module ram
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

    // Internal interface
    input logic int_sel,                    // Chip select
    input logic [ADDR_WIDTH-1:0] int_addr,  // Local address
    input logic [DATA_WIDTH-1:0] int_wdata, // Write data
    input logic [DATA_WIDTH/8-1:0] int_wen, // Write enable per byte
    output logic [DATA_WIDTH-1:0] int_rdata // Read data
);

    logic [DATA_WIDTH-1:0] mem [0:RAM_SIZE-1];

    // Dual-port RAM with external interface priority
    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            ext_rdata <= '0;
            int_rdata <= '0;
        end else begin
            // External interface has priority for writes
            if (ext_sel && |ext_wen) begin
                // External write
                for (int i = 0; i < DATA_WIDTH/8; i++) begin
                    if (ext_wen[i]) begin
                        mem[ext_addr][i*8 +: 8] <= ext_wdata[i*8 +: 8];
                    end
                end
            end else if (int_sel && |int_wen) begin
                // Internal write (only if external not writing)
                for (int i = 0; i < DATA_WIDTH/8; i++) begin
                    if (int_wen[i]) begin
                        mem[int_addr][i*8 +: 8] <= int_wdata[i*8 +: 8];
                    end
                end
            end
            
            // Independent reads (can happen simultaneously)
            if (ext_sel) begin
                ext_rdata <= mem[ext_addr];
            end
            
            if (int_sel) begin
                int_rdata <= mem[int_addr];
            end
        end
    end

// SystemVerilog Assertions for Error Detection
`ifdef ASSERT_ON
    // Assertion 1: Address bounds checking
    property ext_addr_in_bounds;
        @(posedge clk) disable iff (!rst_n)
        ext_sel |-> (ext_addr < RAM_SIZE);
    endproperty
    assert_ext_addr_bounds: assert property(ext_addr_in_bounds)
        else $error("[RAM] ERROR: External address %0d out of bounds (max: %0d) at time %0t", ext_addr, RAM_SIZE-1, $time);

    property int_addr_in_bounds;
        @(posedge clk) disable iff (!rst_n)
        int_sel |-> (int_addr < RAM_SIZE);
    endproperty
    assert_int_addr_bounds: assert property(int_addr_in_bounds)
        else $error("[RAM] ERROR: Internal address %0d out of bounds (max: %0d) at time %0t", int_addr, RAM_SIZE-1, $time);

    // Assertion 2: No simultaneous writes to same address
    property no_write_conflict;
        @(posedge clk) disable iff (!rst_n)
        (ext_sel && |ext_wen && int_sel && |int_wen) |-> (ext_addr != int_addr);
    endproperty
    assert_no_write_conflict: assert property(no_write_conflict)
        else $error("[RAM] ERROR: Simultaneous writes to same address %0d at time %0t", ext_addr, $time);

    // Assertion 3: Write enable validation
    property valid_write_enables;
        @(posedge clk) disable iff (!rst_n)
        (ext_sel && |ext_wen) |-> (ext_wen != 0) &&
        (int_sel && |int_wen) |-> (int_wen != 0);
    endproperty
    assert_valid_wen: assert property(valid_write_enables)
        else $error("[RAM] ERROR: Invalid write enable pattern at time %0t", $time);

    // Coverage for RAM access patterns
    covergroup ram_coverage @(posedge clk);
        cp_ext_access: coverpoint {ext_sel, |ext_wen} {
            bins ext_read = {2'b10};
            bins ext_write = {2'b11};
            bins ext_idle = {2'b00};
        }
        cp_int_access: coverpoint {int_sel, |int_wen} {
            bins int_read = {2'b10};
            bins int_write = {2'b11};
            bins int_idle = {2'b00};
        }
        cp_priority: coverpoint {ext_sel, int_sel} {
            bins ext_only = {2'b10};
            bins int_only = {2'b01};
            bins both_active = {2'b11};
            bins both_idle = {2'b00};
        }
    endgroup
    
    ram_coverage cov_inst = new();

    // Monitor: Display access patterns
    always @(posedge clk) begin
        if (rst_n) begin
            if (ext_sel && int_sel && |ext_wen && |int_wen && ext_addr == int_addr)
                $warning("[RAM] WARNING: Write conflict prevented by priority at addr %0d", ext_addr);
        end
    end
`endif

endmodule