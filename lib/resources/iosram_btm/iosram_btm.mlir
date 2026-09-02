// Functional description of the iosram_btm resource.
//
// @match holds one func.func per (slot offset, evt port) pair, named after what
// it does. The resource spans 4 slots and getRelativePortNum() maps (slot,
// port) to an AGU as slot_offset * PORTS_PER_SLOT + port, so the same four ISA
// port names address a different datapath depending on which of the resource's
// slots the evt names:
//
//   slot+0  the io staging path: io <-> the internal staging buffers <-> sram
//   slot+1  the fabric path: sram <-> data_port1
//
// So @sram_read and @bulk_read are both evt port 3, on slot+0 and slot+1
// respectively. Nothing sits on slot+0 port 2: that would be the io-to-sram
// write, which iosram_btm has no io input direction to drive, so @bulk_write on
// slot+1 is the only port 2.
//
// The sram and the external io subsystem are both operands. The io side is in
// memref address space 1 to mark it as off-resource, and its depth is dynamic
// because the io address space is external; the sram side is bounded by the
// sram depth.
//
// Every match has a counterpart in @replace, but only the io-side port is
// selectable from a program written in affine + arith: the sram and fabric
// paths are AGU-driven and nothing in such a program expresses them, so they
// are loaded and then skipped.
//
// Each replacement rop also says, in `uses`, which parts of the resource it
// holds while it is active. rtl/logic.sv.j2 gives the AGU map:
//
//   AGU 0  IO out                 @output_buffer
//   AGU 1  SRAM read to IO        @sram_read
//   AGU 2  SRAM write from bulk   @bulk_write
//   AGU 3  SRAM read to bulk      @bulk_read
//
// The SRAM is dual ported -- one write port, one read port. Two readers share
// the read port here, @sram_read winning it over @bulk_read; @bulk_write has
// the write port to itself. That is what `sram_write_port` and `sram_read_port`
// say. The SRAM contents are deliberately not a part: every
// access touches them, so listing them would make every pair conflict and say
// nothing more than "same resource" does.
//
// Widths are fixed to the reference configuration: WORD_BITWIDTH = 16,
// IO_DATA_WIDTH = 256 (16 words per transfer), SRAM_ADDR_WIDTH = 6 (64 rows),
// IO_ADDR_WIDTH = 16.
//
// iosram_btm has no io input connection: an evt on the input_buffer
// port is rejected, so no function describes it.

module @iosram_btm {

  module @match {

    // Io staging, outbound. @sram_read fills the staging buffer mid-cycle and
    // this sends it at the end of the cycle.
    func.func @output_buffer(%io_buffer: memref<?xvector<16xi16>, 1>, %addr: index, %v: vector<16xi16>)
        attributes {benefit = 11 : i32} {
      memref.store %v, %io_buffer[%addr] : memref<?xvector<16xi16>, 1>
      return
    }

    func.func @sram_read(%sram: memref<64xvector<16xi16>>, %addr: index) -> vector<16xi16>
        attributes {benefit = 11 : i32} {
      %v = memref.load %sram[%addr] : memref<64xvector<16xi16>>
      return %v : vector<16xi16>
    }

    // The fabric path, on the resource's second slot.

    func.func @bulk_read(%sram: memref<64xvector<16xi16>>, %addr: index) -> vector<16xi16>
        attributes {benefit = 11 : i32} {
      %v = memref.load %sram[%addr] : memref<64xvector<16xi16>>
      return %v : vector<16xi16>
    }

    // Nothing contends for the sram write port here: the io-to-sram write that
    // would hold priority over this one needs an io input direction, which
    // iosram_btm does not have.
    func.func @bulk_write(%sram: memref<64xvector<16xi16>>, %addr: index, %v: vector<16xi16>)
        attributes {benefit = 11 : i32} {
      memref.store %v, %sram[%addr] : memref<64xvector<16xi16>>
      return
    }
  }

  module @replace {

    func.func @output_buffer(%io_buffer: memref<?xvector<16xi16>, 1>, %addr: index, %v: vector<16xi16>) {
      drra.rop %v {evt = {port = 1 : i32},
                  uses = ["agu:0", "external_write:0"]} : (vector<16xi16>) -> ()
      return
    }

    func.func @sram_read(%sram: memref<64xvector<16xi16>>, %addr: index) -> vector<16xi16> {
      %v = drra.rop {evt = {port = 3 : i32},
                    uses = ["agu:1", "sram_read_port:0"]} : () -> vector<16xi16>
      return %v : vector<16xi16>
    }

    func.func @bulk_read(%sram: memref<64xvector<16xi16>>, %addr: index) -> vector<16xi16> {
      %v = drra.rop {evt = {port = 3 : i32}, slot_offset = 1 : i32,
                    uses = ["agu:3", "sram_read_port:0", "output_bulk:0"]}
          : () -> vector<16xi16>
      return %v : vector<16xi16>
    }

    func.func @bulk_write(%sram: memref<64xvector<16xi16>>, %addr: index, %v: vector<16xi16>) {
      drra.rop %v {evt = {port = 2 : i32}, slot_offset = 1 : i32,
                  uses = ["agu:2", "sram_write_port:0", "input_bulk:0"]}
          : (vector<16xi16>) -> ()
      return
    }
  }
}
