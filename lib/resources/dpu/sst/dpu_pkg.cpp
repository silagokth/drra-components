#include "dpu_pkg.h"
#include "dpu.h"

std::unordered_map<uint32_t, std::function<void(uint32_t)>>
DPU_PKG::createInstructionHandlers(DPU *dpu) {
  return {{(uint32_t)OpCode::REP,
           [dpu](uint32_t instr) {
             auto segments_def = getIsaDefinitions().at(OpCode::REP);
             Instruction instruction(instr, dpu->format, segments_def);
             REPInstruction rep_instr(instruction);
             dpu->handleREP(rep_instr);
           }},
          {(uint32_t)OpCode::REPX,
           [dpu](uint32_t instr) {
             auto segments_def = getIsaDefinitions().at(OpCode::REPX);
             Instruction instruction(instr, dpu->format, segments_def);
             REPXInstruction repx_instr(instruction);
             dpu->handleREPX(repx_instr);
           }},
          {(uint32_t)OpCode::FSM,
           [dpu](uint32_t instr) {
             auto segments_def = getIsaDefinitions().at(OpCode::FSM);
             Instruction instruction(instr, dpu->format, segments_def);
             FSMInstruction fsm_instr(instruction);
             dpu->handleFSM(fsm_instr);
           }},
          {(uint32_t)OpCode::DPU_OP, [dpu](uint32_t instr) {
             auto segments_def = getIsaDefinitions().at(OpCode::DPU_OP);
             Instruction instruction(instr, dpu->format, segments_def);
             DPUInstruction dpu_instr(instruction);
             dpu->handleDPU(dpu_instr);
           }}};
}
