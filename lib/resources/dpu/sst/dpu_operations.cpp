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
      {DPU_PKG::CONF_MODE::CONF_MODE_ADD_CONST,
       [dpu] { Impl::handleAddConst(dpu); }},
      {DPU_PKG::CONF_MODE::CONF_MODE_SUBT, [dpu] { Impl::handleSubt(dpu); }},
      {DPU_PKG::CONF_MODE::CONF_MODE_SUBT_ABS,
       [dpu] { Impl::handleSubtAbs(dpu); }},
      {DPU_PKG::CONF_MODE::CONF_MODE_MULT, [dpu] { Impl::handleMult(dpu); }},
      {DPU_PKG::CONF_MODE::CONF_MODE_MULT_CONST,
       [dpu] { Impl::handleMultConst(dpu); }},
      {DPU_PKG::CONF_MODE::CONF_MODE_LD_IR, [dpu] { Impl::handleLoadIR(dpu); }},
      {DPU_PKG::CONF_MODE::CONF_MODE_MAC, [dpu] { Impl::handleMAC(dpu); }}};
}

namespace Impl {

void handleIdle(Dpu *dpu) {
  // Access through public interface - assuming DPU has an 'out' member
  // accessible You might need to add a getOutput() method to DPU class
  // dpu->getOutput().output("IDLE\n");
}

void handleAdd(Dpu *dpu) {
  dpu->handleOperation("ADD",
                       [](int64_t a, int64_t b) { return add_sat(a, b); });
}

void handleAddConst(Dpu *dpu) {
  dpu->handleOperation("ADD_CONST",
                       [](int64_t a, int64_t b) { return add_sat(a, b); });
}

void handleSubt(Dpu *dpu) {
  dpu->handleOperation("SUBT",
                       [](int64_t a, int64_t b) { return add_sat(a, -b); });
}

void handleSubtAbs(Dpu *dpu) {
  dpu->handleOperation("SUBT_ABS",
                       [](int64_t a, int64_t b) { return add_sat(a, -b); });
}

void handleMult(Dpu *dpu) {
  dpu->handleOperation("MULT", [dpu](int64_t a, int64_t b) {
    return mul_sat(a, b, dpu->getWordBitwidth(), dpu->fractional_bitwidth);
  });
}

void handleMultConst(Dpu *dpu) {
  dpu->handleOperation("MULT_CONST", [dpu](int64_t a, int64_t b) {
    return mul_sat(a, b, dpu->getWordBitwidth(), dpu->fractional_bitwidth);
  });
}

void handleLoadIR(Dpu *dpu) {
  dpu->handleOperation("LD_IR", [](int64_t a, int64_t b) {
    return b; // Load immediate to register
  });
}

void handleMAC(Dpu *dpu) {
  dpu->handleOperation("MAC", [dpu](int64_t a, int64_t b) {
    auto &acc_reg = dpu->getAccumulateRegister();

    // Fixed-point MAC. mul_sat rounds (a*b) >> fractional_bitwidth and clamps
    // to the word bitwidth (= sat16 of the product, matching the RTL
    // multiplier's saturate=1); add_sat then accumulates with saturation
    // (mirroring dpu.sv.j2's adder). At fractional_bitwidth=0 this reduces to
    // the plain integer sat16(a*b) MAC.
    int64_t result = add_sat(
        dpu->vectorToInt64(acc_reg),
        mul_sat(a, b, dpu->getWordBitwidth(), dpu->fractional_bitwidth));
    acc_reg = dpu->int64ToVector(result);
    return result;
  });
}

} // namespace Impl
} // namespace DPU_Operations
