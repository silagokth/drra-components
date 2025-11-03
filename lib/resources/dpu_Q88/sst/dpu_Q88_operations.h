#ifndef _DPU_OPERATIONS_H
#define _DPU_OPERATIONS_H

#include "dpu_Q88_pkg.h"
#include <functional>

// Forward declaration
class Dpu_q88;

namespace DPU_Q88_Operations {

std::function<void()> getDPUHandler(Dpu_q88 *dpu, DPU_Q88_PKG::DPU_MODE mode);
std::unordered_map<DPU_Q88_PKG::DPU_MODE, std::function<void()>>
createHandlers(Dpu_q88 *dpu);

// Individual operation implementations
namespace Impl {
void handleIdle(Dpu_q88 *dpu);
void handleAdd(Dpu_q88 *dpu);
void handleAddConst(Dpu_q88 *dpu);
void handleSubt(Dpu_q88 *dpu);
void handleSubtAbs(Dpu_q88 *dpu);
void handleMult(Dpu_q88 *dpu);
void handleMultConst(Dpu_q88 *dpu);
void handleLoadIR(Dpu_q88 *dpu);
void handleMAC(Dpu_q88 *dpu);
} // namespace Impl

inline int16_t vectorToInt16(std::vector<uint8_t> data) {
  int16_t result = 0;
  for (size_t i = 0; i < 2; i++) {
    result |= data[i] << (i * 8);
  }
  return result;
}

inline std::vector<uint8_t> int16ToVector(int16_t data) {
  std::vector<uint8_t> result(2);
  uint16_t u = static_cast<uint16_t>(data);
  result[0] = u & 0xFF;
  result[1] = (u >> 8) & 0xFF;
  return result;
}

inline std::vector<uint8_t> int32ToVector(int32_t data) {
  std::vector<uint8_t> result(4);
  uint32_t u = static_cast<uint32_t>(data);
  for (size_t i = 0; i < 4; i++) {
    result[i] |= (u >> (i * 8)) & 0xFF;
  }
  return result;
}

inline int16_t add_q88_sat(int16_t a, int16_t b) {
  int32_t s = (int32_t)a + (int32_t)b;
  if (s > INT16_MAX)
    s = INT16_MAX;
  if (s < INT16_MIN)
    s = INT16_MIN;
  return (int16_t)s;
}

inline int16_t mul_q88_sat(int16_t a, int16_t b) {
  int32_t p = (int32_t)a * (int32_t)b; // Q16.16
  // rounding, p's going to >>8,
  // so +/- a 0x80 to Q16.16 to pre-decide the last bit after shifting to Q8.8
  //  if(p>=0){
  //    p = p + 128;
  //  } else {
  //    p = p - 128;
  //  }
  p = p >> 8;
  if (p > INT16_MAX)
    p = INT16_MAX;
  if (p < INT16_MIN)
    p = INT16_MIN;
  return (int16_t)p;
}

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
} // namespace DPU_Q88_Operations

#endif // _DPU_OPERATIONS_H
