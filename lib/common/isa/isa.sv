// ISA objects and classes

class instruction
  int[1:0] fields [string];

  function instruction pack(int[1:0] fields [string]);
    instruction inst;
    inst.fields = fields;
    return inst;
  endfunction

  function logic[31:0] unpack();
    // foreach field, shift left by the field[0]

    logic[31:0] instr;
    instr = 32'b0;
    foreach this.fields[i] begin
      instr = instr | (fields[i][1:0] << i);
    end
    return instr;
  endfunction
endclass
