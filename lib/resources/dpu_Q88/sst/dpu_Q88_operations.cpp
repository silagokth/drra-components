#include "dpu_Q88_operations.h"
#include "dpu_Q88.h"
#include "dpu_Q88_pkg.h"

namespace DPU_Q88_Operations {

std::function<void()> getDPUHandler(Dpu_q88 *dpu, DPU_Q88_PKG::DPU_MODE mode) {
  static auto handlers = DPU_Q88_Operations::createHandlers(dpu);
  if (handlers.find(mode) != handlers.end()) {
    return handlers[mode];
  } else {
    dpu->out.fatal(CALL_INFO, -1, "Unsupported DPU mode: %d\n", mode);
    return [] {}; // This line will never be reached due to fatal exit
  }
}

std::unordered_map<DPU_Q88_PKG::DPU_MODE, std::function<void()>>
createHandlers(Dpu_q88 *dpu) {
  return {
      {DPU_Q88_PKG::DPU_MODE::DPU_MODE_IDLE, [dpu] { Impl::handleIdle(dpu); }},
      {DPU_Q88_PKG::DPU_MODE::DPU_MODE_ADD, [dpu] { Impl::handleAdd(dpu); }},
      {DPU_Q88_PKG::DPU_MODE::DPU_MODE_ADD_CONST,
       [dpu] { Impl::handleAddConst(dpu); }},
      {DPU_Q88_PKG::DPU_MODE::DPU_MODE_SUBT, [dpu] { Impl::handleSubt(dpu); }},
      {DPU_Q88_PKG::DPU_MODE::DPU_MODE_SUBT_ABS,
       [dpu] { Impl::handleSubtAbs(dpu); }},
      {DPU_Q88_PKG::DPU_MODE::DPU_MODE_MULT, [dpu] { Impl::handleMult(dpu); }},
      {DPU_Q88_PKG::DPU_MODE::DPU_MODE_MULT_CONST,
       [dpu] { Impl::handleMultConst(dpu); }},
      {DPU_Q88_PKG::DPU_MODE::DPU_MODE_LD_IR,
       [dpu] { Impl::handleLoadIR(dpu); }},
      {DPU_Q88_PKG::DPU_MODE::DPU_MODE_MAC, [dpu] { Impl::handleMAC(dpu); }}};
}

namespace Impl {

void handleIdle(Dpu_q88 *dpu) {
  // Access through public interface - assuming DPU has an 'out' member
  // accessible You might need to add a getOutput() method to DPU class
  // dpu->getOutput().output("IDLE\n");
}

void handleAdd(Dpu_q88 *dpu) {
  dpu->handleOperation("ADD",
                       [](int16_t a, int16_t b) { return add_q88_sat(a, b); });
}

void handleAddConst(Dpu_q88 *dpu) {
  dpu->handleOperation("ADD_CONST",
                       [](int16_t a, int16_t b) { return add_q88_sat(a, b); });
}

void handleSubt(Dpu_q88 *dpu) {
  dpu->handleOperation("SUBT",
                       [](int64_t a, int64_t b) { return add_sat(a, -b); });
}

void handleSubtAbs(Dpu_q88 *dpu) {
  dpu->handleOperation("SUBT_ABS",
                       [](int64_t a, int64_t b) { return add_sat(a, -b); });
}

void handleMult(Dpu_q88 *dpu) {
  dpu->handleOperation("MULT",
                       [](int64_t a, int64_t b) { return mul_sat(a, b); });
}

void handleMultConst(Dpu_q88 *dpu) {
  dpu->handleOperation("MULT_CONST",
                       [](int64_t a, int64_t b) { return mul_sat(a, b); });
}

void handleLoadIR(Dpu_q88 *dpu) {
  dpu->handleOperation("LD_IR", [](int64_t a, int64_t b) {
    return b; // Load immediate to register
  });
}

void handleMAC(Dpu_q88 *dpu) {
  dpu->handleOperation("MAC", [dpu](int16_t a, int16_t b) {
    int32_t p = (int32_t)a * (int32_t)b; // Q16.16
    p = p >> 8;
    p = p + dpu->getAccumulateRegister();
    // p = p + vectorToint32(accumulate_register);
    // accumulate_register = int32ToVector(p);
    dpu->getAccumulateRegister() = p;
    if (p > INT16_MAX)
      p = (int16_t)INT16_MAX;
    if (p < INT16_MIN)
      p = (int16_t)INT16_MIN;
    return (int16_t)p;
  });
}

} // namespace Impl
} // namespace DPU_Q88_Operations
