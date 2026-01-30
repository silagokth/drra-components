#ifndef _DPU_PKG_H
#define _DPU_PKG_H

#include "instruction.h"
#include <cstdint>
#include <functional>
#include <unordered_map>

class Dpu;

namespace DPU_PKG {

// Supported opcodes from ISA
enum OpCode { DPU = 3, REP = 0, REPX = 1, TRANS = 2 };

enum DPU_INSTR_DPU {
  DPU_INSTR_DPU_OPTION_BITWIDTH = 2,
  DPU_INSTR_DPU_MODE_BITWIDTH = 5,
  DPU_INSTR_DPU_IMMEDIATE_BITWIDTH = 16
};

enum DPU_INSTR_REP {
  DPU_INSTR_REP_ITER_BITWIDTH = 8,
  DPU_INSTR_REP_STEP_BITWIDTH = 8,
  DPU_INSTR_REP_DELAY_BITWIDTH = 8
};

enum DPU_INSTR_REPX {
  DPU_INSTR_REPX_ITER_BITWIDTH = 8,
  DPU_INSTR_REPX_STEP_BITWIDTH = 8,
  DPU_INSTR_REPX_DELAY_BITWIDTH = 8
};

enum DPU_INSTR_TRANS {
  DPU_INSTR_TRANS_DELAY_BITWIDTH = 24,
};

// ISA segment definitions
inline const std::unordered_map<OpCode, std::vector<SegmentRange>> &
getIsaDefinitions() {
  uint32_t INSTR_PAYLOAD_BITWIDTH = 24;
  static const std::unordered_map<OpCode, std::vector<SegmentRange>>
      segmentsDef = {
          {OpCode::DPU,
           {SegmentRange("option", DPU_INSTR_DPU_OPTION_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH -
                             DPU_INSTR_DPU_OPTION_BITWIDTH),
            SegmentRange("mode", DPU_INSTR_DPU_MODE_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH -
                             DPU_INSTR_DPU_OPTION_BITWIDTH -
                             DPU_INSTR_DPU_MODE_BITWIDTH),
            SegmentRange("immediate", DPU_INSTR_DPU_IMMEDIATE_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH -
                             DPU_INSTR_DPU_OPTION_BITWIDTH -
                             DPU_INSTR_DPU_MODE_BITWIDTH -
                             DPU_INSTR_DPU_IMMEDIATE_BITWIDTH)}},
          {OpCode::REP,
           {SegmentRange("iter", DPU_INSTR_REP_ITER_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH - DPU_INSTR_REP_ITER_BITWIDTH),
            SegmentRange("step", DPU_INSTR_REP_STEP_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH - DPU_INSTR_REP_ITER_BITWIDTH -
                             DPU_INSTR_REP_STEP_BITWIDTH),
            SegmentRange("delay", DPU_INSTR_REP_DELAY_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH - DPU_INSTR_REP_ITER_BITWIDTH -
                             DPU_INSTR_REP_STEP_BITWIDTH -
                             DPU_INSTR_REP_DELAY_BITWIDTH)}},
          {OpCode::REPX,
           {SegmentRange("iter", DPU_INSTR_REPX_ITER_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH - DPU_INSTR_REPX_ITER_BITWIDTH),
            SegmentRange("step", DPU_INSTR_REPX_STEP_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH - DPU_INSTR_REPX_ITER_BITWIDTH -
                             DPU_INSTR_REPX_STEP_BITWIDTH),
            SegmentRange("delay", DPU_INSTR_REPX_DELAY_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH - DPU_INSTR_REPX_ITER_BITWIDTH -
                             DPU_INSTR_REPX_STEP_BITWIDTH -
                             DPU_INSTR_REPX_DELAY_BITWIDTH)}},
          {OpCode::TRANS,
           {SegmentRange("delay", DPU_INSTR_TRANS_DELAY_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH -
                             DPU_INSTR_TRANS_DELAY_BITWIDTH)}}};
  return segmentsDef;
}

// ISA verbo mappings
enum DPU_MODE {
  DPU_MODE_IDLE = 0,
  DPU_MODE_ADD = 1,
  DPU_MODE_SUM_ACC = 2,
  DPU_MODE_ADD_CONST = 3,
  DPU_MODE_SUBT = 4,
  DPU_MODE_SUBT_ABS = 5,
  DPU_MODE_MODE_6 = 6,
  DPU_MODE_MULT = 7,
  DPU_MODE_MULT_ADD = 8,
  DPU_MODE_MULT_CONST = 9,
  DPU_MODE_MAC = 10,
  DPU_MODE_LD_IR = 11,
  DPU_MODE_AXPY = 12,
  DPU_MODE_MAX_MIN_ACC = 13,
  DPU_MODE_MAX_MIN_CONST = 14,
  DPU_MODE_MODE_15 = 15,
  DPU_MODE_MAX_MIN = 16,
  DPU_MODE_SHIFT_L = 17,
  DPU_MODE_SHIFT_R = 18,
  DPU_MODE_SIGM = 19,
  DPU_MODE_TANHYP = 20,
  DPU_MODE_EXPON = 21,
  DPU_MODE_LK_RELU = 22,
  DPU_MODE_RELU = 23,
  DPU_MODE_DIV = 24,
  DPU_MODE_ACC_SOFTMAX = 25,
  DPU_MODE_DIV_SOFTMAX = 26,
  DPU_MODE_LD_ACC = 27,
  DPU_MODE_SCALE_DW = 28,
  DPU_MODE_SCALE_UP = 29,
  DPU_MODE_MAC_INTER = 30,
  DPU_MODE_MODE_31 = 31
};

// Instruction formats
struct DPUInstruction {
  uint32_t slot, option, mode, immediate;

  DPUInstruction(const Instruction &instr) {
    slot = instr.slot;
    option = instr.get("option").value;
    mode = instr.get("mode").value;
    immediate = instr.get("immediate").value;
  }
};
struct REPInstruction {
  uint32_t slot, iter, step, delay;

  REPInstruction(const Instruction &instr) {
    slot = instr.slot;
    iter = instr.get("iter").value;
    step = instr.get("step").value;
    delay = instr.get("delay").value;
  }
};
struct REPXInstruction {
  uint32_t slot, iter, step, delay;

  REPXInstruction(const Instruction &instr) {
    slot = instr.slot;
    iter = instr.get("iter").value;
    step = instr.get("step").value;
    delay = instr.get("delay").value;
  }
};
struct TRANSInstruction {
  uint32_t slot, delay;

  TRANSInstruction(const Instruction &instr) {
    slot = instr.slot;
    delay = instr.get("delay").value;
  }
};

std::unordered_map<uint32_t, std::function<void(uint32_t)>>
createInstructionHandlers(Dpu *dpu_obj);

} // namespace DPU_PKG

#endif // _DPU_PKG_H
