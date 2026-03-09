#ifndef _DPU_OPERATIONS_H
#define _DPU_OPERATIONS_H

#include "dpu_pkg.h"
#include <cstdint>
#include <functional>

// Forward declaration
class Dpu;

namespace DPU_Operations {

std::function<void()> getDPUHandler(Dpu *dpu, DPU_PKG::DPU_MODE mode);
std::unordered_map<DPU_PKG::DPU_MODE, std::function<void()>>
createHandlers(Dpu *dpu);

// Individual operation implementations
namespace Impl {
void handleIdle(Dpu *dpu);
void handleAdd(Dpu *dpu);
void handleAddConst(Dpu *dpu);
void handleSubt(Dpu *dpu);
void handleSubtAbs(Dpu *dpu);
void handleMult(Dpu *dpu);
void handleMultConst(Dpu *dpu);
void handleLoadIR(Dpu *dpu);
void handleMAC(Dpu *dpu);
} // namespace Impl

// Utility functions for arithmetic operations
inline int64_t add_sat(int64_t a, int64_t b) {
  if (a > 0) {
    if (b > INT64_MAX - a) {
      return INT64_MAX;
    }
  } else if (b < INT64_MIN - a) {
    return INT64_MIN;
  }
  return a + b;
}

inline int64_t round_shift(int64_t val, int frac) {
  if (frac <= 0)
    return val;

  int64_t half = INT64_C(1) << (frac - 1);
  int64_t rounded = val + (val >= 0 ? half : -half);
  return rounded >> frac;
}

inline int64_t mul_sat(int64_t a, int64_t b, size_t bitwidth, uint32_t frac) {
  const int64_t MAX_RESULT = (INT64_C(1) << (bitwidth - 1)) - 1;
  const int64_t MIN_RESULT = -(INT64_C(1) << (bitwidth - 1));

  if (a == 0 || b == 0)
    return 0;

  if (a > 0) {
    if (b > 0) {
      if (a > INT64_MAX / b) {
        return MAX_RESULT;
      }
    } else {
      if (b < INT64_MIN / a) {
        return MIN_RESULT;
      }
    }
  } else {
    if (b > 0) {
      if (a < INT64_MIN / b) {
        return MIN_RESULT;
      }
    } else {
      if (a != INT64_MIN && b < INT64_MAX / a) {
        return MAX_RESULT;
      }
    }
  }

  int64_t product = a * b;
  int64_t result = round_shift(product, frac);

  if (result > MAX_RESULT) {
    return MAX_RESULT;
  } else if (result < MIN_RESULT) {
    return MIN_RESULT;
  }

  return result;
}
} // namespace DPU_Operations

#endif // _DPU_OPERATIONS_H
