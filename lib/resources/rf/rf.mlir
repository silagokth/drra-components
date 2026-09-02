// Functional description of the rf resource.
//
// The rf has no conf.mode. What selects behaviour is the evt/rep/trans port
// field, so @match holds one func.func per port in isa.json's verbo_map, named
// after it. Each takes the register file and the AGU address as operands.
//
// @replace holds the counterpart under the same name and signature. The address
// operand goes unused in every one of them: the AGU sweeps it, so it is not an
// instruction operand -- the affine map is lifted onto the rop separately.
//
// Each replacement rop also says, in `uses`, which parts of the RF it holds
// while it is active. An RF is not indivisible: rtl/logic.sv.j2 gives it four
// independent access paths, and the AGU index there *is* the port number --
// agu_valid[0] word write, [1] word read, [2] bulk write, [3] bulk read -- each
// with its own data wires. So a word write and a bulk read want nothing in
// common and can run on one RF, while two word reads cannot.
//
// The register array itself is deliberately not listed. Every access touches
// it, so listing it would make every pair of accesses conflict and say nothing
// more than "same resource" does. Two arrays sharing one RF is a question of
// addresses and RF_DEPTH, not of parts; what keeps distinct arrays apart is the
// storage identity the compiler lifts off the accessed memref.
//
// Widths are fixed to the reference configuration: WORD_BITWIDTH = 16,
// RF_DEPTH = 64, IO_DATA_WIDTH = 256, so a bulk access is 16 words.

module @rf {

  module @match {

    // Reads happen early in the cycle and writes late, so a value another
    // resource produces this cycle is written back on the next one.

    func.func @word_read(%registers: memref<64xi16>, %addr: index) -> i16
        attributes {benefit = 10 : i32} {
      %v = memref.load %registers[%addr] : memref<64xi16>
      return %v : i16
    }

    func.func @word_write(%registers: memref<64xi16>, %addr: index, %v: i16)
        attributes {benefit = 10 : i32} {
      memref.store %v, %registers[%addr] : memref<64xi16>
      return
    }

    // A bulk address is scaled by the number of words in a bulk transfer, so
    // bulk address a covers registers[16*a .. 16*a+15]. That scaling is why
    // BULK_ADDR_WIDTH is 2 against a 64-deep file. It is descriptive: the
    // matcher never walks into a pattern's own address arithmetic.
    func.func @bulk_read(%registers: memref<64xi16>, %addr: index) -> vector<16xi16>
        attributes {benefit = 10 : i32} {
      %words = arith.constant 16 : index
      %base = arith.muli %addr, %words : index
      %v = vector.load %registers[%base] : memref<64xi16>, vector<16xi16>
      return %v : vector<16xi16>
    }

    func.func @bulk_write(%registers: memref<64xi16>, %addr: index, %v: vector<16xi16>)
        attributes {benefit = 10 : i32} {
      %words = arith.constant 16 : index
      %base = arith.muli %addr, %words : index
      vector.store %v, %registers[%base] : memref<64xi16>, vector<16xi16>
      return
    }

    // Not an evt port and not selectable: conf writes one register straight
    // from the instruction stream, with the address and value as instruction
    // segments rather than an AGU address and a data port, so both come from
    // the match rather than from a fixed value. No replacement expresses that
    // yet.
    func.func @conf(%registers: memref<64xi16>, %addr: index, %value: i16)
        attributes {benefit = 10 : i32} {
      memref.store %value, %registers[%addr] : memref<64xi16>
      return
    }
  }

  module @replace {

    func.func @word_read(%registers: memref<64xi16>, %addr: index) -> i16 {
      %v = drra.rop {evt = {port = 1 : i32},
                    uses = ["agu:1", "output_narrow:0"]} : () -> i16
      return %v : i16
    }

    func.func @word_write(%registers: memref<64xi16>, %addr: index, %v: i16) {
      drra.rop %v {evt = {port = 0 : i32},
                  uses = ["agu:0", "input_narrow:0"]} : (i16) -> ()
      return
    }

    func.func @bulk_read(%registers: memref<64xi16>, %addr: index) -> vector<16xi16> {
      %v = drra.rop {evt = {port = 3 : i32},
                    uses = ["agu:3", "output_bulk:0"]} : () -> vector<16xi16>
      return %v : vector<16xi16>
    }

    func.func @bulk_write(%registers: memref<64xi16>, %addr: index, %v: vector<16xi16>) {
      drra.rop %v {evt = {port = 2 : i32},
                  uses = ["agu:2", "input_bulk:0"]} : (vector<16xi16>) -> ()
      return
    }

    // Never selected. The conf carries no segments because both of them --
    // address and value -- come from the match rather than from a fixed value,
    // and there is no way to say that yet. That is the one thing standing
    // between this and being selectable.
    func.func @conf(%registers: memref<64xi16>, %addr: index, %value: i16) {
      drra.rop %value {conf = {},
                      uses = ["conf_write"]} : (i16) -> ()
      return
    }
  }
}
