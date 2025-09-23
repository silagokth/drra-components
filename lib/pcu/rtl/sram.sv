// dual-port SRAM module with byte-enable write capability  
// port A: write-only
// port B: read-only

module sram
#(
    parameter ADDR_WIDTH = 16,
    parameter DATA_WIDTH = 32,
    parameter DEPTH = 1 << ADDR_WIDTH
)
(
    input logic clk,
    input logic rst_n,

    input logic wsel,                    // Chip select
    input logic [ADDR_WIDTH-1:0] waddr,  // Local address
    input logic [DATA_WIDTH-1:0] wdata, // Write data
    input logic [DATA_WIDTH/8-1:0] wen, // Write enable per byte

    input logic rsel,                    // Chip select
    input logic [ADDR_WIDTH-1:0] raddr,  // Local address
    output logic [DATA_WIDTH-1:0] rdata // Read data
);

    // Memory array - initialize with default value for simulation
    logic [DATA_WIDTH-1:0] mem_array [0:DEPTH-1] = '{default: '0};

    // Write operation - no reset needed due to initialization above
    always_ff @(posedge clk) begin
        if (wsel && |wen && waddr < DEPTH) begin
            for (int i = 0; i < DATA_WIDTH/8; i++) begin
                if (wen[i]) begin
                    mem_array[waddr][i*8 +: 8] <= wdata[i*8 +: 8];
                end
            end
        end
    end

    // Read operation
    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            rdata <= '0;
        end else if (rsel && raddr < DEPTH) begin
            rdata <= mem_array[raddr];
        end else if (rsel) begin
            rdata <= '0; // Return zero for out-of-bounds
        end
    end

    // Assertions for validation
    `ifdef ASSERTIONS_ON
    
    raddrounds_check_a: assert property (@(posedge clk) 
        wsel |-> waddr < DEPTH) 
        else $error("SRAM: Write address out of bounds");
        
    raddrounds_check_b: assert property (@(posedge clk) 
        rsel |-> raddr < DEPTH) 
        else $error("SRAM: Read address out of bounds");
    
    collision_detect: assert property (@(posedge clk) 
        (wsel && rsel && |wen && (waddr == raddr)) |-> 
        ($warning("SRAM: Read/write collision"), 1'b1));
    
    initial begin
        assert (DATA_WIDTH % 8 == 0) else $fatal("DATA_WIDTH must be byte-aligned");
        assert (DEPTH == (1 << ADDR_WIDTH)) else $fatal("DEPTH/ADDR_WIDTH mismatch");
    end
    
    `endif

endmodule