#include "dpu_operations.h"
#include "dpu.h"

namespace DPU_Operations {

std::function<void()> getDPUHandler(DPU *dpu, DPU_PKG::DPU_MODE mode) {
  static auto handlers = createHandlers(dpu);
  if (handlers.find(mode) != handlers.end()) {
    return handlers[mode];
  } else {
    dpu->out.fatal(CALL_INFO, -1, "Unsupported DPU mode: %d\n", mode);
    return [] {}; // This line will never be reached due to fatal exit
  }
}

std::unordered_map<DPU_PKG::DPU_MODE, std::function<void()>>
createHandlers(DPU *dpu) {
  return {{DPU_PKG::IDLE, [dpu] { Impl::handleIdle(dpu); }},
          {DPU_PKG::ADD, [dpu] { Impl::handleAdd(dpu); }},
          {DPU_PKG::ADD_CONST, [dpu] { Impl::handleAddConst(dpu); }},
          {DPU_PKG::SUBT, [dpu] { Impl::handleSubt(dpu); }},
          {DPU_PKG::SUBT_ABS, [dpu] { Impl::handleSubtAbs(dpu); }},
          {DPU_PKG::MULT, [dpu] { Impl::handleMult(dpu); }},
          {DPU_PKG::MULT_CONST, [dpu] { Impl::handleMultConst(dpu); }},
          {DPU_PKG::LD_IR, [dpu] { Impl::handleLoadIR(dpu); }},
          {DPU_PKG::MAC, [dpu] { Impl::handleMAC(dpu); }}};
}

namespace Impl {

void handleIdle(DPU *dpu) {
  // Access through public interface - assuming DPU has an 'out' member
  // accessible You might need to add a getOutput() method to DPU class
  // dpu->getOutput().output("IDLE\n");
}

void handleAdd(DPU *dpu) {
  dpu->handleOperation("ADD",
                       [](int64_t a, int64_t b) { return add_sat(a, b); });
}

void handleAddConst(DPU *dpu) {
  dpu->handleOperation("ADD_CONST",
                       [](int64_t a, int64_t b) { return add_sat(a, b); });
}

void handleSubt(DPU *dpu) {
  dpu->handleOperation("SUBT",
                       [](int64_t a, int64_t b) { return add_sat(a, -b); });
}

void handleSubtAbs(DPU *dpu) {
  dpu->handleOperation("SUBT_ABS",
                       [](int64_t a, int64_t b) { return add_sat(a, -b); });
}

void handleMult(DPU *dpu) {
  dpu->handleOperation("MULT",
                       [](int64_t a, int64_t b) { return mul_sat(a, b); });
}

void handleMultConst(DPU *dpu) {
  dpu->handleOperation("MULT_CONST",
                       [](int64_t a, int64_t b) { return mul_sat(a, b); });
}

void handleLoadIR(DPU *dpu) {
  dpu->handleOperation("LD_IR", [](int64_t a, int64_t b) {
    return b; // Load immediate to register
  });
}

void handleMAC(DPU *dpu) {
  dpu->handleOperation("MAC", [dpu](int64_t a, int64_t b) {
    // Note: This might need adjustment based on how you want to handle
    // the accumulate register access. You might need to add more public
    // methods to the DPU class.
    auto &acc_reg = dpu->getAccumulateRegister();

    // This assumes vectorToInt64 and int64ToVector are accessible
    // You might need to move these to dpu_pkg.h or make them public
    int64_t result = add_sat(dpu->vectorToInt64(acc_reg), mul_sat(a, b));
    acc_reg = dpu->int64ToVector(result);
    return result;
  });
}

} // namespace Impl
} // namespace DPU_Operations
