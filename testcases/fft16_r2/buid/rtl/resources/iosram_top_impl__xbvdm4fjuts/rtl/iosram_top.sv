`define iosram_top_impl _xbvdm4fjuts
`define iosram_top_impl_pkg _xbvdm4fjuts_pkg

package _xbvdm4fjuts_pkg;
    parameter BULK_BITWIDTH = 256;
    parameter FSM_PER_SLOT = 4;
    parameter INSTR_OPCODE_BITWIDTH = 3;
    parameter IO_ADDR_WIDTH = 16;
    parameter IO_DATA_WIDTH = 256;
    parameter NUM_SLOTS = 16;
    parameter RESOURCE_INSTR_WIDTH = 27;
    parameter SRAM_ADDR_WIDTH = 6;
    parameter WORD_BITWIDTH = 32;

    typedef struct packed {
        logic _init_addr_sd;
        logic [15:0] _init_addr;
        logic [1:0] _port;
    } dsu_t;

    function static dsu_t unpack_dsu;
        input logic [23:0] instr;
        dsu_t _dsu;
        _dsu._init_addr_sd = instr[23];
        _dsu._init_addr  = instr[22:7];
        _dsu._port  = instr[6:5];
        return _dsu;
    endfunction

    function static logic [23:0] pack_dsu;
        input dsu_t _dsu;
        logic [23:0] instr;

        instr[23] = _dsu._init_addr_sd;
        instr[22:7] = _dsu._init_addr;
        instr[6:5] = _dsu._port;
        return instr;
    endfunction
    typedef struct packed {
        logic [1:0] _port;
        logic [3:0] _level;
        logic [5:0] _iter;
        logic [5:0] _step;
        logic [5:0] _delay;
    } rep_t;

    function static rep_t unpack_rep;
        input logic [23:0] instr;
        rep_t _rep;
        _rep._port  = instr[23:22];
        _rep._level  = instr[21:18];
        _rep._iter  = instr[17:12];
        _rep._step  = instr[11:6];
        _rep._delay  = instr[5:0];
        return _rep;
    endfunction

    function static logic [23:0] pack_rep;
        input rep_t _rep;
        logic [23:0] instr;

        instr[23:22] = _rep._port;
        instr[21:18] = _rep._level;
        instr[17:12] = _rep._iter;
        instr[11:6] = _rep._step;
        instr[5:0] = _rep._delay;
        return instr;
    endfunction
    typedef struct packed {
        logic [1:0] _port;
        logic [3:0] _level;
        logic [5:0] _iter;
        logic [5:0] _step;
        logic [5:0] _delay;
    } repx_t;

    function static repx_t unpack_repx;
        input logic [23:0] instr;
        repx_t _repx;
        _repx._port  = instr[23:22];
        _repx._level  = instr[21:18];
        _repx._iter  = instr[17:12];
        _repx._step  = instr[11:6];
        _repx._delay  = instr[5:0];
        return _repx;
    endfunction

    function static logic [23:0] pack_repx;
        input repx_t _repx;
        logic [23:0] instr;

        instr[23:22] = _repx._port;
        instr[21:18] = _repx._level;
        instr[17:12] = _repx._iter;
        instr[11:6] = _repx._step;
        instr[5:0] = _repx._delay;
        return instr;
    endfunction

    parameter DEPTH = 64;
    parameter WIDTH = 256;

endpackage

module _xbvdm4fjuts
import _xbvdm4fjuts_pkg::*;
(
    input  logic clk_0,
    input  logic rst_n_0,
    input  logic instr_en_0,
    input  logic [RESOURCE_INSTR_WIDTH-1:0] instr_0,
    input  logic [3:0] activate_0,
    input  logic [WORD_BITWIDTH-1:0] word_data_in_0,
    output logic [WORD_BITWIDTH-1:0] word_data_out_0,
    input  logic [BULK_BITWIDTH-1:0] bulk_data_in_0,
    output logic [BULK_BITWIDTH-1:0] bulk_data_out_0,
    input  logic clk_1,
    input  logic rst_n_1,
    input  logic instr_en_1,
    input  logic [RESOURCE_INSTR_WIDTH-1:0] instr_1,
    input  logic [3:0] activate_1,
    input  logic [WORD_BITWIDTH-1:0] word_data_in_1,
    output logic [WORD_BITWIDTH-1:0] word_data_out_1,
    input  logic [BULK_BITWIDTH-1:0] bulk_data_in_1,
    output logic [BULK_BITWIDTH-1:0] bulk_data_out_1,
    input  logic clk_2,
    input  logic rst_n_2,
    input  logic instr_en_2,
    input  logic [RESOURCE_INSTR_WIDTH-1:0] instr_2,
    input  logic [3:0] activate_2,
    input  logic [WORD_BITWIDTH-1:0] word_data_in_2,
    output logic [WORD_BITWIDTH-1:0] word_data_out_2,
    input  logic [BULK_BITWIDTH-1:0] bulk_data_in_2,
    output logic [BULK_BITWIDTH-1:0] bulk_data_out_2,
    input  logic clk_3,
    input  logic rst_n_3,
    input  logic instr_en_3,
    input  logic [RESOURCE_INSTR_WIDTH-1:0] instr_3,
    input  logic [3:0] activate_3,
    input  logic [WORD_BITWIDTH-1:0] word_data_in_3,
    output logic [WORD_BITWIDTH-1:0] word_data_out_3,
    input  logic [BULK_BITWIDTH-1:0] bulk_data_in_3,
    output logic [BULK_BITWIDTH-1:0] bulk_data_out_3,
    output logic io_en_in,
    output logic [IO_ADDR_WIDTH-1:0] io_addr_in,
    input  logic [IO_DATA_WIDTH-1:0] io_data_in
);

    // Useless outputs:
    assign word_data_out_0 = 0;
    assign word_data_out_1 = 0;
    assign word_data_out_2 = 0;
    assign word_data_out_3 = 0;
    assign bulk_data_out_0 = 0;
    assign bulk_data_out_2 = 0;
    assign bulk_data_out_3 = 0;

    logic clk, rst_n;
    assign clk = clk_0;
    assign rst_n = rst_n_0;

    logic [INSTR_OPCODE_BITWIDTH-1:0] opcode_0_0, opcode_0_2, opcode_1_3;
    logic [RESOURCE_INSTR_WIDTH-INSTR_OPCODE_BITWIDTH-1:0] payload_0_0, payload_0_2, payload_1_3;

    assign opcode_0_0 = instr_0[RESOURCE_INSTR_WIDTH-1:RESOURCE_INSTR_WIDTH-INSTR_OPCODE_BITWIDTH];
    assign payload_0_0 = instr_0[RESOURCE_INSTR_WIDTH-INSTR_OPCODE_BITWIDTH-1:0];
    assign opcode_0_2 = instr_0[RESOURCE_INSTR_WIDTH-1:RESOURCE_INSTR_WIDTH-INSTR_OPCODE_BITWIDTH];
    assign payload_0_2 = instr_0[RESOURCE_INSTR_WIDTH-INSTR_OPCODE_BITWIDTH-1:0];
    assign opcode_1_3 = instr_1[RESOURCE_INSTR_WIDTH-1:RESOURCE_INSTR_WIDTH-INSTR_OPCODE_BITWIDTH];
    assign payload_1_3 = instr_1[RESOURCE_INSTR_WIDTH-INSTR_OPCODE_BITWIDTH-1:0];
    
    dsu_t dsu_0_0, dsu_0_2, dsu_1_3;
    rep_t rep_0_0, rep_0_2, rep_1_3;
    repx_t repx_0_0, repx_0_2, repx_1_3;

    logic [IO_ADDR_WIDTH-1:0] addr_0_0;
    logic [SRAM_ADDR_WIDTH-1:0] addr_0_2, addr_1_3;
    logic dsu_valid_0_0, dsu_valid_0_2, dsu_valid_1_3;
    logic rep_valid_0_0, rep_valid_0_2, rep_valid_1_3;
    logic repx_valid_0_0, repx_valid_0_2, repx_valid_1_3;
    logic port_enable_0_0, port_enable_0_2, port_enable_1_3;
    logic [IO_ADDR_WIDTH-1:0] port_addr_0_0;
    logic [SRAM_ADDR_WIDTH-1:0] port_addr_0_2, port_addr_1_3;

    assign dsu_valid_0_0 = instr_en_0 && (opcode_0_0 == 6);
    assign rep_valid_0_0 = instr_en_0 && (opcode_0_0 == 0);
    assign repx_valid_0_0 = instr_en_0 && (opcode_0_0 == 1);
    assign dsu_0_0 = dsu_valid_0_0 ? unpack_dsu(payload_0_0) : '{default: 0};
    assign rep_0_0 = rep_valid_0_0 ? unpack_rep(payload_0_0) : '{default: 0};
    assign repx_0_0 = repx_valid_0_0 ? unpack_repx(payload_0_0) : '{default: 0};
    assign dsu_valid_0_2 = instr_en_0 && (opcode_0_2 == 6);
    assign rep_valid_0_2 = instr_en_0 && (opcode_0_2 == 0);
    assign repx_valid_0_2 = instr_en_0 && (opcode_0_2 == 1);
    assign dsu_0_2 = dsu_valid_0_2 ? unpack_dsu(payload_0_2) : '{default: 0};
    assign rep_0_2 = rep_valid_0_2 ? unpack_rep(payload_0_2) : '{default: 0};
    assign repx_0_2 = repx_valid_0_2 ? unpack_repx(payload_0_2) : '{default: 0};
    assign dsu_valid_1_3 = instr_en_1 && (opcode_1_3 == 6);
    assign rep_valid_1_3 = instr_en_1 && (opcode_1_3 == 0);
    assign repx_valid_1_3 = instr_en_1 && (opcode_1_3 == 1);
    assign dsu_1_3 = dsu_valid_1_3 ? unpack_dsu(payload_1_3) : '{default: 0};
    assign rep_1_3 = rep_valid_1_3 ? unpack_rep(payload_1_3) : '{default: 0};
    assign repx_1_3 = repx_valid_1_3 ? unpack_repx(payload_1_3) : '{default: 0};

    logic [IO_ADDR_WIDTH-1:0] step_0_0;
    logic [IO_ADDR_WIDTH-1:0] delay_0_0;
    logic [IO_ADDR_WIDTH-1:0] iter_0_0;
    logic [IO_ADDR_WIDTH-1:0] init_addr_0_0;

    logic [SRAM_ADDR_WIDTH-1:0] step_0_2, step_1_3;
    logic [SRAM_ADDR_WIDTH-1:0] delay_0_2, delay_1_3;
    logic [SRAM_ADDR_WIDTH-1:0] iter_0_2, iter_1_3;
    logic [SRAM_ADDR_WIDTH-1:0] init_addr_0_2, init_addr_1_3;

    assign step_0_0 = {10'b0, rep_0_0._step};
    assign delay_0_0 = {10'b0, rep_0_0._delay};
    assign iter_0_0 = {10'b0, rep_0_0._iter};
    assign init_addr_0_0 = dsu_0_0._init_addr;
    assign step_0_2 = rep_0_2._step[SRAM_ADDR_WIDTH-1:0];
    assign delay_0_2 = rep_0_2._delay[SRAM_ADDR_WIDTH-1:0];
    assign iter_0_2 = rep_0_2._iter[SRAM_ADDR_WIDTH-1:0];
    assign init_addr_0_2 = dsu_0_2._init_addr[SRAM_ADDR_WIDTH-1:0];
    assign step_1_3 = rep_1_3._step[SRAM_ADDR_WIDTH-1:0];
    assign delay_1_3 = rep_1_3._delay[SRAM_ADDR_WIDTH-1:0];
    assign iter_1_3 = rep_1_3._iter[SRAM_ADDR_WIDTH-1:0];
    assign init_addr_1_3 = dsu_1_3._init_addr[SRAM_ADDR_WIDTH-1:0];
    
    agu #(
      .ADDRESS_WIDTH(IO_ADDR_WIDTH),
      .NUMBER_OF_LEVELS(4)
    ) aug_0_0 (
      .clk(clk),
      .rst_n(rst_n),
      .activate(activate_0[0]),
      .load_initial(dsu_valid_0_0 & dsu_0_0._port == 0),
      .load_level(rep_valid_0_0 & rep_0_0._port == 0),
      .is_extended(),  // TODO: not supported yet
      .level_to_load(rep_0_0._level[1:0]),
      .step(step_0_0),
      .delay(delay_0_0),
      .iterations(iter_0_0),
      .initial_address(init_addr_0_0),
      .address_valid(port_enable_0_0),
      .address(port_addr_0_0)
  );

  agu #(
      .ADDRESS_WIDTH(SRAM_ADDR_WIDTH),
      .NUMBER_OF_LEVELS(4)
  ) agu_0_2 (
      .clk(clk),
      .rst_n(rst_n),
      .activate(activate_0[2]),
      .load_initial(dsu_valid_0_2 & dsu_0_2._port == 2),
      .load_level(rep_valid_0_2 & rep_0_2._port == 2),
      .is_extended(),  // TODO: not supported yet
      .level_to_load(rep_0_2._level[1:0]),
      .step(step_0_2),
      .delay(delay_0_2),
      .iterations(iter_0_2),
      .initial_address(init_addr_0_2),
      .address_valid(port_enable_0_2),
      .address(port_addr_0_2)
  );

  agu #(
      .ADDRESS_WIDTH(SRAM_ADDR_WIDTH),
      .NUMBER_OF_LEVELS(4)
  ) agu_1_3 (
      .clk(clk),
      .rst_n(rst_n),
      .activate(activate_1[3]),
      .load_initial(dsu_valid_1_3 & dsu_1_3._port == 3),
      .load_level(rep_valid_1_3 & rep_1_3._port == 3),
      .is_extended(),  // TODO: not supported yet
      .level_to_load(rep_1_3._level[1:0]),
      .step(step_1_3),
      .delay(delay_1_3),
      .iterations(iter_1_3),
      .initial_address(init_addr_1_3),
      .address_valid(port_enable_1_3),
      .address(port_addr_1_3)
  );

    // merge the IO writing and normal writing to SRAM, IO has higher priority.
    // merge the writing and reading, writing has higher priority.
    logic [SRAM_ADDR_WIDTH-1:0] sram_in_addr;
    logic sram_in_en;
    logic [BULK_BITWIDTH-1:0] sram_in_data;
    logic [SRAM_ADDR_WIDTH-1:0] sram_out_addr;
    logic sram_out_en;
    logic [BULK_BITWIDTH-1:0] sram_out_data;
    logic [SRAM_ADDR_WIDTH-1:0] sram_addr;
    logic sram_write_en;
    always_comb begin
        if (port_enable_0_2) begin
            sram_in_en = 1;
            sram_in_addr = port_addr_0_2;
            sram_in_data = io_data_in;
        end
        else begin
            sram_in_en = 0;
            sram_in_addr = 0;
            sram_in_data = 0;
        end
    end
    always_comb begin
        if (port_enable_1_3) begin
            sram_out_en = 1;
            sram_out_addr = port_addr_1_3;
        end
        else begin
            sram_out_en = 0;
            sram_out_addr = 0;
        end
    end
    
    always_comb begin
        if (port_enable_1_3) begin
            bulk_data_out_1 = sram_out_data;
        end
        else begin
            bulk_data_out_1 = 0;
        end
    end

    // IO
    assign io_en_in = port_enable_0_0;
    assign io_addr_in = port_addr_0_0;

    sram #(
        .DEPTH(DEPTH),
        .WIDTH(WIDTH)
    ) sram_inst (
        .clk(clk),
        .enable_a_n(~sram_in_en),
        .enable_b_n(~sram_out_en),
        .write_enable_a_n(~sram_in_en),
        .write_enable_b_n(1'b1),
        .address_a(sram_in_addr),
        .address_b(sram_out_addr),
        .data_a(sram_in_data),
        .data_b(0),
        .q_b(sram_out_data)
    );

endmodule

