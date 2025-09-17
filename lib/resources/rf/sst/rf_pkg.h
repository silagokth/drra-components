#ifndef _RF_PKG_H
#define _RF_PKG_H

#include "instruction.h"
#include <cstdint>
#include <functional>
#include <unordered_map>

class Rf;

namespace RF_PKG {

// Supported opcodes from ISA
enum OpCode { DSU = 6, REP = 0, REPX = 1 };

// ISA segment definitions
inline const std::unordered_map<OpCode, std::vector<SegmentRange>> &
getIsaDefinitions() {
  static const std::unordered_map<OpCode, std::vector<SegmentRange>>
      segmentsDef = {
          {OpCode::DSU,
           {SegmentRange("init_addr_sd", 1, 23),
            SegmentRange("init_addr", 16, 7), SegmentRange("port", 2, 5)}},
          {OpCode::REP,
           {SegmentRange("port", 2, 22), SegmentRange("level", 4, 18),
            SegmentRange("iter", 6, 12), SegmentRange("step", 6, 6),
            SegmentRange("delay", 6, 0)}},
          {OpCode::REPX,
           {SegmentRange("port", 2, 22), SegmentRange("level", 4, 18),
            SegmentRange("iter", 6, 12), SegmentRange("step", 6, 6),
            SegmentRange("delay", 6, 0)}}};
  return segmentsDef;
}

// ISA verbo mappings
enum DSU_INIT_ADDR_SD { DSU_INIT_ADDR_SD_S = 0, DSU_INIT_ADDR_SD_D = 1 };
enum DSU_PORT {
  DSU_PORT_WRITE_TO_RF_WORD = 0,
  DSU_PORT_READ_FROM_RF_WORD = 1,
  DSU_PORT_WRITE_TO_RF_BULK = 2,
  DSU_PORT_READ_FROM_RF_BULK = 3
};
enum REP_PORT {
  REP_PORT_WRITE_TO_RF_WORD = 0,
  REP_PORT_READ_FROM_RF_WORD = 1,
  REP_PORT_WRITE_TO_RF_BULK = 2,
  REP_PORT_READ_FROM_RF_BULK = 3
};
enum REPX_PORT {
  REPX_PORT_WRITE_TO_RF_WORD = 0,
  REPX_PORT_READ_FROM_RF_WORD = 1,
  REPX_PORT_WRITE_TO_RF_BULK = 2,
  REPX_PORT_READ_FROM_RF_BULK = 3
};

// Instruction formats
struct DSUInstruction {
  uint32_t slot, init_addr_sd, init_addr, port;

  DSUInstruction(const Instruction &instr) {
    slot = instr.slot;
    init_addr_sd = instr.get("init_addr_sd").value;
    init_addr = instr.get("init_addr").value;
    port = instr.get("port").value;
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

std::unordered_map<uint32_t, std::function<void(uint32_t)>>
createInstructionHandlers(Rf *rf_obj);

} // namespace RF_PKG

#endif // _RF_PKG_H
