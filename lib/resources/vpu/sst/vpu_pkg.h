#ifndef _VPU_PKG_H
#define _VPU_PKG_H

#include "instruction.h"
#include <cstdint>
#include <functional>
#include <unordered_map>

class Vpu;

namespace VPU_PKG {

// Supported opcodes from ISA
enum OpCode { VPU = 4, EVT = 0, REP = 1, REPX = 2, TRANS = 3 };

enum VPU_INSTR_VPU {
  VPU_INSTR_VPU_CONFIG_BITWIDTH = 2,
  VPU_INSTR_VPU_MODE_BITWIDTH = 6,
  VPU_INSTR_VPU_IMMEDIATE_BITWIDTH = 16
};
enum VPU_INSTR_EVT { VPU_INSTR_EVT_PORT_BITWIDTH = 1 };
enum VPU_INSTR_REP {
  VPU_INSTR_REP_PORT_BITWIDTH = 1,
  VPU_INSTR_REP_ITER_BITWIDTH = 8,
  VPU_INSTR_REP_STEP_BITWIDTH = 7,
  VPU_INSTR_REP_DELAY_BITWIDTH = 8
};
enum VPU_INSTR_REPX {
  VPU_INSTR_REPX_PORT_BITWIDTH = 1,
  VPU_INSTR_REPX_ITER_BITWIDTH = 8,
  VPU_INSTR_REPX_STEP_BITWIDTH = 7,
  VPU_INSTR_REPX_DELAY_BITWIDTH = 8
};
enum VPU_INSTR_TRANS {
  VPU_INSTR_TRANS_PORT_BITWIDTH = 1,
  VPU_INSTR_TRANS_DELAY_BITWIDTH = 23
};

// ISA segment definitions
inline const std::unordered_map<OpCode, std::vector<SegmentRange>> &
getIsaDefinitions() {
  static const std::unordered_map<OpCode, std::vector<SegmentRange>>
      segmentsDef = {
          {OpCode::VPU,
           {SegmentRange("config", 2, 22), SegmentRange("mode", 6, 16),
            SegmentRange("immediate", 16, 0)}},
          {OpCode::EVT, {SegmentRange("port", 1, 23)}},
          {OpCode::REP,
           {SegmentRange("port", 1, 23), SegmentRange("iter", 8, 15),
            SegmentRange("step", 7, 8), SegmentRange("delay", 8, 0)}},
          {OpCode::REPX,
           {SegmentRange("port", 1, 23), SegmentRange("iter", 8, 15),
            SegmentRange("step", 7, 8), SegmentRange("delay", 8, 0)}},
          {OpCode::TRANS,
           {SegmentRange("port", 1, 23), SegmentRange("delay", 23, 0)}}};
  return segmentsDef;
}

// ISA verbo mappings
enum VPU_MODE {
  VPU_MODE_IDLE = 0,
  VPU_MODE_ADD = 1,
  VPU_MODE_ADD_ACC = 2,
  VPU_MODE_SUB = 3,
  VPU_MODE_SUB_ACC = 4,
  VPU_MODE_MUL = 5,
  VPU_MODE_MAC = 6,
  VPU_MODE_MIN = 7,
  VPU_MODE_MAX = 8,
  VPU_MODE_LSHIFT = 9,
  VPU_MODE_RSHIFT = 10,
  VPU_MODE_SEQ = 11,
  VPU_MODE_SNE = 12,
  VPU_MODE_SLT = 13,
  VPU_MODE_SLE = 14,
  VPU_MODE_SGT = 15,
  VPU_MODE_SGE = 16,
  VPU_MODE_AND = 17,
  VPU_MODE_OR = 18,
  VPU_MODE_XOR = 19,
  VPU_MODE_VADD = 20,
  VPU_MODE_VADD_ACC = 21,
  VPU_MODE_VSUB = 22,
  VPU_MODE_VSUB_ACC = 23,
  VPU_MODE_VMUL = 24,
  VPU_MODE_VMAC = 25,
  VPU_MODE_VMIN = 26,
  VPU_MODE_VMAX = 27,
  VPU_MODE_VLSHIFT = 28,
  VPU_MODE_VRSHIFT = 29,
  VPU_MODE_VSEQ = 30,
  VPU_MODE_VSNE = 31,
  VPU_MODE_VSLT = 32,
  VPU_MODE_VSLE = 33,
  VPU_MODE_VSGT = 34,
  VPU_MODE_VSGE = 35,
  VPU_MODE_VAND = 36,
  VPU_MODE_VOR = 37,
  VPU_MODE_VXOR = 38,
  VPU_MODE_VNOT = 39,
  VPU_MODE_ADD_IMM = 40,
  VPU_MODE_MUL_IMM = 41,
  VPU_MODE_MIN_IMM = 42,
  VPU_MODE_MAX_IMM = 43,
  VPU_MODE_LSHIFT_IMM = 44,
  VPU_MODE_RSHIFT_IMM = 45,
  VPU_MODE_SEQ_IMM = 46,
  VPU_MODE_SNE_IMM = 47,
  VPU_MODE_SLT_IMM = 48,
  VPU_MODE_SLE_IMM = 49,
  VPU_MODE_SGT_IMM = 50,
  VPU_MODE_SGE_IMM = 51,
  VPU_MODE_AND_IMM = 52,
  VPU_MODE_OR_IMM = 53,
  VPU_MODE_XOR_IMM = 54
};
enum EVT_PORT { EVT_PORT_VPU = 0, EVT_PORT_RST = 1 };
enum REP_PORT { REP_PORT_VPU = 0, REP_PORT_RST = 1 };
enum REPX_PORT { REPX_PORT_VPU = 0, REPX_PORT_RST = 1 };
enum TRANS_PORT { TRANS_PORT_VPU = 0, TRANS_PORT_RST = 1 };

// Instruction formats
struct VPUInstruction {
  uint32_t slot, config, mode, immediate;

  VPUInstruction(const Instruction &instr) {
    slot = instr.slot;
    config = instr.get("config").value;
    mode = instr.get("mode").value;
    immediate = instr.get("immediate").value;
  }
};
struct EVTInstruction {
  uint32_t slot, port;

  EVTInstruction(const Instruction &instr) {
    slot = instr.slot;
    port = instr.get("port").value;
  }
};
struct REPInstruction {
  uint32_t slot, port, iter, step, delay;

  REPInstruction(const Instruction &instr) {
    slot = instr.slot;
    port = instr.get("port").value;
    iter = instr.get("iter").value;
    step = instr.get("step").value;
    delay = instr.get("delay").value;
  }
};
struct REPXInstruction {
  uint32_t slot, port, iter, step, delay;

  REPXInstruction(const Instruction &instr) {
    slot = instr.slot;
    port = instr.get("port").value;
    iter = instr.get("iter").value;
    step = instr.get("step").value;
    delay = instr.get("delay").value;
  }
};
struct TRANSInstruction {
  uint32_t slot, port, delay;

  TRANSInstruction(const Instruction &instr) {
    slot = instr.slot;
    port = instr.get("port").value;
    delay = instr.get("delay").value;
  }
};

std::unordered_map<uint32_t, std::function<void(uint32_t)>>
createInstructionHandlers(Vpu *vpu_obj);

} // namespace VPU_PKG

#endif // _VPU_PKG_H
