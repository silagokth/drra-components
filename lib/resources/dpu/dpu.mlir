// Functional description of the dpu resource.
//
// @match holds one func.func per conf.mode in isa.json, named after it, plus
// @rst for the evt rst port. Its arguments are the wildcards and its body is
// the shape to look for in the program.
//
// @replace holds the counterpart under the same name, with the same signature,
// so an argument binds positionally to whatever the match bound -- MLIR drops
// SSA names at parse time, so position is what links the two halves. An
// argument the replacement leaves unused is dropped from the instruction: that
// is how the accumulate register stays implicit in the DPU rather than being
// special-cased in the compiler.
//
// These describe the arithmetic only. The hardware clamps every result to the
// word rather than wrapping (rtl/adder.sv.j2 and rtl/multiplier.sv.j2 with
// saturate = 1, and int64ToVector() on the SST side); that saturation is left
// out here, so overflow behaviour is not described.
//
// Widths are fixed to the reference configuration: WORD_BITWIDTH = 16,
// FRACTIONAL_BITWIDTH = 0, IS_SIGNED = 1.

module @dpu {

  module @match {

    // conf.mode 0. Drives nothing, so there is no body to match and no
    // replacement: it is never selected out of a program.
    func.func @idle()
        attributes {benefit = 10 : i32} {
      return
    }

    func.func @add(%a: i16, %b: i16) -> i16
        attributes {benefit = 10 : i32} {
      %c = arith.addi %a, %b : i16
      return %c : i16
    }

    // At FRACTIONAL_BITWIDTH = 0 the product needs no rescaling. For a non-zero
    // FRAC this is where multiplier.sv.j2's round-half-away-from-zero shift by
    // FRAC would sit.
    func.func @mult(%a: i16, %b: i16) -> i16
        attributes {benefit = 10 : i32} {
      %c = arith.muli %a, %b : i16
      return %c : i16
    }

    // The accumulate register is state, so it is an operand the function reads
    // and writes rather than a value passed in: the product is added to
    // whatever the register currently holds, and the new total is stored back
    // as well as driven out. Nothing here initialises it -- @rst does that.
    func.func @mac(%acc: memref<i16>, %a: i16, %b: i16) -> i16
        attributes {benefit = 20 : i32} {
      %cur = memref.load %acc[] : memref<i16>
      %p = arith.muli %a, %b : i16
      %s = arith.addi %cur, %p : i16
      memref.store %s, %acc[] : memref<i16>
      return %s : i16
    }

    // Clears the accumulate register. It returns the cleared value as well as
    // storing it, because that value is what the program carries into the loop
    // as the iter_args initialiser -- the match roots on whatever produces it.
    func.func @rst(%acc: memref<i16>) -> i16
        attributes {benefit = 10 : i32} {
      %zero = arith.constant 0 : i16
      memref.store %zero, %acc[] : memref<i16>
      return %zero : i16
    }
  }

  module @replace {

    // Never selected -- @idle has no body to match -- but it is still a real
    // DPU mode, so the counterpart records what it would lower to.
    func.func @idle() {
      drra.rop {conf = {mode = 0 : i32}} : () -> ()
      return
    }

    func.func @add(%a: i16, %b: i16) -> i16 {
      %r = drra.rop %a, %b {conf = {mode = 1 : i32}} : (i16, i16) -> i16
      return %r : i16
    }

    func.func @mult(%a: i16, %b: i16) -> i16 {
      %r = drra.rop %a, %b {conf = {mode = 7 : i32}} : (i16, i16) -> i16
      return %r : i16
    }

    // %acc is unused: the accumulate register is not an instruction operand.
    func.func @mac(%acc: memref<i16>, %a: i16, %b: i16) -> i16 {
      %r = drra.rop %a, %b {conf = {mode = 10 : i32}} : (i16, i16) -> i16
      return %r : i16
    }

    // An evt on the rst port, on its own timing pattern. It still produces the
    // value the loop carries in.
    func.func @rst(%acc: memref<i16>) -> i16 {
      %r = drra.rop {evt = {port = 1 : i32}} : () -> i16
      return %r : i16
    }
  }
}
