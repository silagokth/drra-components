#include "swb_pkg.h"
#include "swb.h"

std::unordered_map<uint32_t, std::function<void(uint32_t)>>
SWB_PKG::createInstructionHandlers(Swb *swb_obj) {
  return {
      {OpCode::EVT,
       [swb_obj](uint32_t instr) {
         auto segment_defs = getIsaDefinitions().at(OpCode::EVT);
         Instruction instruction(instr, swb_obj->format, segment_defs);
         EVTInstruction decoded_instr(instruction);
         swb_obj->handleEVT(decoded_instr);
       }},
      {(uint32_t)OpCode::REP,
       [swb_obj](uint32_t instr) {
         auto segment_defs = getIsaDefinitions().at(OpCode::REP);
         Instruction instruction(instr, swb_obj->format, segment_defs);
         REPInstruction decoded_instr(instruction);
         swb_obj->handleREP(decoded_instr);
       }},
      {(uint32_t)OpCode::REPX,
       [swb_obj](uint32_t instr) {
         auto segment_defs = getIsaDefinitions().at(OpCode::REPX);
         Instruction instruction(instr, swb_obj->format, segment_defs);
         REPXInstruction decoded_instr(instruction);
         swb_obj->handleREPX(decoded_instr);
       }},
      {(uint32_t)OpCode::TRANS,
       [swb_obj](uint32_t instr) {
         auto segment_defs = getIsaDefinitions().at(OpCode::TRANS);
         Instruction instruction(instr, swb_obj->format, segment_defs);
         TRANSInstruction decoded_instr(instruction);
         swb_obj->handleTRANS(decoded_instr);
       }},
      {(uint32_t)OpCode::SWB,
       [swb_obj](uint32_t instr) {
         auto segment_defs = getIsaDefinitions().at(OpCode::SWB);
         Instruction instruction(instr, swb_obj->format, segment_defs);
         SWBInstruction decoded_instr(instruction);
         swb_obj->handleSWB(decoded_instr);
       }},
      {(uint32_t)OpCode::ROUTE,
       [swb_obj](uint32_t instr) {
         auto segment_defs = getIsaDefinitions().at(OpCode::ROUTE);
         Instruction instruction(instr, swb_obj->format, segment_defs);
         ROUTEInstruction decoded_instr(instruction);
         swb_obj->handleROUTE(decoded_instr);
       }},
  };
}
