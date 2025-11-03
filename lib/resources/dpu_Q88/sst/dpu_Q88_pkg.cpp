#include "dpu_Q88_pkg.h"
#include "dpu_Q88.h"

std::unordered_map<uint32_t, std::function<void(uint32_t)>>
DPU_Q88_PKG::createInstructionHandlers(Dpu_q88 *dpu_q88_obj) {
  return {{(uint32_t)OpCode::DPU,
           [dpu_q88_obj](uint32_t instr) {
             auto segment_defs = getIsaDefinitions().at(OpCode::DPU);
             Instruction instruction(instr, dpu_q88_obj->format, segment_defs);
             DPUInstruction decoded_instr(instruction);
             dpu_q88_obj->handleDPU(decoded_instr);
           }},
          {(uint32_t)OpCode::REP,
           [dpu_q88_obj](uint32_t instr) {
             auto segment_defs = getIsaDefinitions().at(OpCode::REP);
             Instruction instruction(instr, dpu_q88_obj->format, segment_defs);
             REPInstruction decoded_instr(instruction);
             dpu_q88_obj->handleREP(decoded_instr);
           }},
          {(uint32_t)OpCode::REPX,
           [dpu_q88_obj](uint32_t instr) {
             auto segment_defs = getIsaDefinitions().at(OpCode::REPX);
             Instruction instruction(instr, dpu_q88_obj->format, segment_defs);
             REPXInstruction decoded_instr(instruction);
             dpu_q88_obj->handleREPX(decoded_instr);
           }},
          {(uint32_t)OpCode::FSM, [dpu_q88_obj](uint32_t instr) {
             auto segment_defs = getIsaDefinitions().at(OpCode::FSM);
             Instruction instruction(instr, dpu_q88_obj->format, segment_defs);
             FSMInstruction decoded_instr(instruction);
             dpu_q88_obj->handleFSM(decoded_instr);
           }}};
}
