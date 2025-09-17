#include "dpu_operations.h"
#include "dpu_2cycle_mac.h"

namespace DPU_Operations {

std::function<void()> getDPUHandler(Dpu_2cycle_mac *dpu,
                                    DPU_2CYCLE_MAC_PKG::DPU_MODE mode) {
  static auto handlers = DPU_Operations::createHandlers(dpu);
  if (handlers.find(mode) != handlers.end()) {
    return handlers[mode];
  } else {
    dpu->out.fatal(CALL_INFO, -1, "Unsupported DPU mode: %d\n", mode);
    return [] {}; // This line will never be reached due to fatal exit
  }
}

std::unordered_map<DPU_2CYCLE_MAC_PKG::DPU_MODE, std::function<void()>>
createHandlers(Dpu_2cycle_mac *dpu) {
  return {{DPU_2CYCLE_MAC_PKG::DPU_MODE::DPU_MODE_IDLE,
           [dpu] { Impl::handleIdle(dpu); }},
          {DPU_2CYCLE_MAC_PKG::DPU_MODE::DPU_MODE_ADD,
           [dpu] { Impl::handleAdd(dpu); }},
          {DPU_2CYCLE_MAC_PKG::DPU_MODE::DPU_MODE_ADD_CONST,
           [dpu] { Impl::handleAddConst(dpu); }},
          {DPU_2CYCLE_MAC_PKG::DPU_MODE::DPU_MODE_SUBT,
           [dpu] { Impl::handleSubt(dpu); }},
          {DPU_2CYCLE_MAC_PKG::DPU_MODE::DPU_MODE_SUBT_ABS,
           [dpu] { Impl::handleSubtAbs(dpu); }},
          {DPU_2CYCLE_MAC_PKG::DPU_MODE::DPU_MODE_MULT,
           [dpu] { Impl::handleMult(dpu); }},
          {DPU_2CYCLE_MAC_PKG::DPU_MODE::DPU_MODE_MULT_CONST,
           [dpu] { Impl::handleMultConst(dpu); }},
          {DPU_2CYCLE_MAC_PKG::DPU_MODE::DPU_MODE_LD_IR,
           [dpu] { Impl::handleLoadIR(dpu); }},
          {DPU_2CYCLE_MAC_PKG::DPU_MODE::DPU_MODE_MAC,
           [dpu] { Impl::handleMAC(dpu); }}};
}

namespace Impl {

void handleIdle(Dpu_2cycle_mac *dpu) {
  // Access through public interface - assuming DPU has an 'out' member
  // accessible You might need to add a getOutput() method to DPU class
  // dpu->getOutput().output("IDLE\n");
}

void handleAdd(Dpu_2cycle_mac *dpu) {
  dpu->handleOperation("ADD",
                       [](int64_t a, int64_t b) { return add_sat(a, b); });
}

void handleAddConst(Dpu_2cycle_mac *dpu) {
  dpu->handleOperation("ADD_CONST",
                       [](int64_t a, int64_t b) { return add_sat(a, b); });
}

void handleSubt(Dpu_2cycle_mac *dpu) {
  dpu->handleOperation("SUBT",
                       [](int64_t a, int64_t b) { return add_sat(a, -b); });
}

void handleSubtAbs(Dpu_2cycle_mac *dpu) {
  dpu->handleOperation("SUBT_ABS",
                       [](int64_t a, int64_t b) { return add_sat(a, -b); });
}

void handleMult(Dpu_2cycle_mac *dpu) {
  dpu->handleOperation("MULT",
                       [](int64_t a, int64_t b) { return mul_sat(a, b); });
}

void handleMultConst(Dpu_2cycle_mac *dpu) {
  dpu->handleOperation("MULT_CONST",
                       [](int64_t a, int64_t b) { return mul_sat(a, b); });
}

void handleLoadIR(Dpu_2cycle_mac *dpu) {
  dpu->handleOperation("LD_IR", [](int64_t a, int64_t b) {
    return b; // Load immediate to register
  });
}

void handleMAC(Dpu_2cycle_mac *dpu) {
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
