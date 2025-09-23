module alimp
import alimp_pkg::*;
(
    // Global signals
    input logic clk,
    input logic rst_n,

    // AXI4-Lite Slave Interface
    input logic [31:0] s_axi_awaddr,
    input logic s_axi_awvalid,
    output logic s_axi_awready,
    input logic [31:0] s_axi_wdata,
    input logic [3:0] s_axi_wstrb,
    input logic s_axi_wvalid,
    output logic s_axi_wready,
    output logic [1:0] s_axi_bresp,
    output logic s_axi_bvalid,
    input logic s_axi_bready,
    input logic [31:0] s_axi_araddr,
    input logic s_axi_arvalid,
    output logic s_axi_arready,
    output logic [31:0] s_axi_rdata,
    output logic [1:0] s_axi_rresp,
    output logic s_axi_rvalid,
    input logic s_axi_rready,

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

    // Instantiate PCU
    pcu pcu_inst (
        .clk            (clk),
        .rst_n          (rst_n),

        // AXI4-Lite Slave Interface
        .AWADDR         (s_axi_awaddr),
        .AWVALID        (s_axi_awvalid),
        .AWREADY        (s_axi_awready),
        .WDATA          (s_axi_wdata),
        .WSTRB          (s_axi_wstrb),
        .WVALID         (s_axi_wvalid),
        .WREADY         (s_axi_wready),
        .BRESP          (s_axi_bresp),
        .BVALID         (s_axi_bvalid),
        .BREADY         (s_axi_bready),
        .ARADDR         (s_axi_araddr),
        .ARVALID        (s_axi_arvalid),
        .ARREADY        (s_axi_arready),
        .RDATA          (s_axi_rdata),
        .RRESP          (s_axi_rresp),
        .RVALID         (s_axi_rvalid),
        .RREADY         (s_axi_rready),

        // Custom signals
        .start_in       (start_in),
        .start_out      (start_out),

        // IO signals
        .io_en_in       (io_en_in),
        .io_addr_in     (io_addr_in),
        .io_data_in     (io_data_in),
        .io_en_out      (io_en_out),
        .io_addr_out    (io_addr_out),
        .io_data_out    (io_data_out)
    );


endmodule : alimp