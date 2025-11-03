#ifndef _SWB_PKG_H
#define _SWB_PKG_H

#include "instruction.h"
#include <cstdint>
#include <functional>
#include <unordered_map>

class Swb;

namespace SWB_PKG {

// Supported opcodes from ISA
enum OpCode { SWB = 4, ROUTE = 5, REP = 0, REPX = 1, FSM = 2 };

// ISA segment definitions
inline const std::unordered_map<OpCode, std::vector<SegmentRange>> &
getIsaDefinitions() {
  static const std::unordered_map<OpCode, std::vector<SegmentRange>>
      segmentsDef = {
          {OpCode::SWB,
           {SegmentRange("option", 2, 22), SegmentRange("channel", 4, 18),
            SegmentRange("source", 4, 14), SegmentRange("target", 4, 10)}},
          {OpCode::ROUTE,
           {SegmentRange("option", 2, 22), SegmentRange("sr", 1, 21),
            SegmentRange("source", 4, 17), SegmentRange("target", 16, 1)}},
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
enum ROUTE_SR { ROUTE_SR_S = 0, ROUTE_SR_R = 1 };
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
struct SWBInstruction {
  uint32_t slot, option, channel, source, target;

  SWBInstruction(const Instruction &instr) {
    slot = instr.slot;
    option = instr.get("option").value;
    channel = instr.get("channel").value;
    source = instr.get("source").value;
    target = instr.get("target").value;
  }
};
struct ROUTEInstruction {
  uint32_t slot, option, sr, source, target;

  ROUTEInstruction(const Instruction &instr) {
    slot = instr.slot;
    option = instr.get("option").value;
    sr = instr.get("sr").value;
    source = instr.get("source").value;
    target = instr.get("target").value;
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
createInstructionHandlers(Swb *swb_obj);

} // namespace SWB_PKG

#endif // _SWB_PKG_H
