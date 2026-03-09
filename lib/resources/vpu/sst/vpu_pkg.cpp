#include "vpu_pkg.h"
#include "vpu.h"

std::unordered_map<uint32_t, std::function<void(uint32_t)>>
VPU_PKG::createInstructionHandlers(Vpu *vpu_obj) {
  return {
    {(uint32_t)OpCode::VPU,
      [vpu_obj](uint32_t instr) {
        auto segment_defs = getIsaDefinitions().at(OpCode::VPU);
        Instruction instruction(instr, vpu_obj->format, segment_defs);
        VPUInstruction decoded_instr(instruction);
        vpu_obj->handleVPU(decoded_instr);
      }
    },    {(uint32_t)OpCode::EVT,
      [vpu_obj](uint32_t instr) {
        auto segment_defs = getIsaDefinitions().at(OpCode::EVT);
        Instruction instruction(instr, vpu_obj->format, segment_defs);
        EVTInstruction decoded_instr(instruction);
        vpu_obj->handleEVT(decoded_instr);
      }
    },    {(uint32_t)OpCode::REP,
      [vpu_obj](uint32_t instr) {
        auto segment_defs = getIsaDefinitions().at(OpCode::REP);
        Instruction instruction(instr, vpu_obj->format, segment_defs);
        REPInstruction decoded_instr(instruction);
        vpu_obj->handleREP(decoded_instr);
      }
    },    {(uint32_t)OpCode::REPX,
      [vpu_obj](uint32_t instr) {
        auto segment_defs = getIsaDefinitions().at(OpCode::REPX);
        Instruction instruction(instr, vpu_obj->format, segment_defs);
        REPXInstruction decoded_instr(instruction);
        vpu_obj->handleREPX(decoded_instr);
      }
    },    {(uint32_t)OpCode::TRANS,
      [vpu_obj](uint32_t instr) {
        auto segment_defs = getIsaDefinitions().at(OpCode::TRANS);
        Instruction instruction(instr, vpu_obj->format, segment_defs);
        TRANSInstruction decoded_instr(instruction);
        vpu_obj->handleTRANS(decoded_instr);
      }
    }  };
}