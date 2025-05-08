module sram_tb (
);
    localparam WIDTH = 256;
    localparam DEPTH = 64;
    
    logic clk;
    logic enable_a_n;
    logic enable_b_n;
    logic write_enable_a_n;
    logic write_enable_b_n;
    logic [WIDTH-1:0] data_a;
    logic [WIDTH-1:0] data_b;
    logic [$clog2(DEPTH)-1:0] address_a;
    logic [$clog2(DEPTH)-1:0] address_b;
    
    logic [WIDTH-1:0] q_a;
    logic [WIDTH-1:0] q_b;


    sram #(        
        .DEPTH(DEPTH),
        .WIDTH(WIDTH)) 
        
        sram_inst (
        .clk(clk),
        .enable_a_n(enable_a_n),
        .enable_b_n(enable_b_n),
        .write_enable_a_n(write_enable_a_n),
        .write_enable_b_n(write_enable_b_n),
        .address_a(address_a),
        .address_b(address_b),
        .data_a(data_a),
        .data_b(data_b),
        .q_a(q_a),
        .q_b(q_b)    
        );

     

endmodule