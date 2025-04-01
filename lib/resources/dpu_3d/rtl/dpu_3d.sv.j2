`define {{name}} {{fingerprint}}
`define {{name}}_pkg {{fingerprint}}_pkg

{% if not already_defined %}
package {{fingerprint}}_pkg;
    {% for p in parameters %}
    parameter {{p}} = {{parameters[p]}};
    {% endfor %}

    {% set payload_bitwidth = isa.format.instr_bitwidth - isa.format.instr_type_bitwidth - isa.format.instr_opcode_bitwidth - isa.format.instr_slot_bitwidth %}
    {% for instr in isa.instructions %}
    {% if instr.segments is defined and instr.segments|length > 0 %}
    typedef struct packed {
        {% for segment in instr.segments %}
        {% if segment.bitwidth == 1 %}
        logic _{{segment.name}};
        {% else %}
        logic [{{segment.bitwidth-1}}:0] _{{segment.name}};
        {% endif %}
        {% endfor %}
    } {{instr.name}}_t;

    function static {{instr.name}}_t unpack_{{instr.name}};
        input logic [{{payload_bitwidth - 1}}:0] instr;
        {{instr.name}}_t _{{instr.name}};
        {% set index=payload_bitwidth -1 %}
        {% for segment in instr.segments %}
        {% if segment.bitwidth==1 %}
        _{{instr.name}}._{{segment.name}} = instr[{{index}}];
        {% else %}
        _{{instr.name}}._{{segment.name}}  = instr[{{index}}:{{index-segment.bitwidth+1}}];
        {% endif %}
        {% set index=index-segment.bitwidth %}
        {% endfor %}
        return _{{instr.name}};
    endfunction

    function static logic [{{ payload_bitwidth - 1 }}:0] pack_{{instr.name}};
        input {{instr.name}}_t _{{instr.name}};
        logic [{{ payload_bitwidth - 1 }}:0] instr;

        {% set index=payload_bitwidth -1 %}
        {% for segment in instr.segments %}
        {% if segment.bitwidth==1 %}
        instr[{{index}}] = _{{instr.name}}._{{segment.name}};
        {% else %}
        instr[{{index}}:{{index-segment.bitwidth+1}}] = _{{instr.name}}._{{segment.name}};
        {% endif %}
        {% set index=index-segment.bitwidth %}
        {% endfor %}
        return instr;
    endfunction
    {% endif %}
    {% endfor %}
endpackage

module {{fingerprint}}
import {{fingerprint}}_pkg::*;
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
    input  logic clk_4,
    input  logic rst_n_4,
    input  logic instr_en_4,
    input  logic [RESOURCE_INSTR_WIDTH-1:0] instr_4,
    input  logic [3:0] activate_4,
    input  logic [WORD_BITWIDTH-1:0] word_data_in_4,
    output logic [WORD_BITWIDTH-1:0] word_data_out_4,
    input  logic [BULK_BITWIDTH-1:0] bulk_data_in_4,
    output logic [BULK_BITWIDTH-1:0] bulk_data_out_4,
    input  logic clk_5,
    input  logic rst_n_5,
    input  logic instr_en_5,
    input  logic [RESOURCE_INSTR_WIDTH-1:0] instr_5,
    input  logic [3:0] activate_5,
    input  logic [WORD_BITWIDTH-1:0] word_data_in_5,
    output logic [WORD_BITWIDTH-1:0] word_data_out_5,
    input  logic [BULK_BITWIDTH-1:0] bulk_data_in_5,
    output logic [BULK_BITWIDTH-1:0] bulk_data_out_5
);

// Write your implementation here

endmodule

{% endif %}