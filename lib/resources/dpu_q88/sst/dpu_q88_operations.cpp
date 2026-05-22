#include "dpu_q88_operations.h"
#include "dpu_q88.h"

namespace Dpu_Q88_Operations {

std::function<void()> getDPUHandler(Dpu_q88 *dpu, DPU_Q88_PKG::DPU_MODE mode) {
  auto handlers = Dpu_Q88_Operations::createHandlers(dpu);
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
      {DPU_Q88_PKG::DPU_MODE::DPU_MODE_LD_IR, [dpu] { Impl::handleLoadIR(dpu); }},
      {DPU_Q88_PKG::DPU_MODE::DPU_MODE_MAC, [dpu] { Impl::handleMAC(dpu); }}};
}

namespace Impl {

void handleIdle(Dpu_q88 *dpu) {
  (void)dpu;
}

void handleAdd(Dpu_q88 *dpu) {
  dpu->handleOperation("ADD", [](int16_t a, int16_t b) {
    return add_q88_sat(a, b);
  });
}

void handleAddConst(Dpu_q88 *dpu) {
  dpu->handleOperation("ADD_CONST", [](int16_t a, int16_t b) {
    return add_q88_sat(a, b);
  });
}

void handleSubt(Dpu_q88 *dpu) {
  dpu->handleOperation("SUBT", [](int16_t a, int16_t b) {
    return sub_q88_sat(a, b);
  });
}

void handleSubtAbs(Dpu_q88 *dpu) {
  dpu->handleOperation("SUBT_ABS", [](int16_t a, int16_t b) {
    return sub_q88_sat(a, b);
  });
}

void handleMult(Dpu_q88 *dpu) {
  dpu->handleOperation("MULT", [](int16_t a, int16_t b) {
    return mul_q88_sat(a, b);
  });
}

void handleMultConst(Dpu_q88 *dpu) {
  dpu->handleOperation("MULT_CONST", [](int16_t a, int16_t b) {
    return mul_q88_sat(a, b);
  });
}

void handleLoadIR(Dpu_q88 *dpu) {
  dpu->handleOperation("LD_IR", [](int16_t a, int16_t b) {
    (void)a;
    return b;
  });
}

void handleMAC(Dpu_q88 *dpu) {
  dpu->handleOperation("MAC", [dpu](int16_t a, int16_t b) {
    int32_t product_q88 =
        static_cast<int32_t>(
            (static_cast<int64_t>(a) * static_cast<int64_t>(b)) >>
            Q88_FRAC_BITS);

    auto &acc_reg = dpu->getAccumulateRegister();
    int32_t acc = vectorToInt32(acc_reg);

    int64_t acc_next_64 = static_cast<int64_t>(acc) + product_q88;
    int32_t acc_next = sat32(acc_next_64);

    acc_reg = int32ToVector(acc_next);

    return sat16(acc_next);
  });
}

} // namespace Impl
} // namespace DPU_Operations
