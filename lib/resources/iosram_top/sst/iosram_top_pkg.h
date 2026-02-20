#ifndef _IOSRAM_TOP_PKG_H
#define _IOSRAM_TOP_PKG_H

#include "instruction.h"
#include <cstdint>
#include <functional>
#include <unordered_map>

class Iosram_top;

namespace IOSRAM_TOP_PKG {

// Supported opcodes from ISA
enum OpCode { DSU = 6, REP = 0, REPX = 1, TRANS = 2 };

enum IOSRAM_TOP_INSTR_DSU {
  IOSRAM_TOP_INSTR_DSU_OPTION_BITWIDTH = 2,
  IOSRAM_TOP_INSTR_DSU_PORT_BITWIDTH = 2,
  IOSRAM_TOP_INSTR_DSU_INIT_ADDR_SD_BITWIDTH = 1,
  IOSRAM_TOP_INSTR_DSU_INIT_ADDR_BITWIDTH = 16
};

enum IOSRAM_TOP_INSTR_REP {
  IOSRAM_TOP_INSTR_REP_PORT_BITWIDTH = 2,
  IOSRAM_TOP_INSTR_REP_ITER_BITWIDTH = 8,
  IOSRAM_TOP_INSTR_REP_STEP_BITWIDTH = 7,
  IOSRAM_TOP_INSTR_REP_DELAY_BITWIDTH = 7
};

enum IOSRAM_TOP_INSTR_REPX {
  IOSRAM_TOP_INSTR_REPX_PORT_BITWIDTH = 2,
  IOSRAM_TOP_INSTR_REPX_ITER_BITWIDTH = 8,
  IOSRAM_TOP_INSTR_REPX_STEP_BITWIDTH = 7,
  IOSRAM_TOP_INSTR_REPX_DELAY_BITWIDTH = 7
};

enum IOSRAM_TOP_INSTR_TRANS {
  IOSRAM_TOP_INSTR_TRANS_PORT_BITWIDTH = 2,
  IOSRAM_TOP_INSTR_TRANS_DELAY_BITWIDTH = 22,
};

// ISA segment definitions
inline const std::unordered_map<OpCode, std::vector<SegmentRange>> &
getIsaDefinitions() {
  uint32_t INSTR_PAYLOAD_BITWIDTH = 24;
  static const std::unordered_map<OpCode, std::vector<SegmentRange>>
      segmentsDef = {
          {OpCode::DSU,
           {SegmentRange("option", IOSRAM_TOP_INSTR_DSU_OPTION_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH -
                             IOSRAM_TOP_INSTR_DSU_OPTION_BITWIDTH),
            SegmentRange("port", IOSRAM_TOP_INSTR_DSU_PORT_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH -
                             IOSRAM_TOP_INSTR_DSU_OPTION_BITWIDTH -
                             IOSRAM_TOP_INSTR_DSU_PORT_BITWIDTH),
            SegmentRange(
                "init_addr_sd", IOSRAM_TOP_INSTR_DSU_INIT_ADDR_SD_BITWIDTH,
                INSTR_PAYLOAD_BITWIDTH - IOSRAM_TOP_INSTR_DSU_OPTION_BITWIDTH -
                    IOSRAM_TOP_INSTR_DSU_PORT_BITWIDTH -
                    IOSRAM_TOP_INSTR_DSU_INIT_ADDR_SD_BITWIDTH),
            SegmentRange("init_addr", IOSRAM_TOP_INSTR_DSU_INIT_ADDR_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH -
                             IOSRAM_TOP_INSTR_DSU_OPTION_BITWIDTH -
                             IOSRAM_TOP_INSTR_DSU_PORT_BITWIDTH -
                             IOSRAM_TOP_INSTR_DSU_INIT_ADDR_SD_BITWIDTH -
                             IOSRAM_TOP_INSTR_DSU_INIT_ADDR_BITWIDTH)}},
          {OpCode::REP,
           {SegmentRange("port", IOSRAM_TOP_INSTR_REP_PORT_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH -
                             IOSRAM_TOP_INSTR_REP_PORT_BITWIDTH),
            SegmentRange("iter", IOSRAM_TOP_INSTR_REP_ITER_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH -
                             IOSRAM_TOP_INSTR_REP_PORT_BITWIDTH -
                             IOSRAM_TOP_INSTR_REP_ITER_BITWIDTH),
            SegmentRange("step", IOSRAM_TOP_INSTR_REP_STEP_BITWIDTH,
                         IOSRAM_TOP_INSTR_REP_DELAY_BITWIDTH),
            SegmentRange("delay", IOSRAM_TOP_INSTR_REP_DELAY_BITWIDTH, 0)}},
          {OpCode::REPX,
           {SegmentRange("port", IOSRAM_TOP_INSTR_REPX_PORT_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH -
                             IOSRAM_TOP_INSTR_REPX_PORT_BITWIDTH),
            SegmentRange("iter", IOSRAM_TOP_INSTR_REPX_ITER_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH -
                             IOSRAM_TOP_INSTR_REPX_PORT_BITWIDTH -
                             IOSRAM_TOP_INSTR_REPX_ITER_BITWIDTH + 1),
            SegmentRange("step", IOSRAM_TOP_INSTR_REPX_STEP_BITWIDTH,
                         IOSRAM_TOP_INSTR_REPX_DELAY_BITWIDTH),
            SegmentRange("delay", IOSRAM_TOP_INSTR_REPX_DELAY_BITWIDTH, 0)}},
          {OpCode::TRANS,
           {SegmentRange("port", IOSRAM_TOP_INSTR_TRANS_PORT_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH -
                             IOSRAM_TOP_INSTR_TRANS_PORT_BITWIDTH),
            SegmentRange("delay", IOSRAM_TOP_INSTR_TRANS_DELAY_BITWIDTH, 0)}}};
  return segmentsDef;
}

// ISA verbo mappings
enum DSU_INIT_ADDR_SD { DSU_INIT_ADDR_SD_S = 0, DSU_INIT_ADDR_SD_D = 1 };
enum DSU_PORT {
  DSU_PORT_SRAM_READ_FROM_IO = 0,
  DSU_PORT_SRAM_WRITE_TO_IO = 1,
  DSU_PORT_IO_WRITE_TO_SRAM = 2,
  DSU_PORT_IO_READ_FROM_SRAM = 3,
  DSU_PORT_WRITE_BULK = 6,
  DSU_PORT_READ_BULK = 7
};
enum REP_PORT {
  REP_PORT_SRAM_READ_FROM_IO = 0,
  REP_PORT_SRAM_WRITE_TO_IO = 1,
  REP_PORT_IO_WRITE_TO_SRAM = 2,
  REP_PORT_IO_READ_FROM_SRAM = 3,
  REP_PORT_WRITE_BULK = 6,
  REP_PORT_READ_BULK = 7
};
enum REPX_PORT {
  REPX_PORT_SRAM_READ_FROM_IO = 0,
  REPX_PORT_SRAM_WRITE_TO_IO = 1,
  REPX_PORT_IO_WRITE_TO_SRAM = 2,
  REPX_PORT_IO_READ_FROM_SRAM = 3,
  REPX_PORT_WRITE_BULK = 6,
  REPX_PORT_READ_BULK = 7
};

// Instruction formats
struct DSUInstruction {
  uint32_t slot, option, port, init_addr_sd, init_addr;

  DSUInstruction(const Instruction &instr) {
    slot = instr.slot;
    option = instr.get("option").value;
    port = instr.get("port").value;
    init_addr_sd = instr.get("init_addr_sd").value;
    init_addr = instr.get("init_addr").value;
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
createInstructionHandlers(Iosram_top *iosram_top_obj);

} // namespace IOSRAM_TOP_PKG

#endif // _RF_PKG_H
