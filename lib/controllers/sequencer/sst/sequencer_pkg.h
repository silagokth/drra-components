#ifndef _SEQUENCER_PKG_H
#define _SEQUENCER_PKG_H

#include "instruction.h"
#include <cstdint>
#include <functional>
#include <unordered_map>

class Sequencer;

namespace SEQUENCER_PKG {

// Supported opcodes from ISA
enum OpCode { HALT = 0, WAIT = 1, ACT = 2, CALC = 3, BRN = 4 };

// ISA segment definitions
inline const std::unordered_map<OpCode, std::vector<SegmentRange>> &
getIsaDefinitions() {
  static const std::unordered_map<OpCode, std::vector<SegmentRange>>
      segmentsDef = {
          {OpCode::HALT, {}},
          {OpCode::WAIT,
           {SegmentRange("mode", 1, 27), SegmentRange("cycle", 27, 0)}},
          {OpCode::ACT,
           {SegmentRange("ports", 16, 12), SegmentRange("mode", 4, 8),
            SegmentRange("param", 8, 0)}},
          {OpCode::CALC,
           {SegmentRange("mode", 6, 22), SegmentRange("operand1", 4, 18),
            SegmentRange("operand2_sd", 1, 17), SegmentRange("operand2", 8, 9),
            SegmentRange("result", 4, 5)}},
          {OpCode::BRN,
           {SegmentRange("reg", 4, 24), SegmentRange("target_true", 9, 15),
            SegmentRange("target_false", 9, 6)}}};
  return segmentsDef;
}

// ISA verbo mappings
enum ACT_MODE {
  ACT_MODE_CONTINUOUS_PORT = 0,
  ACT_MODE_ALL_PORT_X = 1,
  ACT_MODE_ACTIVATION_VECTOR = 2
};

enum CALC_MODE {
  CALC_MODE_IDLE = 0,
  CALC_MODE_ADD = 1,
  CALC_MODE_SUB = 2,
  CALC_MODE_LLS = 3,
  CALC_MODE_LRS = 4,
  CALC_MODE_MUL = 5,
  CALC_MODE_DIV = 6,
  CALC_MODE_MOD = 7,
  CALC_MODE_BITAND = 8,
  CALC_MODE_BITOR = 9,
  CALC_MODE_BITINV = 10,
  CALC_MODE_BITXOR = 11,
  CALC_MODE_EQ = 17,
  CALC_MODE_NE = 18,
  CALC_MODE_GT = 19,
  CALC_MODE_GE = 20,
  CALC_MODE_LT = 21,
  CALC_MODE_LE = 22,
  CALC_MODE_ADDH = 23,
  CALC_MODE_AND = 32,
  CALC_MODE_OR = 33,
  CALC_MODE_NOT = 34
};
enum CALC_OPERAND2_SD { CALC_OPERAND2_SD_S = 0, CALC_OPERAND2_SD_D = 1 };

// Instruction formats
struct HALTInstruction {
  uint32_t slot;

  HALTInstruction(const Instruction &instr) { slot = instr.slot; }
};
struct WAITInstruction {
  uint32_t slot, mode, cycle;

  WAITInstruction(const Instruction &instr) {
    slot = instr.slot;
    mode = instr.get("mode").value;
    cycle = instr.get("cycle").value;
  }
};
struct ACTInstruction {
  uint32_t slot, ports, mode, param;

  ACTInstruction(const Instruction &instr) {
    slot = instr.slot;
    ports = instr.get("ports").value;
    mode = instr.get("mode").value;
    param = instr.get("param").value;
  }
};
struct CALCInstruction {
  uint32_t slot, mode, operand1, operand2_sd, operand2, result;

  CALCInstruction(const Instruction &instr) {
    slot = instr.slot;
    mode = instr.get("mode").value;
    operand1 = instr.get("operand1").value;
    operand2_sd = instr.get("operand2_sd").value;
    operand2 = instr.get("operand2").value;
    result = instr.get("result").value;
  }
};
struct BRNInstruction {
  uint32_t slot, reg;
  int32_t target_true, target_false;

  BRNInstruction(const Instruction &instr) {
    slot = instr.slot;
    reg = instr.get("reg").value;

    // get length of target_true and target_false segments
    uint32_t targetTrueSegmentLength, targetFalseSegmentLength;
    for (const auto &seg : getIsaDefinitions().at(OpCode::BRN)) {
      if (seg.name == "target_true") {
        targetTrueSegmentLength = seg.bitwidth;
      } else if (seg.name == "target_false") {
        targetFalseSegmentLength = seg.bitwidth;
      }
    }
    target_true = instr.get("target_true").value;
    target_false = instr.get("target_false").value;

    // Sign extend targetTrue and targetFalse
    if (target_true & (1 << (targetTrueSegmentLength - 1))) {
      target_true |= ~((1 << targetTrueSegmentLength) - 1);
    }
    if (target_false & (1 << (targetFalseSegmentLength - 1))) {
      target_false |= ~((1 << targetFalseSegmentLength) - 1);
    }
  }
};

std::unordered_map<uint32_t, std::function<void(uint32_t)>>
createInstructionHandlers(Sequencer *sequencer_obj);

} // namespace SEQUENCER_PKG

#endif // _SEQUENCER_PKG_H
