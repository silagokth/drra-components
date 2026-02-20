#include "iosram_top_pkg.h"
#include "iosram_top.h"

std::unordered_map<uint32_t, std::function<void(uint32_t)>>
IOSRAM_TOP_PKG::createInstructionHandlers(Iosram_top *iosram_top_obj) {
  return {
      {(uint32_t)OpCode::DSU,
       [iosram_top_obj](uint32_t instr) {
         auto segment_defs = getIsaDefinitions().at(OpCode::DSU);
         Instruction instruction(instr, iosram_top_obj->format, segment_defs);
         DSUInstruction decoded_instr(instruction);
         iosram_top_obj->handleDSU(decoded_instr);
       }},
      {(uint32_t)OpCode::REP,
       [iosram_top_obj](uint32_t instr) {
         auto segment_defs = getIsaDefinitions().at(OpCode::REP);
         Instruction instruction(instr, iosram_top_obj->format, segment_defs);
         REPInstruction decoded_instr(instruction);
         iosram_top_obj->handleREP(decoded_instr);
       }},
      {(uint32_t)OpCode::REPX,
       [iosram_top_obj](uint32_t instr) {
         auto segment_defs = getIsaDefinitions().at(OpCode::REPX);
         Instruction instruction(instr, iosram_top_obj->format, segment_defs);
         REPXInstruction decoded_instr(instruction);
         iosram_top_obj->handleREPX(decoded_instr);
       }},
      {(uint32_t)OpCode::TRANS, [iosram_top_obj](uint32_t instr) {
         auto segment_defs = getIsaDefinitions().at(OpCode::TRANS);
         Instruction instruction(instr, iosram_top_obj->format, segment_defs);
         TRANSInstruction decoded_instr(instruction);
         iosram_top_obj->handleTRANS(decoded_instr);
       }}};
}
