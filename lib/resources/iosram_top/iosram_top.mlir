// Functional description of the iosram_top resource.
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
// So @sram_write and @bulk_write are both evt port 2, on slot+0 and slot+1
// respectively. Nothing sits on slot+0 port 3: that would be the sram-to-io
// read, which iosram_top has no io output direction to drive, so @bulk_read on
// slot+1 is the only port 3.
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
//   AGU 0  IO in                  @input_buffer
//   AGU 1  SRAM write from IO     @sram_write
//   AGU 2  SRAM write from bulk   @bulk_write
//   AGU 3  SRAM read to bulk      @bulk_read
//
// The SRAM is dual ported -- one write port, one read port. Two writers share
// the write port here, @sram_write winning it over @bulk_write, which is the
// arbitration the note on @bulk_write below describes; @bulk_read has the read
// port to itself. That is what `sram_write_port` and `sram_read_port` say. The SRAM contents are deliberately not a part: every
// access touches them, so listing them would make every pair conflict and say
// nothing more than "same resource" does.
//
// Widths are fixed to the reference configuration: WORD_BITWIDTH = 16,
// IO_DATA_WIDTH = 256 (16 words per transfer), SRAM_ADDR_WIDTH = 6 (64 rows),
// IO_ADDR_WIDTH = 16.
//
// iosram_top has no io output connection: an evt on the output_buffer
// port is rejected, so no function describes it.

module @iosram_top {

  module @match {

    // Io staging, inbound. The request goes out early in the cycle; the
    // response is consumed by @sram_write later in the same cycle, which is why
    // the two are separate functions rather than one transfer.
    func.func @input_buffer(%io_buffer: memref<?xvector<16xi16>, 1>, %addr: index) -> vector<16xi16>
        attributes {benefit = 11 : i32} {
      %v = memref.load %io_buffer[%addr] : memref<?xvector<16xi16>, 1>
      return %v : vector<16xi16>
    }

    // The io response landing in the sram. Holds priority on the single sram
    // write port; see @bulk_write.
    func.func @sram_write(%sram: memref<64xvector<16xi16>>, %addr: index, %v: vector<16xi16>)
        attributes {benefit = 11 : i32} {
      memref.store %v, %sram[%addr] : memref<64xvector<16xi16>>
      return
    }

    // The fabric path, on the resource's second slot.

    func.func @bulk_read(%sram: memref<64xvector<16xi16>>, %addr: index) -> vector<16xi16>
        attributes {benefit = 11 : i32} {
      %v = memref.load %sram[%addr] : memref<64xvector<16xi16>>
      return %v : vector<16xi16>
    }

    // The sram has a single write port and @sram_write wins it: when that AGU
    // produces an address in the same cycle, the RTL write mux drops this write
    // and the routed data arriving on the bulk port is discarded. That
    // arbitration is a property of the two ports together, not of this access,
    // so it is stated here rather than built into the function.
    func.func @bulk_write(%sram: memref<64xvector<16xi16>>, %addr: index, %v: vector<16xi16>)
        attributes {benefit = 11 : i32} {
      memref.store %v, %sram[%addr] : memref<64xvector<16xi16>>
      return
    }
  }

  module @replace {

    func.func @input_buffer(%io_buffer: memref<?xvector<16xi16>, 1>, %addr: index) -> vector<16xi16> {
      %v = drra.rop {evt = {port = 0 : i32},
                    uses = ["agu:0", "external_read:0"]} : () -> vector<16xi16>
      return %v : vector<16xi16>
    }

    func.func @sram_write(%sram: memref<64xvector<16xi16>>, %addr: index, %v: vector<16xi16>) {
      drra.rop %v {evt = {port = 2 : i32},
                  uses = ["agu:1", "sram_write_port:0"]} : (vector<16xi16>) -> ()
      return
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
