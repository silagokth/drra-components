#ifndef _DPU_PKG_H
#define _DPU_PKG_H

#include "instruction.h"
#include <cstdint>
#include <functional>
#include <unordered_map>

class Dpu;

namespace DPU_PKG {

// Supported opcodes from ISA
enum OpCode { DPU = 3, REP = 0, REPX = 1, FSM = 2 };

// ISA segment definitions
inline const std::unordered_map<OpCode, std::vector<SegmentRange>> &
getIsaDefinitions() {
  static const std::unordered_map<OpCode, std::vector<SegmentRange>>
      segmentsDef = {
          {OpCode::DPU,
           {SegmentRange("option", 2, 22), SegmentRange("mode", 5, 17),
            SegmentRange("immediate", 16, 1)}},
          {OpCode::REP,
           {SegmentRange("port", 2, 22), SegmentRange("level", 4, 18),
            SegmentRange("iter", 6, 12), SegmentRange("step", 6, 6),
            SegmentRange("delay", 6, 0)}},
          {OpCode::REPX,
           {SegmentRange("port", 2, 22), SegmentRange("level", 4, 18),
            SegmentRange("iter", 6, 12), SegmentRange("step", 6, 6),
            SegmentRange("delay", 6, 0)}},
          {OpCode::FSM,
           {SegmentRange("port", 2, 22), SegmentRange("delay_0", 7, 15),
            SegmentRange("delay_1", 7, 8), SegmentRange("delay_2", 7, 1)}}};
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
enum REP_PORT {
  REP_PORT_READ_NARROW = 0,
  REP_PORT_READ_WIDE = 1,
  REP_PORT_WRITE_NARROW = 2,
  REP_PORT_WRITE_WIDE = 3
};
enum REPX_PORT {
  REPX_PORT_READ_NARROW = 0,
  REPX_PORT_READ_WIDE = 1,
  REPX_PORT_WRITE_NARROW = 2,
  REPX_PORT_WRITE_WIDE = 3
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
  uint32_t slot, port, level, iter, step, delay;

  REPXInstruction(const Instruction &instr) {
    slot = instr.slot;
    port = instr.get("port").value;
    level = instr.get("level").value;
    iter = instr.get("iter").value;
    step = instr.get("step").value;
    delay = instr.get("delay").value;
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

std::unordered_map<uint32_t, std::function<void(uint32_t)>>
createInstructionHandlers(Dpu *dpu_obj);

} // namespace DPU_PKG

#endif // _DPU_PKG_H
