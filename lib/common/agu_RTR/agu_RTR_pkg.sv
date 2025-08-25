package agu_RTR_pkg;

    typedef struct packed {
        logic [5:0] _iter;
        logic [6:0] _delay;
        logic [3:0] _step;
        logic [2:0] _level;
        logic [1:0] _option;
    } rep_t;

    typedef struct packed {
        logic [5:0] _iter;
        logic [6:0] _delay;
        logic [3:0] _step;
        logic [2:0] _level;
        logic [1:0] _option;
    } repx_t;

    typedef struct packed {
        logic [17:0] _delay;
        logic [ 3:0] _level;
    } trans_t;

    function static rep_t unpack_rep;
        input logic [21:0] instr;
        rep_t _rep;
        _rep._iter    = instr[ 5: 0];
        _rep._delay   = instr[12: 6];
        _rep._step    = instr[16:13];
        _rep._level   = instr[19:17];
        _rep._option  = instr[21:20];
        return _rep;
    endfunction

    function static repx_t unpack_repx;
        input logic [21:0] instr;
        repx_t _repx;
        _repx._iter    = instr[ 5: 0];
        _repx._delay   = instr[12: 6];
        _repx._step    = instr[16:13];
        _repx._level   = instr[19:17];
        _repx._option  = instr[21:20];
        return _repx;
    endfunction

    function static trans_t unpack_trans;
        input logic [21:0] instr;
        trans_t _trans;
        _trans._delay   = instr[17:0];
        _trans._level   = instr[21:18];
        return _trans;
    endfunction

endpackage : agu_RTR_pkg