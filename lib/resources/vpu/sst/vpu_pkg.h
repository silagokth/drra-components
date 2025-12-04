#ifndef _VPU_PKG_H
#define _VPU_PKG_H

#include "instruction.h"
#include <cstdint>
#include <functional>
#include <unordered_map>

class Vpu;

namespace VPU_PKG {

// Supported opcodes from ISA
enum OpCode {
    VPU = 3,    REP = 0,    TRANS = 2};

// ISA segment definitions
inline const std::unordered_map<OpCode, std::vector<SegmentRange>> &getIsaDefinitions() {
  static const std::unordered_map<OpCode, std::vector<SegmentRange>> segmentsDef = {
    {OpCode::VPU, {
      SegmentRange("option", 2, 22),      SegmentRange("mode", 5, 17),      SegmentRange("immediate", 16, 1)    }},    {OpCode::REP, {
      SegmentRange("port", 2, 22),      SegmentRange("level", 4, 18),      SegmentRange("iter", 6, 12),      SegmentRange("step", 6, 6),      SegmentRange("delay", 6, 0)    }},    {OpCode::TRANS, {
      SegmentRange("port", 2, 22),      SegmentRange("delay", 12, 10),      SegmentRange("level", 4, 6)    }}  };
  return segmentsDef;
}

// ISA verbo mappings
enum VPU_MODE {
  VPU_MODE_IDLE = 0,  VPU_MODE_ADD = 1,  VPU_MODE_SUM_ACC = 2,  VPU_MODE_ADD_IMM = 3,  VPU_MODE_SUBT = 4,  VPU_MODE_MULT = 7,  VPU_MODE_MULT_ADD = 8,  VPU_MODE_MULT_IMM = 9,  VPU_MODE_MAC = 10,  VPU_MODE_LD_ACC = 27,  VPU_MODE_MAC_IMM = 30};
enum REP_PORT {
  REP_PORT_INPUT0 = 0,  REP_PORT_INPUT1 = 1,  REP_PORT_MODE = 2};

// Instruction formats
struct VPUInstruction {
  uint32_t slot, option, mode, immediate;

  VPUInstruction(const Instruction &instr) {
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
struct TRANSInstruction {
  uint32_t slot, port, delay, level;

  TRANSInstruction(const Instruction &instr) {
    slot = instr.slot;
    port = instr.get("port").value;
    delay = instr.get("delay").value;
    level = instr.get("level").value;
  }
};

std::unordered_map<uint32_t, std::function<void(uint32_t)>>
createInstructionHandlers(Vpu *vpu_obj);

} // namespace VPU_PKG

#endif // _VPU_PKG_H