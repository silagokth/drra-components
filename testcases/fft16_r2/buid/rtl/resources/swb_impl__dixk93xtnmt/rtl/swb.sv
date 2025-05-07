`define swb_impl _dixk93xtnmt
`define swb_impl_pkg _dixk93xtnmt_pkg

package _dixk93xtnmt_pkg;
    parameter BULK_BITWIDTH = 256;
    parameter FSM_PER_SLOT = 4;
    parameter INSTR_OPCODE_BITWIDTH = 3;
    parameter NUM_SLOTS = 16;
    parameter RESOURCE_INSTR_WIDTH = 27;
    parameter WORD_BITWIDTH = 32;

    typedef struct packed {
        logic [1:0] _option;
        logic [3:0] _channel;
        logic [3:0] _source;
        logic [3:0] _target;
    } swb_t;

    function static swb_t unpack_swb;
        input logic [23:0] instr;
        swb_t _swb;
        _swb._option  = instr[23:22];
        _swb._channel  = instr[21:18];
        _swb._source  = instr[17:14];
        _swb._target  = instr[13:10];
        return _swb;
    endfunction

    function static logic [23:0] pack_swb;
        input swb_t _swb;
        logic [23:0] instr;

        instr[23:22] = _swb._option;
        instr[21:18] = _swb._channel;
        instr[17:14] = _swb._source;
        instr[13:10] = _swb._target;
        return instr;
    endfunction
    typedef struct packed {
        logic [1:0] _option;
        logic _sr;
        logic [3:0] _source;
        logic [15:0] _target;
    } route_t;

    function static route_t unpack_route;
        input logic [23:0] instr;
        route_t _route;
        _route._option  = instr[23:22];
        _route._sr = instr[21];
        _route._source  = instr[20:17];
        _route._target  = instr[16:1];
        return _route;
    endfunction

    function static logic [23:0] pack_route;
        input route_t _route;
        logic [23:0] instr;

        instr[23:22] = _route._option;
        instr[21] = _route._sr;
        instr[20:17] = _route._source;
        instr[16:1] = _route._target;
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
    typedef struct packed {
        logic [1:0] _port;
        logic [6:0] _delay_0;
        logic [6:0] _delay_1;
        logic [6:0] _delay_2;
    } fsm_t;

    function static fsm_t unpack_fsm;
        input logic [23:0] instr;
        fsm_t _fsm;
        _fsm._port  = instr[23:22];
        _fsm._delay_0  = instr[21:15];
        _fsm._delay_1  = instr[14:8];
        _fsm._delay_2  = instr[7:1];
        return _fsm;
    endfunction

    function static logic [23:0] pack_fsm;
        input fsm_t _fsm;
        logic [23:0] instr;

        instr[23:22] = _fsm._port;
        instr[21:15] = _fsm._delay_0;
        instr[14:8] = _fsm._delay_1;
        instr[7:1] = _fsm._delay_2;
        return instr;
    endfunction

    parameter OPCODE_SWB = 3'b100;
    parameter OPCODE_ROUTE = 3'b101;

    parameter NUM_OPTIONS = 4;

endpackage

module _dixk93xtnmt
import _dixk93xtnmt_pkg::*;
(
    input  logic clk_0,
    input  logic rst_n_0,
    input  logic instr_en_0,
    input  logic [RESOURCE_INSTR_WIDTH-1:0] instr_0,
    input  logic [FSM_PER_SLOT-1:0] activate_0,
    input logic [NUM_SLOTS-1:0][WORD_BITWIDTH-1:0] word_channels_in,
    output logic [NUM_SLOTS-1:0][WORD_BITWIDTH-1:0] word_channels_out,
    input logic [NUM_SLOTS-1:0][BULK_BITWIDTH-1:0] bulk_intracell_in,
    output logic [NUM_SLOTS-1:0][BULK_BITWIDTH-1:0] bulk_intracell_out,
    input logic [BULK_BITWIDTH-1:0] bulk_intercell_n_in,
    input logic [BULK_BITWIDTH-1:0] bulk_intercell_w_in,
    input logic [BULK_BITWIDTH-1:0] bulk_intercell_e_in,
    input logic [BULK_BITWIDTH-1:0] bulk_intercell_s_in,
    output logic [BULK_BITWIDTH-1:0] bulk_intercell_n_out,
    output logic [BULK_BITWIDTH-1:0] bulk_intercell_w_out,
    output logic [BULK_BITWIDTH-1:0] bulk_intercell_e_out,
    output logic [BULK_BITWIDTH-1:0] bulk_intercell_s_out
);

    logic clk, rst_n;
    assign clk = clk_0;
    assign rst_n = rst_n_0;

    logic [2:0] opcode;
    logic [23:0] payload;
    swb_t swb;
    route_t route;

    logic [NUM_OPTIONS-1:0][NUM_SLOTS-1:0][3:0] swb_configs, swb_configs_next;
    logic [NUM_OPTIONS-1:0][3:0] route_in_src_configs, route_in_src_configs_next;
    logic [NUM_OPTIONS-1:0][15:0] route_in_dst_configs, route_in_dst_configs_next;
    logic [NUM_OPTIONS-1:0][3:0] route_out_src_configs, route_out_src_configs_next;
    logic [NUM_OPTIONS-1:0][15:0] route_out_dst_configs, route_out_dst_configs_next;

    logic [NUM_SLOTS-1:0][3:0] curr_swb_configs, curr_swb_configs_next;
    logic [3:0] curr_route_in_src_configs, curr_route_in_src_configs_next;
    logic [15:0] curr_route_in_dst_configs, curr_route_in_dst_configs_next;
    logic [3:0] curr_route_out_src_configs, curr_route_out_src_configs_next;
    logic [15:0] curr_route_out_dst_configs, curr_route_out_dst_configs_next;

    logic [1:0] current_option;

    assign current_option = 0;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            swb_configs <= 0;
            route_in_src_configs <= 0;
            route_in_dst_configs <= 0;
            route_out_src_configs <= 0;
            route_out_dst_configs <= 0;
            curr_swb_configs <= 0;
            curr_route_in_src_configs <= 0;
            curr_route_in_dst_configs <= 0;
            curr_route_out_src_configs <= 0;
            curr_route_out_dst_configs <= 0;
        end else begin
            swb_configs <= swb_configs_next;
            route_in_src_configs <= route_in_src_configs_next;
            route_in_dst_configs <= route_in_dst_configs_next;
            route_out_src_configs <= route_out_src_configs_next;
            route_out_dst_configs <= route_out_dst_configs_next;
            curr_swb_configs <= curr_swb_configs_next;
            curr_route_in_src_configs <= curr_route_in_src_configs_next;
            curr_route_in_dst_configs <= curr_route_in_dst_configs_next;
            curr_route_out_src_configs <= curr_route_out_src_configs_next;
            curr_route_out_dst_configs <= curr_route_out_dst_configs_next;
        end
    end

    always_comb begin
      opcode = 0;
      payload = 0;
      swb_configs_next = swb_configs;
      route_in_src_configs_next = route_in_src_configs;
      route_in_dst_configs_next = route_in_dst_configs;
      route_out_src_configs_next = route_out_src_configs;
      route_out_dst_configs_next = route_out_dst_configs;
      curr_swb_configs_next = curr_swb_configs;
      curr_route_in_src_configs_next = curr_route_in_src_configs;
      curr_route_in_dst_configs_next = curr_route_in_dst_configs;
      curr_route_out_src_configs_next = curr_route_out_src_configs;
      curr_route_out_dst_configs_next = curr_route_out_dst_configs;

      if (activate_0[0]) begin
        curr_swb_configs_next = swb_configs[current_option];
      end
      if (activate_0[2]) begin
        curr_route_in_src_configs_next = route_in_src_configs[current_option];
        curr_route_in_dst_configs_next = route_in_dst_configs[current_option];
        curr_route_out_src_configs_next = route_out_src_configs[current_option];
        curr_route_out_dst_configs_next = route_out_dst_configs[current_option];
      end
      if (instr_en_0) begin
          opcode = instr_0[26:24];
          payload = instr_0[23:0];
          case(opcode)
              OPCODE_SWB: begin
                      // Switchbox
                      swb = unpack_swb(payload);
                      swb_configs_next[swb._option][swb._target] = swb._source;
                  end
              OPCODE_ROUTE: begin
                      // Router
                      route = unpack_route(payload);
                      if (route._sr==1'b0) begin // send
                          route_out_src_configs_next[route._option] = route._source;
                          route_out_dst_configs_next[route._option] = route._target;
                      end else begin // receive
                          route_in_src_configs_next[route._option] = route._source;
                          route_in_dst_configs_next[route._option] = route._target;
                      end
              end
              default: begin
                      // Do nothing
              end
          endcase
      end
    end

    logic [BULK_BITWIDTH-1:0] bulk_intercell_c_in, bulk_intercell_c_out;
    logic [BULK_BITWIDTH-1:0] bulk_intercell_self;
    always_comb begin
      bulk_intercell_c_in = 0;
      bulk_intercell_c_out = 0;
      bulk_intercell_self = 0;
      bulk_intercell_e_out = 0;
      bulk_intercell_n_out = 0;
      bulk_intercell_w_out = 0;
      bulk_intercell_s_out = 0;

      for(int i=0; i<NUM_SLOTS; i=i+1) begin
        word_channels_out[i] = word_channels_in[curr_swb_configs[i]];
      end

      bulk_intercell_c_out = bulk_intracell_in[curr_route_out_src_configs];
      
      if (curr_route_out_dst_configs[1]) begin
        bulk_intercell_n_out = bulk_intercell_c_out;
      end else if (curr_route_out_dst_configs[3]) begin
        bulk_intercell_w_out = bulk_intercell_c_out;
      end else if (curr_route_out_dst_configs[3]) begin
        bulk_intercell_self = bulk_intercell_c_out;
      end else if (curr_route_out_dst_configs[5]) begin
        bulk_intercell_e_out = bulk_intercell_c_out;
      end else if (curr_route_out_dst_configs[7]) begin
        bulk_intercell_s_out = bulk_intercell_c_out;
      end

      if (curr_route_in_src_configs == 1) begin
        bulk_intercell_c_in = bulk_intercell_n_in;
      end else if (curr_route_out_src_configs == 3) begin
        bulk_intercell_c_in = bulk_intercell_w_in;
      end else if (curr_route_out_src_configs == 4) begin
        bulk_intercell_c_in = bulk_intercell_self;
      end else if (curr_route_out_src_configs == 5) begin
        bulk_intercell_c_in = bulk_intercell_e_in;
      end else if (curr_route_out_src_configs == 7) begin
        bulk_intercell_c_in = bulk_intercell_s_in;
      end

      for(int i=0; i<NUM_SLOTS; i=i+1) begin
        bulk_intracell_out[i] = 0;
        if(curr_route_in_dst_configs[i]) begin
          bulk_intracell_out[i] = bulk_intercell_c_in;
        end
      end

    end
endmodule

