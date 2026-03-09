#ifndef _VPU_OPERATIONS_H
#define _VPU_OPERATIONS_H

#include "vpu_pkg.h"
#include <cstdint>
#include <functional>

// Forward declaration
class Vpu;

namespace VPU_Operations {

std::function<void()> getVPUHandler(Vpu *vpu, VPU_PKG::VPU_MODE mode);
std::unordered_map<VPU_PKG::VPU_MODE, std::function<void()>>
createHandlers(Vpu *vpu);

// Individual operation implementations
namespace Impl {
void handle_idle(Vpu *vpu);
void handle_add(Vpu *vpu);
void handle_add_acc(Vpu *vpu);
void handle_sub(Vpu *vpu);
void handle_sub_acc(Vpu *vpu);
void handle_mul(Vpu *vpu);
void handle_mac(Vpu *vpu);
void handle_min(Vpu *vpu);
void handle_max(Vpu *vpu);
void handle_lshift(Vpu *vpu);
void handle_rshift(Vpu *vpu);
void handle_seq(Vpu *vpu);
void handle_sne(Vpu *vpu);
void handle_slt(Vpu *vpu);
void handle_sle(Vpu *vpu);
void handle_sgt(Vpu *vpu);
void handle_sge(Vpu *vpu);
void handle_and(Vpu *vpu);
void handle_or(Vpu *vpu);
void handle_xor(Vpu *vpu);
void handle_vadd(Vpu *vpu);
void handle_vadd_acc(Vpu *vpu);
void handle_vsub(Vpu *vpu);
void handle_vsub_acc(Vpu *vpu);
void handle_vmul(Vpu *vpu);
void handle_vmac(Vpu *vpu);
void handle_vmin(Vpu *vpu);
void handle_vmax(Vpu *vpu);
void handle_vlshift(Vpu *vpu);
void handle_vrshift(Vpu *vpu);
void handle_vseq(Vpu *vpu);
void handle_vsne(Vpu *vpu);
void handle_vslt(Vpu *vpu);
void handle_vsle(Vpu *vpu);
void handle_vsgt(Vpu *vpu);
void handle_vsge(Vpu *vpu);
void handle_vand(Vpu *vpu);
void handle_vor(Vpu *vpu);
void handle_vxor(Vpu *vpu);
void handle_vnot(Vpu *vpu);
void handle_add_imm(Vpu *vpu);
void handle_sub_imm(Vpu *vpu);
void handle_mul_imm(Vpu *vpu);
void handle_mac_imm(Vpu *vpu);
void handle_min_imm(Vpu *vpu);
void handle_max_imm(Vpu *vpu);
void handle_lshift_imm(Vpu *vpu);
void handle_rshift_imm(Vpu *vpu);
void handle_seq_imm(Vpu *vpu);
void handle_sne_imm(Vpu *vpu);
void handle_slt_imm(Vpu *vpu);
void handle_sle_imm(Vpu *vpu);
void handle_sgt_imm(Vpu *vpu);
void handle_sge_imm(Vpu *vpu);
void handle_and_imm(Vpu *vpu);
void handle_or_imm(Vpu *vpu);
void handle_xor_imm(Vpu *vpu);
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
} // namespace VPU_Operations

#endif // _VPU_OPERATIONS_H
