module pcu
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

    // Custom signals
    input logic start_in,
    output logic start_out,

    // IO signals
    output logic io_en_in,
    output logic [IO_ADDR_WIDTH-1:0] io_addr_in,
    input logic [IO_DATA_WIDTH-1:0] io_data_in,
    output logic io_en_out,
    output logic [IO_ADDR_WIDTH-1:0] io_addr_out,
    output logic [IO_DATA_WIDTH-1:0] io_data_out
);

    // External interface
    logic ext_sel;                      // Chip select
    logic [ADDR_WIDTH-1:0] ext_addr;    // Local address
    logic [DATA_WIDTH-1:0] ext_wdata;   // Write data
    logic [DATA_WIDTH/8-1:0] ext_wen;   // Write enable per byte
    logic [DATA_WIDTH-1:0] ext_rdata;  // Read data


    // Internal signals
    logic int_valid;                  // Memory request valid
    logic [ADDR_WIDTH-1:0] int_addr;  // Local address
    logic [DATA_WIDTH-1:0] int_wdata; // Write data
    logic [DATA_WIDTH/8-1:0] int_wen; // Write enable per byte
    logic [DATA_WIDTH-1:0] int_rdata;  // Read data

    logic ext_ram_sel;
    logic ext_boot_sel;



    logic int_ram_sel;
    assign int_ram_sel = int_valid && (int_addr >= RAM_INT_ADDR_BEGIN) && (int_addr <= RAM_INT_ADDR_END);

    // Instantiate core
    core core_inst (
        .clk(clk),
        .rst_n(rst_n),
        .int_valid(int_valid),
        .int_addr(int_addr),
        .int_wdata(int_wdata),
        .int_wen(int_wen),
        .int_rdata(int_rdata),
        .start(start_in)
    );

    // Instantiate RAM
    ram ram_inst (
        .clk(clk),
        .rst_n(rst_n),
        .ext_sel(ext_ram_sel),
        .ext_addr(ext_addr - RAM_EXT_ADDR_BEGIN),
        .ext_wdata(ext_wdata),
        .ext_wen(ext_wen),
        .ext_rdata(ext_rdata),
        .int_sel(int_ram_sel),
        .int_addr(int_addr - RAM_INT_ADDR_BEGIN),
        .int_wdata(int_wdata),
        .int_wen(int_wen),
        .int_rdata(int_rdata)
    );

    // Instantiate Boot module
    boot boot_inst (
        .clk(clk),
        .rst_n(rst_n),
        .ext_sel(ext_boot_sel),
        .ext_addr(ext_addr - BOOT_EXT_ADDR_BEGIN), // Boot module address offset
        .ext_wdata(ext_wdata),
        .ext_wen(ext_wen),
        .ext_rdata(ext_rdata),
        .start_in(start_in),
        .start_out(start_out)
    );

    // Instantiate external interface
    extif extif_inst (
        .clk(clk),
        .rst_n(rst_n),
        .AWADDR(AWADDR),
        .AWVALID(AWVALID),
        .AWREADY(AWREADY),
        .WDATA(WDATA),
        .WSTRB(WSTRB),
        .WVALID(WVALID),
        .WREADY(WREADY),
        .BRESP(BRESP),
        .BVALID(BVALID),
        .BREADY(BREADY),
        .ARADDR(ARADDR),
        .ARVALID(ARVALID),
        .ARREADY(ARREADY),
        .RDATA(RDATA),
        .RRESP(RRESP),
        .RVALID(RVALID),
        .RREADY(RREADY),
        .ext_ram_sel(ext_ram_sel),
        .ext_boot_sel(ext_boot_sel),
        .ext_addr(ext_addr),
        .ext_wdata(ext_wdata),
        .ext_wen(ext_wen),
        .ext_rdata(ext_rdata)
    );

endmodule
