#include "iosram_both_pkg.h"
#include "iosram_both.h"

std::unordered_map<uint32_t, std::function<void(uint32_t)>>
IOSRAM_BOTH_PKG::createInstructionHandlers(Iosram_both *iosram_both_obj) {
  return {
      {(uint32_t)OpCode::DSU,
       [iosram_both_obj](uint32_t instr) {
         auto segment_defs = getIsaDefinitions().at(OpCode::DSU);
         Instruction instruction(instr, iosram_both_obj->format, segment_defs);
         DSUInstruction decoded_instr(instruction);
         iosram_both_obj->handleDSU(decoded_instr);
       }},
      {(uint32_t)OpCode::REP,
       [iosram_both_obj](uint32_t instr) {
         auto segment_defs = getIsaDefinitions().at(OpCode::REP);
         Instruction instruction(instr, iosram_both_obj->format, segment_defs);
         REPInstruction decoded_instr(instruction);
         iosram_both_obj->handleREP(decoded_instr);
       }},
      {(uint32_t)OpCode::REPX,
       [iosram_both_obj](uint32_t instr) {
         auto segment_defs = getIsaDefinitions().at(OpCode::REPX);
         Instruction instruction(instr, iosram_both_obj->format, segment_defs);
         REPXInstruction decoded_instr(instruction);
         iosram_both_obj->handleREPX(decoded_instr);
       }},
      {(uint32_t)OpCode::TRANS, [iosram_both_obj](uint32_t instr) {
         auto segment_defs = getIsaDefinitions().at(OpCode::TRANS);
         Instruction instruction(instr, iosram_both_obj->format, segment_defs);
         TRANSInstruction decoded_instr(instruction);
         iosram_both_obj->handleTRANS(decoded_instr);
       }}};
}
