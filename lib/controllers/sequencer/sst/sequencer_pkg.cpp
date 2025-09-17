#include "sequencer_pkg.h"
#include "sequencer.h"

std::unordered_map<uint32_t, std::function<void(uint32_t)>>
SEQUENCER_PKG::createInstructionHandlers(Sequencer *sequencer_obj) {
  return {
      {(uint32_t)OpCode::HALT,
       [sequencer_obj](uint32_t instr) {
         auto segment_defs = getIsaDefinitions().at(OpCode::HALT);
         Instruction instruction(instr, sequencer_obj->format, segment_defs);
         HALTInstruction decoded_instr(instruction);
         sequencer_obj->handleHALT(decoded_instr);
       }},
      {(uint32_t)OpCode::WAIT,
       [sequencer_obj](uint32_t instr) {
         auto segment_defs = getIsaDefinitions().at(OpCode::WAIT);
         Instruction instruction(instr, sequencer_obj->format, segment_defs);
         WAITInstruction decoded_instr(instruction);
         sequencer_obj->handleWAIT(decoded_instr);
       }},
      {(uint32_t)OpCode::ACT,
       [sequencer_obj](uint32_t instr) {
         auto segment_defs = getIsaDefinitions().at(OpCode::ACT);
         Instruction instruction(instr, sequencer_obj->format, segment_defs);
         ACTInstruction decoded_instr(instruction);
         sequencer_obj->handleACT(decoded_instr);
       }},
      {(uint32_t)OpCode::CALC,
       [sequencer_obj](uint32_t instr) {
         auto segment_defs = getIsaDefinitions().at(OpCode::CALC);
         Instruction instruction(instr, sequencer_obj->format, segment_defs);
         CALCInstruction decoded_instr(instruction);
         sequencer_obj->handleCALC(decoded_instr);
       }},
      {(uint32_t)OpCode::BRN, [sequencer_obj](uint32_t instr) {
         auto segment_defs = getIsaDefinitions().at(OpCode::BRN);
         Instruction instruction(instr, sequencer_obj->format, segment_defs);
         BRNInstruction decoded_instr(instruction);
         sequencer_obj->handleBRN(decoded_instr);
       }}};
}
