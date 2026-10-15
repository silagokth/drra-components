#include "dpu_operations.h"
#include "dpu.h"

namespace DPU_Operations {

std::function<void()> getDPUHandler(Dpu *dpu, DPU_PKG::CONF_MODE mode) {
  // NOTE: must NOT be `static` — the handler closures capture `dpu`, so a
  // function-local static would permanently bind every DPU instance to the
  // first one constructed (wrong-instance bug, and unsafe under SST threads).
  // Called once per config instruction, so rebuilding the small map is cheap.
  auto handlers = DPU_Operations::createHandlers(dpu);
  if (handlers.find(mode) != handlers.end()) {
    return handlers[mode];
  } else {
    dpu->out.fatal(CALL_INFO, -1, "Unsupported DPU mode: %d\n", mode);
    return [] {}; // This line will never be reached due to fatal exit
  }
}

std::unordered_map<DPU_PKG::CONF_MODE, std::function<void()>>
createHandlers(Dpu *dpu) {
  return {
      {DPU_PKG::CONF_MODE::CONF_MODE_IDLE, [dpu] { Impl::handleIdle(dpu); }},
      {DPU_PKG::CONF_MODE::CONF_MODE_ADD, [dpu] { Impl::handleAdd(dpu); }},
      {DPU_PKG::CONF_MODE::CONF_MODE_SUBT, [dpu] { Impl::handleSubt(dpu); }},
      {DPU_PKG::CONF_MODE::CONF_MODE_MULT, [dpu] { Impl::handleMult(dpu); }},
      {DPU_PKG::CONF_MODE::CONF_MODE_MAC, [dpu] { Impl::handleMAC(dpu); }},
      {DPU_PKG::CONF_MODE::CONF_MODE_MAX_MIN_ACC,
       [dpu] { Impl::handleMaxAcc(dpu); }}};
}

namespace Impl {

void handleIdle(Dpu *dpu) {
  // Access through public interface - assuming DPU has an 'out' member
  // accessible You might need to add a getOutput() method to DPU class
  // dpu->getOutput().output("IDLE\n");
}

void handleAdd(Dpu *dpu) {
  dpu->handleOperation("ADD", [dpu](int64_t a, int64_t b) {
    return dpu->saturate ? add_sat(a, b) : wrap(a + b, dpu->getWordBitwidth());
  });
}

void handleSubt(Dpu *dpu) {
  // in0 - in1, mirroring logic.sv.j2 CONF_MODE_SUBT. The operands are
  // word-sized, so -b cannot overflow; int64ToVector clamps the result to the
  // word like the RTL adder's saturation.
  dpu->handleOperation("SUBT", [dpu](int64_t a, int64_t b) {
    return dpu->saturate ? add_sat(a, -b) : wrap(a - b, dpu->getWordBitwidth());
  });
}

void handleMult(Dpu *dpu) {
  dpu->handleOperation("MULT", [dpu](int64_t a, int64_t b) {
    // Second output: the product's upper word (RTL out1 / word_data_out_1)
    dpu->sendOut1(
        mul_high(a, b, dpu->getWordBitwidth(), dpu->fractional_bitwidth));
    return dpu->saturate
               ? mul_sat(a, b, dpu->getWordBitwidth(), dpu->fractional_bitwidth)
               : mul_wrap(a, b, dpu->getWordBitwidth(), dpu->fractional_bitwidth);
  });
}

void handleMAC(Dpu *dpu) {
  // Stage 1 of a two-cycle MAC (logic.sv.j2). mul_sat rounds (a*b) >>
  // fractional_bitwidth and clamps to the word bitwidth (= sat16 of the
  // product, matching the RTL multiplier's saturate=1); Dpu::finishMAC adds it
  // to the accumulator with saturation next cycle (mirroring the RTL adder).
  // At fractional_bitwidth=0 this reduces to the plain integer sat16(a*b) MAC.
  // With SATURATE = 0 both steps wrap.
  int64_t a, b;
  dpu->readOperands(a, b);
  size_t bw = dpu->getWordBitwidth();
  int64_t product = dpu->saturate
                        ? mul_sat(a, b, bw, dpu->fractional_bitwidth)
                        : mul_wrap(a, b, bw, dpu->fractional_bitwidth);
  dpu->out.output("DPU MAC stage 1 (in0=%ld, in1=%ld, product=%ld)\n", a, b,
                  product);
  dpu->issueMACProduct(product);
}

void handleMaxAcc(Dpu *dpu) {
  // Accumulating max, mirroring logic.sv.j2 CONF_MODE_MAX_MIN_ACC: acc = max(acc,
  // in0), in1 ignored. The accumulator shares the MAC register, so EVT_PORT_RST
  // clears it and an empty register reads as 0: the reset value is 0 and a window
  // yields max(0, x1..xn) (== max(x1..xn) for non-negative input). vectorToInt64
  // sign-extends, so the compare is signed like the RTL's.
  // handleOperation still requires BOTH data buffers to be non-empty, so the
  // schedule must keep in1 driven (the max-pool emitter keeps the kernel read).
  dpu->handleOperation("MAX_MIN_ACC", [dpu](int64_t a, int64_t /*b*/) {
    auto &acc_reg = dpu->getAccumulateRegister();
    int64_t acc = dpu->vectorToInt64(acc_reg);
    int64_t result = (a > acc) ? a : acc;
    acc_reg = dpu->int64ToVector(result);
    return result;
  });
}

} // namespace Impl
} // namespace DPU_Operations
