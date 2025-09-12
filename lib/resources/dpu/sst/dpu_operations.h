#ifndef _DPU_OPERATIONS_H
#define _DPU_OPERATIONS_H

#include "dpu_pkg.h"
#include <functional>

// Forward declaration
class DPU;

namespace DPU_Operations {

std::function<void()> getDPUHandler(DPU *dpu, DPU_PKG::DPU_MODE mode);
std::unordered_map<DPU_PKG::DPU_MODE, std::function<void()>>
createHandlers(DPU *dpu);

// Individual operation implementations
namespace Impl {
void handleIdle(DPU *dpu);
void handleAdd(DPU *dpu);
void handleAddConst(DPU *dpu);
void handleSubt(DPU *dpu);
void handleSubtAbs(DPU *dpu);
void handleMult(DPU *dpu);
void handleMultConst(DPU *dpu);
void handleLoadIR(DPU *dpu);
void handleMAC(DPU *dpu);
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

inline int64_t mul_sat(int64_t a, int64_t b) {
  if (a == 0 || b == 0) {
    return 0;
  }

  if (a > 0) {
    if (b > 0) {
      if (a > INT64_MAX / b) {
        return INT64_MAX;
      }
    } else {
      if (b < INT64_MIN / a) {
        return INT64_MIN;
      }
    }
  } else {
    if (b > 0) {
      if (a < INT64_MIN / b) {
        return INT64_MIN;
      }
    } else {
      if (a != INT64_MIN && b < INT64_MAX / a) {
        return INT64_MAX;
      }
    }
  }

  return a * b;
}
} // namespace DPU_Operations

#endif // _DPU_OPERATIONS_H
