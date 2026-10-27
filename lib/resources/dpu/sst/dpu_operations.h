#ifndef _DPU_OPERATIONS_H
#define _DPU_OPERATIONS_H

#include "dpu_pkg.h"
#include <cstdint>
#include <functional>

// Forward declaration
class Dpu;

namespace DPU_Operations {

std::function<void()> getDPUHandler(Dpu *dpu, DPU_PKG::CONF_MODE mode);
std::unordered_map<DPU_PKG::CONF_MODE, std::function<void()>>
createHandlers(Dpu *dpu);

// Individual operation implementations
namespace Impl {
void handleIdle(Dpu *dpu);
void handleAdd(Dpu *dpu);
void handleSubt(Dpu *dpu);
void handleMult(Dpu *dpu);
void handleMAC(Dpu *dpu);
void handleMaxAcc(Dpu *dpu);
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

  // Round half away from zero: round the magnitude (ties up), then restore the
  // sign. Biasing the signed value by -half for negatives and arithmetic-
  // shifting would double-round (the shift already floors toward -inf).
  int64_t half = INT64_C(1) << (frac - 1);
  int64_t mag = (val < 0) ? -val : val;
  int64_t r = (mag + half) >> frac;
  return (val < 0) ? -r : r;
}

// Two's-complement wrap of val to a signed bitwidth-bit word: the RTL with
// SATURATE = 0 keeps the low bits of the sum / rounded product.
inline int64_t wrap(int64_t val, size_t bitwidth) {
  if (bitwidth == 0 || bitwidth >= 64)
    return val;
  uint64_t mask = (UINT64_C(1) << bitwidth) - 1;
  uint64_t raw = static_cast<uint64_t>(val) & mask;
  if (raw & (UINT64_C(1) << (bitwidth - 1)))
    raw |= ~mask;
  return static_cast<int64_t>(raw);
}

// Product without saturation: rounded like mul_sat, then wrapped to the word.
// The operands are words, so a * b cannot overflow int64.
inline int64_t mul_wrap(int64_t a, int64_t b, size_t bitwidth, uint32_t frac) {
  return wrap(round_shift(a * b, frac), bitwidth);
}

// Upper word of the rounded product, mirroring the RTL multiplier's out_high:
// the bitwidth bits above the ones mul_sat / mul_wrap return, sign-extended.
inline int64_t mul_high(int64_t a, int64_t b, size_t bitwidth, uint32_t frac) {
  return wrap(round_shift(a * b, frac) >> bitwidth, bitwidth);
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
