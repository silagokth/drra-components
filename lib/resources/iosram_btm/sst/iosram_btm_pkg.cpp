#include "iosram_btm_pkg.h"
#include "iosram_btm.h"

std::unordered_map<uint32_t, std::function<void(uint32_t)>>
IOSRAM_BTM_PKG::createInstructionHandlers(Iosram_btm *iosram_btm_obj) {
  return {
      {(uint32_t)OpCode::DSU,
       [iosram_btm_obj](uint32_t instr) {
         auto segment_defs = getIsaDefinitions().at(OpCode::DSU);
         Instruction instruction(instr, iosram_btm_obj->format, segment_defs);
         DSUInstruction decoded_instr(instruction);
         iosram_btm_obj->handleDSU(decoded_instr);
       }},
      {(uint32_t)OpCode::REP,
       [iosram_btm_obj](uint32_t instr) {
         auto segment_defs = getIsaDefinitions().at(OpCode::REP);
         Instruction instruction(instr, iosram_btm_obj->format, segment_defs);
         REPInstruction decoded_instr(instruction);
         iosram_btm_obj->handleREP(decoded_instr);
       }},
      {(uint32_t)OpCode::REPX,
       [iosram_btm_obj](uint32_t instr) {
         auto segment_defs = getIsaDefinitions().at(OpCode::REPX);
         Instruction instruction(instr, iosram_btm_obj->format, segment_defs);
         REPXInstruction decoded_instr(instruction);
         iosram_btm_obj->handleREPX(decoded_instr);
       }},
      {(uint32_t)OpCode::TRANS, [iosram_btm_obj](uint32_t instr) {
         auto segment_defs = getIsaDefinitions().at(OpCode::TRANS);
         Instruction instruction(instr, iosram_btm_obj->format, segment_defs);
         TRANSInstruction decoded_instr(instruction);
         iosram_btm_obj->handleTRANS(decoded_instr);
       }}};
}
