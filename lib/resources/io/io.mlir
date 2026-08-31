// Functional description of the io resource.
//
// @match holds one func.func per evt port in isa.json, named after it. The
// external io subsystem is a memref in address space 1, passed in as an
// operand, so a transfer is an ordinary load or store against it -- the io
// resource has no local storage of its own. Its depth is dynamic because the
// io address space is external and was never a real constraint.
//
// Each direction is really two port actions on the same AGU, one subcycle
// apart: the request goes out, and the payload moves on the following
// subcycle. The staging buffer between the two halves is what keeps the output
// path from racing the input path.
//
// @replace holds the counterpart under the same name and signature. The buffer
// and the address both go unused: the AGU sweeps the address, and which buffer
// is addressed is the resource's own wiring, not an instruction operand.
//
// Widths are fixed to the reference configuration: WORD_BITWIDTH = 16,
// IO_DATA_WIDTH = 256 (16 words per transfer), IO_ADDR_WIDTH = 16.

module @io {

  module @match {

    func.func @input_buffer(%io_input: memref<?xvector<16xi16>, 1>,
                            %addr: index) -> vector<16xi16>
        attributes {benefit = 12 : i32} {
      %v = memref.load %io_input[%addr] : memref<?xvector<16xi16>, 1>
      return %v : vector<16xi16>
    }

    func.func @output_buffer(%io_output: memref<?xvector<16xi16>, 1>,
                             %addr: index, %v: vector<16xi16>)
        attributes {benefit = 12 : i32} {
      memref.store %v, %io_output[%addr] : memref<?xvector<16xi16>, 1>
      return
    }
  }

  module @replace {

    func.func @input_buffer(%io_input: memref<?xvector<16xi16>, 1>,
                            %addr: index) -> vector<16xi16> {
      %v = drra.rop {evt = {port = 0 : i32}} : () -> vector<16xi16>
      return %v : vector<16xi16>
    }

    func.func @output_buffer(%io_output: memref<?xvector<16xi16>, 1>,
                             %addr: index, %v: vector<16xi16>) {
      drra.rop %v {evt = {port = 1 : i32}} : (vector<16xi16>) -> ()
      return
    }
  }
}
