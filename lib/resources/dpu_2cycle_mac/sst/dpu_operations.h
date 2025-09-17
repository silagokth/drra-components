#ifndef _DPU_OPERATIONS_H
#define _DPU_OPERATIONS_H

#include "dpu_2cycle_mac_pkg.h"
#include <functional>

// Forward declaration
class Dpu_2cycle_mac;

namespace DPU_Operations {

std::function<void()> getDPUHandler(Dpu_2cycle_mac *dpu,
                                    DPU_2CYCLE_MAC_PKG::DPU_MODE mode);
std::unordered_map<DPU_2CYCLE_MAC_PKG::DPU_MODE, std::function<void()>>
createHandlers(Dpu_2cycle_mac *dpu);

// Individual operation implementations
namespace Impl {
void handleIdle(Dpu_2cycle_mac *dpu);
void handleAdd(Dpu_2cycle_mac *dpu);
void handleAddConst(Dpu_2cycle_mac *dpu);
void handleSubt(Dpu_2cycle_mac *dpu);
void handleSubtAbs(Dpu_2cycle_mac *dpu);
void handleMult(Dpu_2cycle_mac *dpu);
void handleMultConst(Dpu_2cycle_mac *dpu);
void handleLoadIR(Dpu_2cycle_mac *dpu);
void handleMAC(Dpu_2cycle_mac *dpu);
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
