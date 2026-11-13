// Functional description of the rf_1row resource: a 16-word, single-row
// register file with the ISA and ports of rf_2row and rf_4row.
//
// The patterns are rf_4row's (see rf_4row.mlir for what each part means). The
// three register files differ only in size, and the memref type is not
// compared when matching, so all three match every array. The smallest has the
// highest benefit and is the one selected; design-space exploration then binds
// the access to any register file (category memory) the array fits in.
//
// With a single row the bulk address is always 0, and a bulk access covers
// all 16 words.

module @rf_1row {

  module @match {

    // Reads happen early in the cycle and writes late, so a value another
    // resource produces this cycle is written back on the next one.

    func.func @word_read(%registers: memref<16xi16>, %addr: index) -> i16
        attributes {benefit = 10 : i32} {
      %v = memref.load %registers[%addr] : memref<16xi16>
      return %v : i16
    }

    func.func @word_write(%registers: memref<16xi16>, %addr: index, %v: i16)
        attributes {benefit = 10 : i32} {
      memref.store %v, %registers[%addr] : memref<16xi16>
      return
    }

    // A bulk address is scaled by the number of words in a bulk transfer, so
    // bulk address a covers registers[16*a .. 16*a+15]. rf_1row has one row,
    // so a is always 0 (BULK_ADDR_WIDTH is 0). The scaling is kept to match
    // rf_4row, and it is descriptive: the matcher never walks into a pattern's own
    // address arithmetic.
    func.func @bulk_read(%registers: memref<16xi16>, %addr: index) -> vector<16xi16>
        attributes {benefit = 10 : i32} {
      %words = arith.constant 16 : index
      %base = arith.muli %addr, %words : index
      %v = vector.load %registers[%base] : memref<16xi16>, vector<16xi16>
      return %v : vector<16xi16>
    }

    func.func @bulk_write(%registers: memref<16xi16>, %addr: index, %v: vector<16xi16>)
        attributes {benefit = 10 : i32} {
      %words = arith.constant 16 : index
      %base = arith.muli %addr, %words : index
      vector.store %v, %registers[%base] : memref<16xi16>, vector<16xi16>
      return
    }

    // Not an evt port and not selectable: conf writes one register straight
    // from the instruction stream, with the address and value as instruction
    // segments rather than an AGU address and a data port, so both come from
    // the match rather than from a fixed value. No replacement expresses that
    // yet.
    func.func @conf(%registers: memref<16xi16>, %addr: index, %value: i16)
        attributes {benefit = 10 : i32} {
      memref.store %value, %registers[%addr] : memref<16xi16>
      return
    }
  }

  module @replace {

    func.func @word_read(%registers: memref<16xi16>, %addr: index) -> i16 {
      %v = drra.rop {evt = {port = 1 : i32},
                    uses = ["agu:1", "output_narrow:0"]} : () -> i16
      return %v : i16
    }

    func.func @word_write(%registers: memref<16xi16>, %addr: index, %v: i16) {
      drra.rop %v {evt = {port = 0 : i32},
                  uses = ["agu:0", "input_narrow:0"]} : (i16) -> ()
      return
    }

    func.func @bulk_read(%registers: memref<16xi16>, %addr: index) -> vector<16xi16> {
      %v = drra.rop {evt = {port = 3 : i32},
                    uses = ["agu:3", "output_bulk:0"]} : () -> vector<16xi16>
      return %v : vector<16xi16>
    }

    func.func @bulk_write(%registers: memref<16xi16>, %addr: index, %v: vector<16xi16>) {
      drra.rop %v {evt = {port = 2 : i32},
                  uses = ["agu:2", "input_bulk:0"]} : (vector<16xi16>) -> ()
      return
    }

    // Never selected. The conf carries no segments because both of them --
    // address and value -- come from the match rather than from a fixed value,
    // and there is no way to say that yet. That is the one thing standing
    // between this and being selectable.
    func.func @conf(%registers: memref<16xi16>, %addr: index, %value: i16) {
      drra.rop %value {conf = {},
                      uses = ["conf_write"]} : (i16) -> ()
      return
    }
  }
}
