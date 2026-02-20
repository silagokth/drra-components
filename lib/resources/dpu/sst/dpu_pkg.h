#ifndef _DPU_PKG_H
#define _DPU_PKG_H

#include "instruction.h"
#include <cstdint>
#include <functional>
#include <unordered_map>

class Dpu;

namespace DPU_PKG {

// Supported opcodes from ISA
enum OpCode { EVT = 0, REP = 1, REPX = 2, TRANS = 3, DPU = 4 };

enum DPU_ISNTR_EVT {
  DPU_INSTR_EVT_PORT_BITWIDTH = 1,
};

enum DPU_INSTR_REP {
  DPU_INSTR_REP_PORT_BITWIDTH = 1,
  DPU_INSTR_REP_ITER_BITWIDTH = 8,
  DPU_INSTR_REP_STEP_BITWIDTH = 7,
  DPU_INSTR_REP_DELAY_BITWIDTH = 8
};

enum DPU_INSTR_REPX {
  DPU_INSTR_REPX_PORT_BITWIDTH = 1,
  DPU_INSTR_REPX_ITER_BITWIDTH = 8,
  DPU_INSTR_REPX_STEP_BITWIDTH = 1,
  DPU_INSTR_REPX_DELAY_BITWIDTH = 8
};

enum DPU_INSTR_TRANS {
  DPU_INSTR_TRANS_PORT_BITWIDTH = 1,
  DPU_INSTR_TRANS_DELAY_BITWIDTH = 23,
};

enum DPU_INSTR_DPU {
  DPU_INSTR_DPU_CONFIG_BITWIDTH = 2,
  DPU_INSTR_DPU_MODE_BITWIDTH = 5,
  DPU_INSTR_DPU_IMMEDIATE_BITWIDTH = 16
};

// ISA segment definitions
inline const std::unordered_map<OpCode, std::vector<SegmentRange>> &
getIsaDefinitions() {
  uint32_t INSTR_PAYLOAD_BITWIDTH = 24;
  static const std::unordered_map<OpCode, std::vector<SegmentRange>>
      segmentsDef = {
          {OpCode::EVT,
           {
               SegmentRange("port", DPU_INSTR_EVT_PORT_BITWIDTH,
                            INSTR_PAYLOAD_BITWIDTH -
                                DPU_INSTR_EVT_PORT_BITWIDTH),
           }},
          {OpCode::REP,
           {SegmentRange("port", DPU_INSTR_REP_PORT_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH - DPU_INSTR_REP_PORT_BITWIDTH),
            SegmentRange("iter", DPU_INSTR_REP_ITER_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH - DPU_INSTR_REP_PORT_BITWIDTH -
                             DPU_INSTR_REP_ITER_BITWIDTH),
            SegmentRange("step", DPU_INSTR_REP_STEP_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH - DPU_INSTR_REP_PORT_BITWIDTH -
                             DPU_INSTR_REP_ITER_BITWIDTH -
                             DPU_INSTR_REP_STEP_BITWIDTH),
            SegmentRange("delay", DPU_INSTR_REP_DELAY_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH - DPU_INSTR_REP_PORT_BITWIDTH -
                             DPU_INSTR_REP_ITER_BITWIDTH -
                             DPU_INSTR_REP_STEP_BITWIDTH -
                             DPU_INSTR_REP_DELAY_BITWIDTH)}},
          {OpCode::REPX,
           {SegmentRange("port", DPU_INSTR_REPX_PORT_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH - DPU_INSTR_REPX_PORT_BITWIDTH),
            SegmentRange("iter", DPU_INSTR_REPX_ITER_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH - DPU_INSTR_REPX_PORT_BITWIDTH -
                             DPU_INSTR_REPX_ITER_BITWIDTH),
            SegmentRange("step", DPU_INSTR_REPX_STEP_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH - DPU_INSTR_REPX_PORT_BITWIDTH -
                             DPU_INSTR_REPX_ITER_BITWIDTH -
                             DPU_INSTR_REPX_STEP_BITWIDTH),
            SegmentRange("delay", DPU_INSTR_REPX_DELAY_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH - DPU_INSTR_REPX_PORT_BITWIDTH -
                             DPU_INSTR_REPX_ITER_BITWIDTH -
                             DPU_INSTR_REPX_STEP_BITWIDTH -
                             DPU_INSTR_REPX_DELAY_BITWIDTH)}},
          {OpCode::TRANS,
           {SegmentRange("port", DPU_INSTR_TRANS_PORT_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH -
                             DPU_INSTR_TRANS_PORT_BITWIDTH),
            SegmentRange("delay", DPU_INSTR_TRANS_DELAY_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH -
                             DPU_INSTR_TRANS_PORT_BITWIDTH -
                             DPU_INSTR_TRANS_DELAY_BITWIDTH)}},
          {OpCode::DPU,
           {SegmentRange("config", DPU_INSTR_DPU_CONFIG_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH -
                             DPU_INSTR_DPU_CONFIG_BITWIDTH),
            SegmentRange("mode", DPU_INSTR_DPU_MODE_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH -
                             DPU_INSTR_DPU_CONFIG_BITWIDTH -
                             DPU_INSTR_DPU_MODE_BITWIDTH),
            SegmentRange("immediate", DPU_INSTR_DPU_IMMEDIATE_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH -
                             DPU_INSTR_DPU_CONFIG_BITWIDTH -
                             DPU_INSTR_DPU_MODE_BITWIDTH -
                             DPU_INSTR_DPU_IMMEDIATE_BITWIDTH)}},
      };
  return segmentsDef;
}

// ISA verbo mappings
enum EVT_PORT {
  EVT_PORT_DPU = 0,
  EVT_PORT_RST = 1,
};

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

struct DPUInstruction {
  uint32_t slot, config, mode, immediate;

  DPUInstruction(const Instruction &instr) {
    slot = instr.slot;
    config = instr.get("config").value;
    mode = instr.get("mode").value;
    immediate = instr.get("immediate").value;
  }
};

std::unordered_map<uint32_t, std::function<void(uint32_t)>>
createInstructionHandlers(Dpu *dpu_obj);

} // namespace DPU_PKG

#endif // _DPU_PKG_H
