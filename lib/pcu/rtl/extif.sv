module extif
import pcu_pkg::*;
(
    input logic clk,
    input logic rst_n,

    // AXI4-Lite Slave Interface
    // Write Address Channel
    input logic [ADDR_WIDTH-1:0] AWADDR,
    input logic AWVALID,
    output logic AWREADY,
    // Write Data Channel
    input logic [DATA_WIDTH-1:0] WDATA,
    input logic [DATA_WIDTH/8-1:0] WSTRB,
    input logic WVALID,
    output logic WREADY,
    // Write Response Channel
    output logic [1:0] BRESP,
    output logic BVALID,
    input logic BREADY,
    // Read Address Channel
    input logic [ADDR_WIDTH-1:0] ARADDR,
    input logic ARVALID,
    output logic ARREADY,
    // Read Data Channel
    output logic [DATA_WIDTH-1:0] RDATA,
    output logic [1:0] RRESP,
    output logic RVALID,
    input logic RREADY,
    
    // External interface
    output logic ext_ram_sel,                // Chip select
    output logic ext_boot_sel,               // Chip select
    output logic [ADDR_WIDTH-1:0] ext_addr,  // Local address
    output logic [DATA_WIDTH-1:0] ext_wdata, // Write data
    output logic [DATA_WIDTH/8-1:0] ext_wen, // Write enable per byte
    input logic [DATA_WIDTH-1:0] ext_rdata   // Read data

);
    logic [ADDR_WIDTH-1:0] ext_local_addr;
    assign ext_sel = ((ext_addr >= ALIMP_ADDR_BASE) && (ext_addr <= ALIMP_ADDR_END));
    assign ext_local_addr = ext_addr - ALIMP_ADDR_BASE;
    assign ext_ram_sel = ext_sel && (ext_local_addr >= RAM_EXT_ADDR_BEGIN) && (ext_addr <= RAM_EXT_ADDR_END);
    assign ext_boot_sel = ext_sel && (ext_local_addr >= BOOT_EXT_ADDR_BEGIN) && (ext_addr <= BOOT_EXT_ADDR_END);

    assign ext_addr = AWVALID ? AWADDR : ARADDR;
    assign ext_wdata = WDATA;
    assign ext_wen = WSTRB;

    // All peripherals are always ready
    assign AWREADY = !ext_sel; // Ready when not selected
    assign WREADY = !ext_sel;  // Ready when not selected
    assign BRESP = 2'b00;      // OKAY
    assign BVALID = ext_sel && AWVALID && WVALID; // Valid when selected and both AWVALID and WVALID are high
    assign ARREADY = !ext_sel; // Ready when not selected
    assign RDATA = ext_rdata;
    assign RRESP = 2'b00;      // OKAY
    assign RVALID = ext_sel && ARVALID; // Valid when selected and ARVALID

endmodule
    
    