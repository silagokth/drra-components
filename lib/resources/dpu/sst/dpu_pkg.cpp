#include "dpu_pkg.h"
#include "dpu.h"

std::unordered_map<uint32_t, std::function<void(uint32_t)>>
DPU_PKG::createInstructionHandlers(Dpu *dpu_obj) {
  return {{(uint32_t)OpCode::DPU,
           [dpu_obj](uint32_t instr) {
             auto segment_defs = getIsaDefinitions().at(OpCode::DPU);
             Instruction instruction(instr, dpu_obj->format, segment_defs);
             DPUInstruction decoded_instr(instruction);
             dpu_obj->handleDPU(decoded_instr);
           }},
          {(uint32_t)OpCode::REP,
           [dpu_obj](uint32_t instr) {
             auto segment_defs = getIsaDefinitions().at(OpCode::REP);
             Instruction instruction(instr, dpu_obj->format, segment_defs);
             REPInstruction decoded_instr(instruction);
             dpu_obj->handleREP(decoded_instr);
           }},
          {(uint32_t)OpCode::REPX,
           [dpu_obj](uint32_t instr) {
             auto segment_defs = getIsaDefinitions().at(OpCode::REPX);
             Instruction instruction(instr, dpu_obj->format, segment_defs);
             REPXInstruction decoded_instr(instruction);
             dpu_obj->handleREPX(decoded_instr);
           }},
          {(uint32_t)OpCode::FSM, [dpu_obj](uint32_t instr) {
             auto segment_defs = getIsaDefinitions().at(OpCode::FSM);
             Instruction instruction(instr, dpu_obj->format, segment_defs);
             FSMInstruction decoded_instr(instruction);
             dpu_obj->handleFSM(decoded_instr);
           }}};
}
