#ifndef _DPU_Q88_OPERATIONS_H
#define _DPU_Q88_OPERATIONS_H

#include "dpu_q88_pkg.h"
#include <climits>
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <vector>

// Forward declaration
class Dpu_q88;

namespace Dpu_Q88_Operations {

std::function<void()> getDPUHandler(Dpu_q88 *dpu,
                                    DPU_Q88_PKG::DPU_MODE mode);
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

constexpr int Q88_FRAC_BITS = 8;

inline int16_t sat16(int64_t value) {
  if (value > INT16_MAX) {
    return INT16_MAX;
  }
  if (value < INT16_MIN) {
    return INT16_MIN;
  }
  return static_cast<int16_t>(value);
}

inline int32_t sat32(int64_t value) {
  if (value > INT32_MAX) {
    return INT32_MAX;
  }
  if (value < INT32_MIN) {
    return INT32_MIN;
  }
  return static_cast<int32_t>(value);
}

inline int16_t vectorToInt16(const std::vector<uint8_t> &data) {
  if (data.empty()) {
    return 0;
  }

  uint16_t value = 0;
  const size_t n = data.size() < 2 ? data.size() : 2;
  for (size_t i = 0; i < n; ++i) {
    value |= static_cast<uint16_t>(data[i]) << (i * 8);
  }

  return static_cast<int16_t>(value);
}

inline std::vector<uint8_t> int16ToVector(int16_t data) {
  std::vector<uint8_t> result(2);
  uint16_t value = static_cast<uint16_t>(data);
  result[0] = static_cast<uint8_t>(value & 0xFF);
  result[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
  return result;
}

inline int32_t vectorToInt32(const std::vector<uint8_t> &data) {
  if (data.empty()) {
    return 0;
  }

  uint32_t value = 0;
  const size_t n = data.size() < 4 ? data.size() : 4;
  for (size_t i = 0; i < n; ++i) {
    value |= static_cast<uint32_t>(data[i]) << (i * 8);
  }

  return static_cast<int32_t>(value);
}

inline std::vector<uint8_t> int32ToVector(int32_t data) {
  std::vector<uint8_t> result(4);
  uint32_t value = static_cast<uint32_t>(data);
  for (size_t i = 0; i < 4; ++i) {
    result[i] = static_cast<uint8_t>((value >> (i * 8)) & 0xFF);
  }
  return result;
}

inline int16_t add_q88_sat(int16_t a, int16_t b) {
  return sat16(static_cast<int32_t>(a) + static_cast<int32_t>(b));
}

inline int16_t sub_q88_sat(int16_t a, int16_t b) {
  return sat16(static_cast<int32_t>(a) - static_cast<int32_t>(b));
}

inline int16_t mul_q88_sat(int16_t a, int16_t b) {
  int64_t product = static_cast<int64_t>(a) * static_cast<int64_t>(b);
  int64_t q88 = product >> Q88_FRAC_BITS;
  return sat16(q88);
}

} // namespace DPU_Operations

#endif // _DPU_Q88_OPERATIONS_H
