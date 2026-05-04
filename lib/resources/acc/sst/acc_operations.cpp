#include "acc_operations.h"
#include "acc.h"
#include <numeric>

namespace ACC_Operations {

std::unordered_map<ACC_PKG::ACC_MODE, std::function<void()>>
createHandlers(Acc *acc, bool include_multiplier) {
  std::unordered_map<ACC_PKG::ACC_MODE, std::function<void()>> handlers = {
      {ACC_PKG::ACC_MODE::ACC_MODE_IDLE,  [acc] { Impl::handleIdle(acc); }},
      {ACC_PKG::ACC_MODE::ACC_MODE_ADD,   [acc] { Impl::handleAdd(acc); }},
      {ACC_PKG::ACC_MODE::ACC_MODE_SUB,   [acc] { Impl::handleSub(acc); }},
      {ACC_PKG::ACC_MODE::ACC_MODE_VADD,  [acc] { Impl::handleVadd(acc); }},
      {ACC_PKG::ACC_MODE::ACC_MODE_VSUB,  [acc] { Impl::handleVsub(acc); }},
  };
  if (include_multiplier) {
    handlers[ACC_PKG::ACC_MODE::ACC_MODE_CAPTURE]    = [acc] { Impl::handleCapture(acc); };
    handlers[ACC_PKG::ACC_MODE::ACC_MODE_VMAC_ADD]   = [acc] { Impl::handleVmacAdd(acc); };
    handlers[ACC_PKG::ACC_MODE::ACC_MODE_VMAC_SUB]   = [acc] { Impl::handleVmacSub(acc); };
    handlers[ACC_PKG::ACC_MODE::ACC_MODE_SQUARE_ADD] = [acc] { Impl::handleSquareAdd(acc); };
    handlers[ACC_PKG::ACC_MODE::ACC_MODE_SQUARE_SUB] = [acc] { Impl::handleSquareSub(acc); };
  }
  return handlers;
}

namespace Impl {

void handleIdle(Acc *acc) {}

void handleAdd(Acc *acc) {
  acc->handleOperation("add", [acc](int64_t input) {
    acc->setAccumulateRegister(acc->getAccumulateRegister() + input);
  });
}

void handleSub(Acc *acc) {
  acc->handleOperation("sub", [acc](int64_t input) {
    acc->setAccumulateRegister(acc->getAccumulateRegister() - input);
  });
}

void handleVadd(Acc *acc) {
  acc->handleVectorOperation("vadd", [acc](std::vector<int64_t> words) {
    int64_t sum = std::accumulate(words.begin(), words.end(), int64_t(0));
    acc->setAccumulateRegister(acc->getAccumulateRegister() + sum);
  });
}

void handleVsub(Acc *acc) {
  acc->handleVectorOperation("vsub", [acc](std::vector<int64_t> words) {
    int64_t sum = std::accumulate(words.begin(), words.end(), int64_t(0));
    acc->setAccumulateRegister(acc->getAccumulateRegister() - sum);
  });
}

void handleCapture(Acc *acc) {
  const auto &buf = acc->getDataBuffer(0);
  if (buf.empty()) {
    acc->getOutput().output(" ACC capture idle (no input data on slot 0)\n");
    return;
  }
  acc->setOperandRegister(acc->byteVectorToInt64Vector(buf));
  acc->getOutput().output(" ACC captured operand register (%zu words)\n",
                          acc->getOperandRegister().size());
}

void handleVmacAdd(Acc *acc) {
  const std::vector<int64_t> &operands = acc->getOperandRegister();
  acc->handleVectorOperation("vmac_add", [acc, &operands](std::vector<int64_t> words) {
    int64_t sum = 0;
    size_t n = std::min(operands.size(), words.size());
    for (size_t i = 0; i < n; i++)
      sum += operands[i] * words[i];
    acc->setAccumulateRegister(acc->getAccumulateRegister() + sum);
  });
}

void handleVmacSub(Acc *acc) {
  const std::vector<int64_t> &operands = acc->getOperandRegister();
  acc->handleVectorOperation("vmac_sub", [acc, &operands](std::vector<int64_t> words) {
    int64_t sum = 0;
    size_t n = std::min(operands.size(), words.size());
    for (size_t i = 0; i < n; i++)
      sum += operands[i] * words[i];
    acc->setAccumulateRegister(acc->getAccumulateRegister() - sum);
  });
}

void handleSquareAdd(Acc *acc) {
  acc->handleVectorOperation("square_add", [acc](std::vector<int64_t> words) {
    int64_t sum = 0;
    for (int64_t w : words)
      sum += w * w;
    acc->setAccumulateRegister(acc->getAccumulateRegister() + sum);
  });
}

void handleSquareSub(Acc *acc) {
  acc->handleVectorOperation("square_sub", [acc](std::vector<int64_t> words) {
    int64_t sum = 0;
    for (int64_t w : words)
      sum += w * w;
    acc->setAccumulateRegister(acc->getAccumulateRegister() - sum);
  });
}

} // namespace Impl

} // namespace ACC_Operations
