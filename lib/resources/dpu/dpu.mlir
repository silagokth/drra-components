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
// Each replacement rop also says, in `uses`, which parts of the DPU it holds
// while it is active. A DPU is not indivisible: it has four configuration
// registers, two narrow inputs and a narrow output, and one AGU per event port.
// %a arrives on input_narrow:1 and %b on input_narrow:0 -- the operand order of
// the rop is not the port order, so the list is written in port order.
// An arithmetic mode holds the config register its mode word sits in and the
// datapath it drives; @rst holds only the AGU of the rst port. So a mac and a
// rst want nothing in common and can run on the same DPU, while two arithmetic
// modes cannot -- one config register holds one mode. That is what the compiler
// intersects when it decides which operations may share an instance; the names
// are ours, and it only ever asks whether two of them are spelled the same.
//
// Each arithmetic replacement also says, in `endpoints`, which slot of the DPU
// each of its operands and results uses, as an offset from the first slot the
// instance occupies. A DPU is two slots wide, and that is not an accounting
// detail: the switchbox addresses a channel by slot and knows nothing of ports,
// so the two narrow inputs are reached as two different target slots. Two
// operands left on one slot would share one channel and only one of them would
// ever arrive. `uses` cannot answer this -- those names are ours, and the
// compiler only ever asks whether two of them are spelled the same -- so the
// slot each value travels on is stated separately, in the operation's own
// result and operand order. A single-slot resource says nothing and gets the
// one slot it has.
//
// Each arithmetic replacement also carries its timing, per pass rather than per
// loop:
//   - `latency`: the cycles from the event that starts one pass through the
//     operation to that pass's result being there to be taken;
//   - `ii` (initiation interval): the cycles between the starts of two
//     successive passes.
// Where a value is carried out of a loop of N passes, the compiler times it at
// latency + (N - 1) * ii: a pipelined operation starts a pass every ii cycles
// and only the last one's latency is left over. So an accumulation of three
// taps with latency 2 and ii 1 is timed at four. Without a latency every step
// of a chain reads as simultaneous, and the register file write is activated
// in the same cycle as the reads feeding the DPU, sampling the output before
// there is one. A rop with no `ii` is taken to have ii = latency; `delay` is
// the older name for `latency`.
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

    // Not commutative: %a is in0 and %b is in1, so endpoints in @replace must
    // keep %a on slot offset 0.
    func.func @subt(%a: i16, %b: i16) -> i16
        attributes {benefit = 10 : i32} {
      %c = arith.subi %a, %b : i16
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

    // conf.mode 13 (max_min_acc) has no pattern yet, so it is never selected.

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
      drra.rop {conf = {mode = 0 : i32},
                uses = ["conf_reg:0"]} : () -> ()
      return
    }

    func.func @add(%a: i16, %b: i16) -> i16 {
      %r = drra.rop %a, %b {conf = {mode = 1 : i32},
                           latency = 1 : i32,
                           ii = 1 : i32,
                           endpoints = {results = [0 : i32],
                                        operands = [0 : i32, 1 : i32]},
                           uses = ["conf_reg:0", "input_narrow:0",
                                   "input_narrow:1", "output_narrow:1"]}
          : (i16, i16) -> i16
      return %r : i16
    }

    func.func @subt(%a: i16, %b: i16) -> i16 {
      %r = drra.rop %a, %b {conf = {mode = 4 : i32},
                           latency = 1 : i32,
                           ii = 1 : i32,
                           endpoints = {results = [0 : i32],
                                        operands = [0 : i32, 1 : i32]},
                           uses = ["conf_reg:0", "input_narrow:0",
                                   "input_narrow:1", "output_narrow:1"]}
          : (i16, i16) -> i16
      return %r : i16
    }

    func.func @mult(%a: i16, %b: i16) -> i16 {
      %r = drra.rop %a, %b {conf = {mode = 7 : i32},
                           latency = 1 : i32,
                           ii = 1 : i32,
                           endpoints = {results = [0 : i32],
                                        operands = [0 : i32, 1 : i32]},
                           uses = ["conf_reg:0", "input_narrow:0",
                                   "input_narrow:1", "output_narrow:1"]}
          : (i16, i16) -> i16
      return %r : i16
    }

    // %acc is unused: the accumulate register is not an instruction operand.
    // Latency 2, ii 1: the RTL registers the product before the accumulate add
    // (rtl/logic.sv.j2), so a pass's sum is in out0 a cycle later than the
    // single-cycle modes', while a new pass still starts every cycle.
    //
    // TODO: occupancy. Stage 2 of a window's last pass holds the adder, the
    // accumulator and out0 for latency - ii cycles after that pass, so the DPU
    // is busy for (N - 1) * ii + latency cycles, not N * ii, and a different
    // mode issued in the cycle right after a MAC window is dropped by the
    // hardware. The compiler's sharing check (`uses`, vesyla ConflictGraph)
    // knows nothing of that tail. It is unreachable today only because every
    // arithmetic mode claims conf_reg:0, so no two of them share a DPU.
    func.func @mac(%acc: memref<i16>, %a: i16, %b: i16) -> i16 {
      %r = drra.rop %a, %b {conf = {mode = 10 : i32},
                           latency = 2 : i32,
                           ii = 1 : i32,
                           endpoints = {results = [0 : i32],
                                        operands = [0 : i32, 1 : i32]},
                           uses = ["conf_reg:0", "input_narrow:0",
                                   "input_narrow:1", "output_narrow:1"]}
          : (i16, i16) -> i16
      return %r : i16
    }

    // An evt on the rst port, on its own timing pattern. It still produces the
    // value the loop carries in.
    func.func @rst(%acc: memref<i16>) -> i16 {
      %r = drra.rop {evt = {port = 1 : i32},
                    uses = ["agu:1"]} : () -> i16
      return %r : i16
    }
  }
}
