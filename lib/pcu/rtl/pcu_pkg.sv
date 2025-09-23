package pcu_pkg;

    `define HAS_SSM

    parameter ADDR_WIDTH = 16; // Address width in bits
    parameter DATA_WIDTH = 32; // Data width in bits
    parameter IO_ADDR_WIDTH = 16; // IO Address width in bits
    parameter IO_DATA_WIDTH = 256; // IO Data width in bits

    parameter ALIMP_ADDR_BASE = 'h0000_0000; // Base address for the entire module
    parameter ALIMP_ADDR_END  = 'h0000_FFFF; // End address for the entire module

    // RAM parameters
    parameter RAM_SIZE = 1024; // Number of 32-bit words in RAM
    parameter RAM_EXT_ADDR_BEGIN = 'h0000_0000; // Base address for RAM in external address space
    parameter RAM_INT_ADDR_BEGIN = 'h0000_0000; // Base address for RAM in internal address space
    parameter RAM_EXT_ADDR_END = RAM_EXT_ADDR_BEGIN + (RAM_SIZE * (DATA_WIDTH/8)) - 1;
    parameter RAM_INT_ADDR_END = RAM_INT_ADDR_BEGIN + (RAM_SIZE * (DATA_WIDTH/8)) - 1;

    // BOOT parameters
    parameter BOOT_EXT_ADDR_BEGIN = 'h0000_C000; // Base address for BOOT in external address space
    parameter BOOT_EXT_ADDR_END = 'h0000_C007;   // End address for BOOT in external address space

    // CORE parameters
    parameter CORE_EXT_ADDR_BEGIN = 'h0000_D000; // Base address for CORE in external address space
    parameter CORE_EXT_ADDR_END = 'h0000_D003;   // End  address for CORE in external address space

    `ifdef HAS_SSM
    // SSM parameters
    parameter SSM_BUFFER_SIZE = 64; // Number of 256-bit words in SSM buffer
    parameter SSM_INT_ADDR_BEGIN = 'h0000_C000; // Base address for SSM in internal address space
    parameter SSM_INT_ADDR_END = 'h0000_CFFF;   // End  address for SSM in internal address space
    `endif

    `ifdef HAS_VPI
    // VPI parameters
    parameter VPI_INT_ADDR_BEGIN = 'h0000_D000; // Base address for VPI in internal address space
    parameter VPI_INT_ADDR_END = 'h0000_DFFF;   // End  address for VPI in internal address space
    `endif
endpackage : pcu_pkg