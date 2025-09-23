package alimp_pkg;

    `define HAS_SSM

    parameter ADDR_WIDTH = 16; // Address width in bits
    parameter DATA_WIDTH = 32; // Data width in bits
    parameter IO_ADDR_WIDTH = 16; // IO Address width in bits
    parameter IO_DATA_WIDTH = 256; // IO Data width in bits

    parameter ALIMP_ADDR_BASE = 'h0000_0000; // Base address for the entire module
    parameter ALIMP_ADDR_END  = 'h0000_FFFF; // End address for the entire module
    
endpackage : alimp_pkg