#ifndef _SWB_PKG_H
#define _SWB_PKG_H

#include "instruction.h"
#include <cstdint>
#include <functional>
#include <unordered_map>

class Swb;

namespace SWB_PKG {

// Supported opcodes from ISA
enum OpCode { EVT = 0, REP = 1, REPX = 2, TRANS = 3, SWB = 4, ROUTE = 5 };

enum SWB_ISNTR_EVT {
  SWB_INSTR_EVT_PORT_BITWIDTH = 1,
};

enum SWB_INSTR_REP {
  SWB_INSTR_REP_PORT_BITWIDTH = 1,
  SWB_INSTR_REP_ITER_BITWIDTH = 8,
  SWB_INSTR_REP_STEP_BITWIDTH = 7,
  SWB_INSTR_REP_DELAY_BITWIDTH = 8
};

enum SWB_INSTR_REPX {
  SWB_INSTR_REPX_PORT_BITWIDTH = 1,
  SWB_INSTR_REPX_ITER_BITWIDTH = 8,
  SWB_INSTR_REPX_STEP_BITWIDTH = 7,
  SWB_INSTR_REPX_DELAY_BITWIDTH = 8
};

enum SWB_INSTR_TRANS {
  SWB_INSTR_TRANS_PORT_BITWIDTH = 1,
  SWB_INSTR_TRANS_DELAY_BITWIDTH = 23,
};

enum SWB_INSTR_SWB {
  SWB_INSTR_SWB_OPTION_BITWIDTH = 2,
  SWB_INSTR_SWB_CHANNEL_BITWIDTH = 4,
  SWB_INSTR_SWB_SOURCE_BITWIDTH = 4,
  SWB_INSTR_SWB_TARGET_BITWIDTH = 4
};

enum SWB_INSTR_ROUTE {
  SWB_INSTR_ROUTE_OPTION_BITWIDTH = 2,
  SWB_INSTR_ROUTE_SR_BITWIDTH = 1,
  SWB_INSTR_ROUTE_SOURCE_BITWIDTH = 4,
  SWB_INSTR_ROUTE_TARGET_BITWIDTH = 16
};

// ISA segment definitions
inline const std::unordered_map<OpCode, std::vector<SegmentRange>> &
getIsaDefinitions() {
  uint32_t INSTR_PAYLOAD_BITWIDTH = 24;
  static const std::unordered_map<OpCode, std::vector<SegmentRange>>
      segmentsDef = {
          {OpCode::EVT,
           {SegmentRange("port", SWB_INSTR_EVT_PORT_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH -
                             SWB_INSTR_EVT_PORT_BITWIDTH)}},
          {OpCode::REP,
           {SegmentRange("port", SWB_INSTR_REP_PORT_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH - SWB_INSTR_REP_PORT_BITWIDTH),
            SegmentRange("iter", SWB_INSTR_REP_ITER_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH - SWB_INSTR_REP_PORT_BITWIDTH -
                             SWB_INSTR_REP_ITER_BITWIDTH),
            SegmentRange("step", SWB_INSTR_REP_STEP_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH - SWB_INSTR_REP_PORT_BITWIDTH -
                             SWB_INSTR_REP_ITER_BITWIDTH -
                             SWB_INSTR_REP_STEP_BITWIDTH),
            SegmentRange("delay", SWB_INSTR_REP_DELAY_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH - SWB_INSTR_REP_PORT_BITWIDTH -
                             SWB_INSTR_REP_ITER_BITWIDTH -
                             SWB_INSTR_REP_STEP_BITWIDTH -
                             SWB_INSTR_REP_DELAY_BITWIDTH)}},
          {OpCode::REPX,
           {SegmentRange("port", SWB_INSTR_REPX_PORT_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH - SWB_INSTR_REPX_PORT_BITWIDTH),
            SegmentRange("iter", SWB_INSTR_REPX_ITER_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH - SWB_INSTR_REPX_PORT_BITWIDTH -
                             SWB_INSTR_REPX_ITER_BITWIDTH),
            SegmentRange("step", SWB_INSTR_REPX_STEP_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH - SWB_INSTR_REPX_PORT_BITWIDTH -
                             SWB_INSTR_REPX_ITER_BITWIDTH -
                             SWB_INSTR_REPX_STEP_BITWIDTH),
            SegmentRange("delay", SWB_INSTR_REPX_DELAY_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH - SWB_INSTR_REPX_PORT_BITWIDTH -
                             SWB_INSTR_REPX_ITER_BITWIDTH -
                             SWB_INSTR_REPX_STEP_BITWIDTH -
                             SWB_INSTR_REPX_DELAY_BITWIDTH)}},
          {OpCode::TRANS,
           {SegmentRange("port", SWB_INSTR_TRANS_PORT_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH -
                             SWB_INSTR_TRANS_PORT_BITWIDTH),
            SegmentRange("delay", SWB_INSTR_TRANS_DELAY_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH -
                             SWB_INSTR_TRANS_PORT_BITWIDTH -
                             SWB_INSTR_TRANS_DELAY_BITWIDTH)}},
          {OpCode::SWB,
           {SegmentRange("option", SWB_INSTR_SWB_OPTION_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH -
                             SWB_INSTR_SWB_OPTION_BITWIDTH),
            SegmentRange("channel", SWB_INSTR_SWB_CHANNEL_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH -
                             SWB_INSTR_SWB_OPTION_BITWIDTH -
                             SWB_INSTR_SWB_CHANNEL_BITWIDTH),
            SegmentRange("source", SWB_INSTR_SWB_SOURCE_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH -
                             SWB_INSTR_SWB_OPTION_BITWIDTH -
                             SWB_INSTR_SWB_CHANNEL_BITWIDTH -
                             SWB_INSTR_SWB_SOURCE_BITWIDTH),
            SegmentRange("target", SWB_INSTR_SWB_TARGET_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH -
                             SWB_INSTR_SWB_OPTION_BITWIDTH -
                             SWB_INSTR_SWB_CHANNEL_BITWIDTH -
                             SWB_INSTR_SWB_SOURCE_BITWIDTH -
                             SWB_INSTR_SWB_TARGET_BITWIDTH)}},
          {OpCode::ROUTE,
           {SegmentRange("option", SWB_INSTR_ROUTE_OPTION_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH -
                             SWB_INSTR_ROUTE_OPTION_BITWIDTH),
            SegmentRange("sr", SWB_INSTR_ROUTE_SR_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH -
                             SWB_INSTR_ROUTE_OPTION_BITWIDTH -
                             SWB_INSTR_ROUTE_SR_BITWIDTH),
            SegmentRange("source", SWB_INSTR_ROUTE_SOURCE_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH -
                             SWB_INSTR_ROUTE_OPTION_BITWIDTH -
                             SWB_INSTR_ROUTE_SR_BITWIDTH -
                             SWB_INSTR_ROUTE_SOURCE_BITWIDTH),
            SegmentRange("target", SWB_INSTR_ROUTE_TARGET_BITWIDTH,
                         INSTR_PAYLOAD_BITWIDTH -
                             SWB_INSTR_ROUTE_OPTION_BITWIDTH -
                             SWB_INSTR_ROUTE_SR_BITWIDTH -
                             SWB_INSTR_ROUTE_SOURCE_BITWIDTH -
                             SWB_INSTR_ROUTE_TARGET_BITWIDTH)}}};
  return segmentsDef;
}

// ISA verbo mappings
const uint32_t PORT_INTRACELL = 0;
const uint32_t PORT_INTERCELL = 1;
enum ROUTE_SR { ROUTE_SR_SEND = 0, ROUTE_SR_RECEIVE = 1 };
enum EVT_PORT {
  EVT_PORT_INTRACELL = PORT_INTRACELL,
  EVT_PORT_INTERCELL = PORT_INTERCELL
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

std::unordered_map<uint32_t, std::function<void(uint32_t)>>
createInstructionHandlers(Swb *swb_obj);

} // namespace SWB_PKG

#endif // _SWB_PKG_H
