`define HAS_SSM



module tb;

    parameter ADDR_WIDTH = 16; // Address width in bits
    parameter DATA_WIDTH = 32; // Data width in bits
    parameter IO_ADDR_WIDTH = 16; // IO Address width in bits
    parameter IO_DATA_WIDTH = 256; // IO Data width in bits

  // stimuli
  logic clk;
  logic rst_n;

`ifdef HAS_DRRA
  logic [COLS-1:0] io_en_in_drra;
  logic [COLS-1:0][IO_ADDR_WIDTH-1:0] io_addr_in_drra;
  logic [COLS-1:0][IO_DATA_WIDTH-1:0] io_data_in_drra;
  logic [COLS-1:0] io_en_out_drra;
  logic [COLS-1:0][IO_ADDR_WIDTH-1:0] io_addr_out_drra;
  logic [COLS-1:0][IO_DATA_WIDTH-1:0] io_data_out_drra;
`endif

  logic io_en_in;
  logic [IO_ADDR_WIDTH-1:0] io_addr_in;
  logic [IO_DATA_WIDTH-1:0] io_data_in;
  logic io_en_out;
  logic [IO_ADDR_WIDTH-1:0] io_addr_out;
  logic [IO_DATA_WIDTH-1:0] io_data_out;

  logic [255:0] input_buffer[int];
  logic [255:0] output_buffer[int];

  // axi-lite signals
  logic [31:0] s_axi_awaddr;
  logic s_axi_awvalid;
  logic s_axi_awready;
  logic [31:0] s_axi_wdata;
  logic [3:0] s_axi_wstrb;
  logic s_axi_wvalid;
  logic s_axi_wready;
  logic [1:0] s_axi_bresp;
  logic s_axi_bvalid;
  logic s_axi_bready;
  logic [31:0] s_axi_araddr;
  logic s_axi_arvalid;
  logic s_axi_arready;
  logic [31:0] s_axi_rdata;
  logic [1:0] s_axi_rresp;
  logic s_axi_rvalid;
  logic s_axi_rready;

  // custom signals
  logic start_in;
  logic start_out;

  // DUT: alimp
  alimp alimp_inst (
      .clk            (clk),
      .rst_n          (rst_n),

      // AXI4-Lite Slave Interface
      .s_axi_awaddr   (s_axi_awaddr),
      .s_axi_awvalid  (s_axi_awvalid),
      .s_axi_awready  (s_axi_awready),
      .s_axi_wdata    (s_axi_wdata),
      .s_axi_wstrb    (s_axi_wstrb),
      .s_axi_wvalid   (s_axi_wvalid),
      .s_axi_wready   (s_axi_wready),
      .s_axi_bresp    (s_axi_bresp),
      .s_axi_bvalid   (s_axi_bvalid),
      .s_axi_bready   (s_axi_bready),
      .s_axi_araddr   (s_axi_araddr),
      .s_axi_arvalid  (s_axi_arvalid),
      .s_axi_arready  (s_axi_arready),
      .s_axi_rdata    (s_axi_rdata),
      .s_axi_rresp    (s_axi_rresp),
      .s_axi_rvalid   (s_axi_rvalid),
      .s_axi_rready   (s_axi_rready),

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

  // instruction memory interface
  localparam IRAM_SIZE = 23767; // 1GB
  logic [DATA_WIDTH-1:0] iram [0:IRAM_SIZE-1];
  logic [DATA_WIDTH-1:0] status;
  logic [DATA_WIDTH-1:0] return_value;
  int check_count = 0;
  initial begin
      if (!$readmemb("firmware.mem", iram)) begin
          $display("Error: Failed to read memory file");
      end else begin
          $display("Memory file loaded successfully");
      end

      // initial clock and reset
      rst_n = 0;
      @(posedge clk);
      @(negedge clk) rst_n = 1;
      @(posedge clk);
      @(negedge clk) rst_n = 0;
      @(posedge clk);
      @(negedge clk) rst_n = 1;

      // write everything to alimp instance using axi-lite interface
      s_axi_awvalid = 0;
      s_axi_wvalid = 0;
      s_axi_bready = 1;
      s_axi_arvalid = 0;
      s_axi_rready = 1;
      start_in = 0;
      @(negedge clk);
      for (int i = 0; i < IRAM_SIZE; i++) begin
          // write address
          s_axi_awaddr = i * 4;
          s_axi_awvalid = 1;
          @(negedge clk);
          while (!s_axi_awready) begin
              @(negedge clk);
          end
          s_axi_awvalid = 0;
          // write data
          s_axi_wdata = iram[i];
          s_axi_wstrb = 4'b1111;
          s_axi_wvalid = 1;
          @(negedge clk);
          while (!s_axi_wready) begin
              @(negedge clk);
          end
          s_axi_wvalid = 0;
          // wait for write response
          @(negedge clk);
          while (!s_axi_bvalid) begin
              @(negedge clk);
          end
          @(negedge clk);
      end
      $display("Instruction memory loaded successfully");

      // assert start signal for one cycle
      @(negedge clk);
      start_in = 1;
      @(negedge clk);
      start_in = 0;

      // Wake up every 100 cycles and check address 0x00000_C000.
      // If the contents is 0x0000_0001, then the program is still running.
      // If the contents is 0x0000_0000, then the program has completed.
      // Then read address 0x0000_2000, this is the return register.
      // If the contents is 0x47545353, then the program has completed successfully
      //   (0x47545353 is ASCII for "GTSS" -- a magic number meaning "Great Success")
      // If the contents is anything else, then the program has failed.

      forever begin
          repeat (100) @(negedge clk);
          // read status
          s_axi_araddr = 'h0000_C000;
          s_axi_arvalid = 1;
          @(negedge clk);
          while (!s_axi_arready) begin
              @(negedge clk);
          end
          s_axi_arvalid = 0;
          @(negedge clk);
          while (!s_axi_rvalid) begin
              @(negedge clk);
          end
          status = s_axi_rdata;
          @(negedge clk);
          if (status == 32'h0000_0000) begin
              // read return value
              s_axi_araddr = 'h0000_2000;
              s_axi_arvalid = 1;
              @(negedge clk);
              while (!s_axi_arready) begin
                  @(negedge clk);
              end
              s_axi_arvalid = 0;
              @(negedge clk);
              while (!s_axi_rvalid) begin
                  @(negedge clk);
              end
              return_value = s_axi_rdata;
              @(negedge clk);
              if (return_value == 32'h47545353) begin
                  $display("Program completed successfully!");
              end else begin
                  $display("Program failed with return value: 0x%08X", return_value);
              end
              $finish;
          end else begin
              check_count = check_count + 1;
              $display("Check %0d: Program still running...", check_count);
          end
      end
      // wait a few cycles and finish
      repeat (10) @(negedge clk);
      $finish;

  end

  // clock
  initial begin
    clk = 0;
    forever begin
      #5 clk = ~clk;
    end
  end


  `ifdef HAS_DRRA
  // Write to IO
  always_ff @(posedge clk or negedge rst_n) begin
    if (~rst_n) begin
      output_buffer.delete();
    end else begin
      for (int i = 0; i < COLS; i++) begin
        if (io_en_out_drra[i]) begin
          output_buffer[io_addr_out[i]] = io_data_out_drra[i];
        end
      end
    end
  end

  // Read from IO
  always_comb begin
    for (int i = 0; i < COLS; i++) begin
      if (io_en_in_drra[i]) begin
        io_data_in_drra[i] = input_buffer[io_addr_in_drra[i]];
      end else begin
        io_data_in_drra[i] = 0;
      end
    end
  end
  `endif

    // Write to IO
  always_ff @(posedge clk or negedge rst_n) begin
    if (~rst_n) begin
      output_buffer.delete();
    end else begin
      if (io_en_out) begin
        output_buffer[io_addr_out] = io_data_out;
      end
    end
  end

  // Read from IO
  always_comb begin
    if (io_en_in) begin
      io_data_in = input_buffer[io_addr_in];
    end else begin
      io_data_in = 0;
    end
  end

endmodule
