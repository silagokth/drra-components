module ssm
import pcu_pkg::*;
(
    input logic clk,
    input logic rst_n,

    // Internal interface as slave
    input logic int_sel,                    // Chip select
    input logic [ADDR_WIDTH-1:0] int_addr,  // Local address
    input logic [DATA_WIDTH-1:0] int_wdata, // Write data
    input logic [DATA_WIDTH/8-1:0] int_wen, // Write enable per byte
    output logic [DATA_WIDTH-1:0] int_rdata, // Read data

    // Custom signals
    output logic io_wen,
    output logic io_ren,
    output logic  [IO_ADDR_WIDTH-1:0] io_raddr,
    input logic [IO_DATA_WIDTH-1:0] io_rdata,
    output logic [IO_ADDR_WIDTH-1:0] io_waddr,
    output logic [IO_DATA_WIDTH-1:0] io_wdata
);

    // Buffer parameters
    localparam BUFFER_ROW = SSM_BUFFER_SIZE; // 32 rows of 256-bit words
    localparam BUFFER_COL = IO_DATA_WIDTH / DATA_WIDTH; // 8 columns (banks) of 32-bit words (256 bits total)
    localparam BUFFER_SIZE = BUFFER_ROW * IO_DATA_WIDTH / 8; // Total buffer size in bytes
    localparam BUFFER_ADDR_BASE = 'b000; // Base address for buffer access
    localparam BUFFER_ADDR_END = BUFFER_ADDR_BASE + BUFFER_SIZE - 1; // End address for buffer access

    // Status register parameters
    localparam STATUS_REG_ADDR = BUFFER_ADDR_END + 1; // Base address for registers

    // Load AGU parameters
    localparam LDAGU_REG_ADDR_L = STATUS_REG_ADDR + 1;
    localparam LDAGU_REG_ADDR_H = STATUS_REG_ADDR + 2;

    // Store AGU parameters
    localparam STAGU_REG_ADDR_L = STATUS_REG_ADDR + 3;
    localparam STAGU_REG_ADDR_H = STATUS_REG_ADDR + 4;



    // Buffer
    // generate BUFFER_COL sram instances, each instance is named as buffer[i].
    logic buffer_wsel [BUFFER_COL-1:0];
    logic buffer_rsel [BUFFER_COL-1:0];
    logic [DATA_WIDTH-1:0] buffer_rdata [BUFFER_COL-1:0];
    logic [DATA_WIDTH-1:0] buffer_wdata [BUFFER_COL-1:0];
    logic [DATA_WIDTH/8-1:0] buffer_wen [BUFFER_COL-1:0];
    logic [ADDR_WIDTH-1-2-$clog2(BUFFER_ROW):0] buffer_raddr[BUFFER_COL-1:0];
    logic [ADDR_WIDTH-1-2-$clog2(BUFFER_ROW):0] buffer_waddr[BUFFER_COL-1:0];
    for (genvar i = 0; i < BUFFER_COL; i++) begin : buffer_gen
        sram #(
            .ADDR_WIDTH(ADDR_WIDTH-2-$clog2(BUFFER_ROW)),
            .DATA_WIDTH(DATA_WIDTH),
            .DEPTH(BUFFER_ROW)
        ) buffer_inst (
            .clk(clk),
            .rst_n(rst_n),
            .wsel(buffer_wsel[i]),
            .waddr(buffer_waddr[i]),
            .wdata(buffer_wdata[i]),
            .wen(buffer_wen[i]),
            .rsel(buffer_rsel[i]),
            .raddr(buffer_raddr[i]),
            .rdata(buffer_rdata[i])
        );
    end

    // -------------------------------------------------------------------------
    // visualization of buffer layout:
    //
    // -------------------------------------------------------
    // |       |  col0   |  col1   |  col2   | ... | col7    |
    // |=======|=========|=========|=========|=====|=========|
    // | row0  | 32-bit  | 32-bit  | 32-bit  | ... | 32-bit  |
    // |-------|---------|---------|---------|-----|---------|
    // | row1  | 32-bit  | 32-bit  | 32-bit  | ... | 32-bit  |
    // |-------|---------|---------|---------|-----|---------|
    // | ...   |   ...   |   ...   |   ...   | ... |   ...   |
    // |-------|---------|---------|---------|-----|---------|
    // | row31 | 32-bit  | 32-bit  | 32-bit  | ... | 32-bit  |
    // -------------------------------------------------------
    //
    // Address mapping:
    //   0x000 -- 0x003: row0, col0
    //   0x004 -- 0x007: row0, col1
    //   ...
    //   0x3F8 -- 0x3FB: row31, col6
    //   0x3FC -- 0x3FF: row31, col7
    //
    // -------------------------------------------------------------------------


    // AGU registers
    // AUG_REG_L: lower 32 bits of address
    // -----------------------------------------------------
    // | Field       | Bits    | Description               |
    // |-------------|---------|---------------------------|
    // | ADDR[31:0]  | [31:0]  | Lower 32 bits of address  |
    // -----------------------------------------------
    // AUG_REG_H: upper 32 bits of address
    // 


    // Status
    enum logic {
        IDLE,
        BUSY
    } ldagu_state, stagu_state;

    // The core can read status register at anytime
    always_comb begin
        int_rdata = '0;
        if (int_sel && !int_wen) begin
            if (int_addr == STATUS_REG_ADDR) begin
                int_rdata = { {(DATA_WIDTH-2){1'b0}}, (ldagu_state == BUSY), (stagu_state == BUSY) }; // Status register
            end
        end
    end

    // Write to control registers is possible only when the corresponding state is IDLE
    typedef struct packed {
        logic        en;         // Enable
        logic [24:0] io_addr;    // IO address (for load AGU, 26 bits for 64MB addressing)
        logic [4:0]  sram_addr;  // SRAM address (for store AGU, 16 bits for 64KB addressing)
        logic [4:0]  io_step;    // IO address step
        logic [4:0]  sram_step;  // SRAM address step
        logic [4:0]  iter;       // Number of iterations
        logic [DATA_WIDTH-1:0] reserved; // Reserved bits to fill up to 32 bits
    } agu_config_t;
    agu_config_t ldagu_config;
    agu_config_t stagu_config;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            ldagu_config <= '0;
            stagu_config <= '0;
        end else begin
            if (ldagu_state == IDLE) begin
                if (int_sel && |int_wen) begin
                    if (int_addr == LDAGU_REG_ADDR_L) begin
                        ldagu_config.en <= int_wdata[0];
                        ldagu_config.io_addr <= int_wdata[29:5];
                        ldagu_config.sram_addr <= int_wdata[4:1];
                    end else if (int_addr == LDAGU_REG_ADDR_H) begin
                        ldagu_config.io_step <= int_wdata[4:0];
                        ldagu_config.sram_step <= int_wdata[9:5];
                        ldagu_config.iter <= int_wdata[14:10];
                    end else begin
                        ldagu_config <= ldagu_config; // No change
                    end
                end
            end else begin
                ldagu_config.io_addr <= ldagu_config.io_addr + ldagu_config.io_step;
                ldagu_config.sram_addr <= ldagu_config.sram_addr + ldagu_config.sram_step;
                if (ldagu_config.iter != 0) begin
                    ldagu_config.iter <= ldagu_config.iter - 1;
                end else begin
                    ldagu_config <= '0; // Clear config when done
                end
            end

            if (stagu_state == IDLE) begin
                if (int_sel && |int_wen) begin
                    if (int_addr == STAGU_REG_ADDR_L) begin
                        stagu_config.en <= int_wdata[0];
                        stagu_config.io_addr <= int_wdata[20:5];
                        stagu_config.sram_addr <= int_wdata[4:1];
                    end else if (int_addr == STAGU_REG_ADDR_H) begin
                        stagu_config.io_step <= int_wdata[4:0];
                        stagu_config.sram_step <= int_wdata[9:5];
                        stagu_config.iter <= int_wdata[14:10];
                    end else begin
                        stagu_config <= stagu_config; // No change
                    end
                end
            end else begin
                stagu_config.io_addr <= stagu_config.io_addr + stagu_config.io_step;
                stagu_config.sram_addr <= stagu_config.sram_addr + stagu_config.sram_step;
                if (stagu_config.iter != 0) begin
                    stagu_config.iter <= stagu_config.iter - 1;
                end else begin
                    stagu_config <= '0; // Clear config when done
                end
            end
        end
    end
                    

    // Address generation FSM for load and store AGUs
    // Each AGU has its own state machine and registers
    logic [IO_DATA_WIDTH-1:0] io_wdata_copy, io_rdata_copy;
    for(genvar i = 0; i < BUFFER_COL; i++) begin : io_data_copy_gen
        assign io_wdata_copy[ (i+1)*DATA_WIDTH-1 : i*DATA_WIDTH ] = buffer_rdata[i];
        assign buffer_rdata[i] = io_rdata_copy[ (i+1)*DATA_WIDTH-1 : i*DATA_WIDTH ];
    end



    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            ldagu_state <= IDLE;
            stagu_state <= IDLE;
            io_ren <= 1'b0;
            io_wen <= 1'b0;
            io_raddr <= '0;
            io_waddr <= '0;
            io_wdata <= '0;
            io_rdata_copy <= '0;
            for (int i = 0; i < BUFFER_COL; i++) begin
                buffer_wsel[i] <= 1'b0;
                buffer_rsel[i] <= 1'b0;
                buffer_waddr[i] <= '0;
                buffer_wdata[i] <= '0;
                buffer_raddr[i] <= '0;
                buffer_wen[i] <= '0;
            end
        end else begin

            // Load AGU state machine
            case (ldagu_state)
                IDLE: begin
                    io_rdata_copy <= '0;
                    if (ldagu_config.en) begin
                        ldagu_state <= BUSY; // Start load operation
                    end else begin
                        ldagu_state <= IDLE;
                    end
                    io_ren <= 1'b0;
                    io_raddr <= '0;
                    
                    if (int_sel) begin
                        if (|int_wen && int_addr >= BUFFER_ADDR_BASE && int_addr < BUFFER_ADDR_END) begin
                            // Writing to buffer
                            logic [ADDR_WIDTH-1:0] local_addr;
                            local_addr = int_addr - BUFFER_ADDR_BASE;
                            for (int i = 0; i < BUFFER_COL; i++) begin
                                if (i == local_addr[ADDR_WIDTH-1:ADDR_WIDTH-4]) begin
                                    buffer_wsel[i] <= 1'b1;
                                    buffer_waddr[i] <= local_addr[ADDR_WIDTH-5-:$clog2(BUFFER_ROW)];
                                    buffer_wdata[i] <= int_wdata;
                                    buffer_wen[i] <= int_wen;
                                end else begin
                                    buffer_wsel[i] <= 1'b0;
                                    buffer_waddr[i] <= '0;
                                    buffer_wdata[i] <= '0;
                                    buffer_wen[i] <= '0;
                                end
                            end
                        end else begin
                            // Not writing to buffer
                            for (int i = 0; i < BUFFER_COL; i++) begin
                                buffer_wsel[i] <= 1'b0;
                                buffer_waddr[i] <= '0;
                                buffer_wdata[i] <= '0;
                                buffer_wen[i] <= '0;
                            end
                        end
                    end else begin
                        // Not selected
                        for (int i = 0; i < BUFFER_COL; i++) begin
                            buffer_wsel[i] <= 1'b0;
                            buffer_waddr[i] <= '0;
                            buffer_wdata[i] <= '0;
                            buffer_wen[i] <= '0;
                            buffer_rsel[i] <= 1'b0;
                            buffer_raddr[i] <= '0;
                        end
                    end
                end
                BUSY: begin
                    if (ldagu_config.iter == 0) begin
                        ldagu_state <= IDLE;
                        io_ren <= 1'b0;
                        io_raddr <= '0;
                        io_rdata_copy <= '0;
                    end else begin
                        // Continue reading data
                        io_ren <= 1'b1;
                        io_raddr <= ldagu_config.io_addr[IO_ADDR_WIDTH-1:0];
                        // Write data to buffer
                        for (int i = 0; i < BUFFER_COL; i++) begin
                            buffer_wsel[i] <= 1'b1;
                            buffer_waddr[i] <= ldagu_config.sram_addr;
                            buffer_wen[i] <= {DATA_WIDTH/8{1'b1}}; // Write all bytes
                        end
                        io_rdata_copy <= io_rdata;
                    end
                end
            endcase

            // Store AGU state machine
            case (stagu_state)
                IDLE: begin
                    if (stagu_config.en) begin
                        stagu_state <= BUSY; // Start store operation
                    end else begin
                        stagu_state <= IDLE;
                    end
                    io_wen <= 1'b0;
                    io_waddr <= '0;
                    io_wdata <= '0;

                    if (int_sel) begin
                        if (|int_wen && int_addr >= BUFFER_ADDR_BASE && int_addr < BUFFER_ADDR_END) begin
                            // Writing to buffer
                            logic [ADDR_WIDTH-1:0] local_addr;
                            local_addr = int_addr - BUFFER_ADDR_BASE;
                            for (int i = 0; i < BUFFER_COL; i++) begin
                                if (i == local_addr[ADDR_WIDTH-1:ADDR_WIDTH-4]) begin
                                    buffer_wsel[i] <= 1'b1;
                                    buffer_waddr[i] <= local_addr[ADDR_WIDTH-5-:$clog2(BUFFER_ROW)];
                                    buffer_wdata[i] <= int_wdata;
                                    buffer_wen[i] <= int_wen;
                                end else begin
                                    buffer_wsel[i] <= 1'b0;
                                    buffer_waddr[i] <= '0;
                                    buffer_wdata[i] <= '0;
                                    buffer_wen[i] <= '0;
                                end
                            end
                        end else begin
                            // Not writing to buffer
                            for (int i = 0; i < BUFFER_COL; i++) begin
                                buffer_wsel[i] <= 1'b0;
                                buffer_waddr[i] <= '0;
                                buffer_wdata[i] <= '0;
                                buffer_wen[i] <= '0;
                            end
                        end
                    end else begin
                        // Not selected
                        for (int i = 0; i < BUFFER_COL; i++) begin
                            buffer_wsel[i] <= 1'b0;
                            buffer_waddr[i] <= '0;
                            buffer_wdata[i] <= '0;
                            buffer_wen[i] <= '0;
                            buffer_rsel[i] <= 1'b0;
                            buffer_raddr[i] <= '0;
                        end
                    end
                end
                BUSY: begin
                    if (stagu_config.iter == 0) begin
                        stagu_state <= IDLE;
                        io_wen <= 1'b0;
                        io_waddr <= '0;
                        io_wdata <= '0;
                        for (int i = 0; i < BUFFER_COL; i++) begin
                            buffer_rsel[i] <= 1'b0;
                            buffer_raddr[i] <= '0;
                        end
                    end else begin
                        // Continue writing data
                        io_wen <= 1'b1;
                        io_waddr <= stagu_config.io_addr[IO_ADDR_WIDTH-1:0];
                        // Read data from buffer
                        for (int i = 0; i < BUFFER_COL; i++) begin
                            buffer_rsel[i] <= 1'b1;
                            buffer_raddr[i] <= stagu_config.sram_addr;
                        end
                        io_wdata <= io_wdata_copy;
                    end
                end
            endcase
        end
    end
endmodule





