#ifndef _DPU_PKG_H
#define _DPU_PKG_H

#include "instruction.h"

#include <climits>
#include <functional>
#include <map>

class DPU;

namespace DPU_PKG {

// Supported opcodes from ISA
enum OpCode { REP = 0, REPX = 1, FSM = 2, DPU_OP = 3 };

// DPU operation modes
enum DPU_MODE {
  IDLE = 0,
  ADD = 1,
  SUM_ACC = 2,
  ADD_CONST = 3,
  SUBT = 4,
  SUBT_ABS = 5,
  MODE_6 = 6,
  MULT = 7,
  MULT_ADD = 8,
  MULT_CONST = 9,
  MAC = 10,
  LD_IR = 11,
  AXPY = 12,
  MAX_MIN_ACC = 13,
  MAX_MIN_CONST = 14,
  MODE_15 = 15,
  MAX_MIN = 16,
  SHIFT_L = 17,
  SHIFT_R = 18,
  SIGM = 19,
  TANHYP = 20,
  EXPON = 21,
  LK_RELU = 22,
  RELU = 23,
  DIV = 24,
  ACC_SOFTMAX = 25,
  DIV_SOFTMAX = 26,
  LD_ACC = 27,
  SCALE_DW = 28,
  SCALE_UP = 29,
  MAC_INTER = 30,
  MODE_31 = 31
};

// ISA segment definitions
inline const std::map<OpCode, std::vector<SegmentRange>> &getIsaDefinitions() {
  static const std::map<OpCode, std::vector<SegmentRange>> segmentsDef = {
      {OpCode::REP,
       {SegmentRange("port", 2, 22), SegmentRange("level", 4, 18),
        SegmentRange("iter", 6, 12), SegmentRange("step", 6, 6),
        SegmentRange("delay", 6, 0)}},
      {OpCode::REPX,
       {SegmentRange("port", 2, 22), SegmentRange("level", 4, 18),
        SegmentRange("iter_msb", 6, 12), SegmentRange("step_msb", 6, 6),
        SegmentRange("delay_msb", 6, 0)}},
      {OpCode::FSM,
       {SegmentRange("port", 3, 21), SegmentRange("delay_0", 7, 14),
        SegmentRange("delay_1", 7, 7), SegmentRange("delay_2", 7, 0)}},
      {OpCode::DPU_OP,
       {SegmentRange("option", 2, 22), SegmentRange("mode", 5, 17),
        SegmentRange("immediate", 16, 1)}}};
  return segmentsDef;
}

// Instruction structures
struct REPInstruction {
  uint32_t slot, port, level, iter, step, delay;

  REPInstruction(const Instruction &instr) {
    slot = instr.slot;
    port = instr.get("port").value;
    level = instr.get("level").value;
    iter = instr.get("iter").value;
    step = instr.get("step").value;
    delay = instr.get("delay").value;
  }
};

struct REPXInstruction {
  uint32_t slot, port, level, iter_msb, step_msb, delay_msb;

  REPXInstruction(const Instruction &instr) {
    slot = instr.slot;
    port = instr.get("port").value;
    level = instr.get("level").value;
    iter_msb = instr.get("iter_msb").value;
    step_msb = instr.get("step_msb").value;
    delay_msb = instr.get("delay_msb").value;
  }
};

struct FSMInstruction {
  uint32_t slot, port, delay_0, delay_1, delay_2;

  FSMInstruction(const Instruction &instr) {
    slot = instr.slot;
    port = instr.get("port").value;
    delay_0 = instr.get("delay_0").value;
    delay_1 = instr.get("delay_1").value;
    delay_2 = instr.get("delay_2").value;
  }
};

struct DPUInstruction {
  uint32_t slot, option, mode;
  uint64_t immediate;

  DPUInstruction(const Instruction &instr) {
    slot = instr.slot;
    option = instr.get("option").value;
    mode = instr.get("mode").value;
    immediate = instr.get("immediate").value;
  }
};

std::unordered_map<uint32_t, std::function<void(uint32_t)>>
createInstructionHandlers(DPU *dpu);

} // namespace DPU_PKG

#endif // _DPU_PKG_H
