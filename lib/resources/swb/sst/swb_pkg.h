#ifndef _SWB_PKG_H
#define _SWB_PKG_H

#include "instruction.h"
#include <cstdint>
#include <functional>
#include <unordered_map>

class Swb;

namespace SWB_PKG {

// Supported opcodes from ISA
enum OpCode { SWB = 4, ROUTE = 5, REP = 0, TRANS = 2 };

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
            SegmentRange("iter", 6, 12), SegmentRange("delay", 12, 0)}},
          {OpCode::TRANS,
           {SegmentRange("port", 2, 22), SegmentRange("level", 2, 20),
            SegmentRange("delay", 14, 6)}}};
  return segmentsDef;
}

// ISA verbo mappings
enum ROUTE_SR { ROUTE_SR_SEND = 0, ROUTE_SR_RECEIVE = 1 };
enum REP_PORT {
  REP_PORT_READ_NARROW = 0,
  REP_PORT_READ_WIDE = 1,
  REP_PORT_WRITE_NARROW = 2,
  REP_PORT_WRITE_WIDE = 3
};
enum TRANS_PORT { TRANS_PORT_INTRACELL = 0, TRANS_PORT_INTERCELL = 2 };

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
  uint32_t slot, port, level, iter, delay;

  REPInstruction(const Instruction &instr) {
    slot = instr.slot;
    port = instr.get("port").value;
    level = instr.get("level").value;
    iter = instr.get("iter").value;
    delay = instr.get("delay").value;
  }
};
struct TRANSInstruction {
  uint32_t slot, port, level, delay;

  TRANSInstruction(const Instruction &instr) {
    slot = instr.slot;
    port = instr.get("port").value;
    level = instr.get("level").value;
    delay = instr.get("delay").value;
  }
};

std::unordered_map<uint32_t, std::function<void(uint32_t)>>
createInstructionHandlers(Swb *swb_obj);

} // namespace SWB_PKG

#endif // _SWB_PKG_H
