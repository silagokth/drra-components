// Functional description of the rf_row resource: a 16-word, single-row
// register file with rf's ISA and ports.
//
// The patterns are rf's (see rf.mlir for what each part means), with two
// differences:
//
//   - `benefit = 9`, one BELOW rf, so for now rf_row is never selected: rf's
//     patterns (10) match every array and claim it first. The compiler cannot
//     yet tell whether an array fits in 16 words, and it does not check that
//     the fabric has an rf_row at all, so ranking rf_row above rf would put
//     arrays of any size into it -- and fail binding on every fabric without
//     one. rf_row is usable from pasm only until vesyla supports it.
//   - `fits_memref`: marks the memref<16xi16> as a capacity, for when the
//     compiler gains that check (only a static array of at most 16 i16
//     elements would match). Today the attribute is ignored.

// With a single row the bulk address is always 0, and a bulk access covers
// all 16 words.

module @rf_row {

  module @match {

    // Reads happen early in the cycle and writes late, so a value another
    // resource produces this cycle is written back on the next one.

    func.func @word_read(%registers: memref<16xi16>, %addr: index) -> i16
        attributes {benefit = 9 : i32, fits_memref} {
      %v = memref.load %registers[%addr] : memref<16xi16>
      return %v : i16
    }

    func.func @word_write(%registers: memref<16xi16>, %addr: index, %v: i16)
        attributes {benefit = 9 : i32, fits_memref} {
      memref.store %v, %registers[%addr] : memref<16xi16>
      return
    }

    // A bulk address is scaled by the number of words in a bulk transfer, so
    // bulk address a covers registers[16*a .. 16*a+15]. rf_row has one row,
    // so a is always 0 (BULK_ADDR_WIDTH is 0). The scaling is kept to match
    // rf, and it is descriptive: the matcher never walks into a pattern's own
    // address arithmetic.
    func.func @bulk_read(%registers: memref<16xi16>, %addr: index) -> vector<16xi16>
        attributes {benefit = 9 : i32, fits_memref} {
      %words = arith.constant 16 : index
      %base = arith.muli %addr, %words : index
      %v = vector.load %registers[%base] : memref<16xi16>, vector<16xi16>
      return %v : vector<16xi16>
    }

    func.func @bulk_write(%registers: memref<16xi16>, %addr: index, %v: vector<16xi16>)
        attributes {benefit = 9 : i32, fits_memref} {
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
        attributes {benefit = 9 : i32, fits_memref} {
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
