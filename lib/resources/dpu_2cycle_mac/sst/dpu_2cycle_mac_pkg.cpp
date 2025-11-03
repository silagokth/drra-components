#include "dpu_2cycle_mac_pkg.h"
#include "dpu_2cycle_mac.h"

std::unordered_map<uint32_t, std::function<void(uint32_t)>>
DPU_2CYCLE_MAC_PKG::createInstructionHandlers(Dpu_2cycle_mac *dpu_2cycle_mac_obj) {
  return {
    {(uint32_t)OpCode::DPU,
      [dpu_2cycle_mac_obj](uint32_t instr) {
        auto segment_defs = getIsaDefinitions().at(OpCode::DPU);
        Instruction instruction(instr, dpu_2cycle_mac_obj->format, segment_defs);
        DPUInstruction decoded_instr(instruction);
        dpu_2cycle_mac_obj->handleDPU(decoded_instr);
      }
    },    {(uint32_t)OpCode::REP,
      [dpu_2cycle_mac_obj](uint32_t instr) {
        auto segment_defs = getIsaDefinitions().at(OpCode::REP);
        Instruction instruction(instr, dpu_2cycle_mac_obj->format, segment_defs);
        REPInstruction decoded_instr(instruction);
        dpu_2cycle_mac_obj->handleREP(decoded_instr);
      }
    },    {(uint32_t)OpCode::REPX,
      [dpu_2cycle_mac_obj](uint32_t instr) {
        auto segment_defs = getIsaDefinitions().at(OpCode::REPX);
        Instruction instruction(instr, dpu_2cycle_mac_obj->format, segment_defs);
        REPXInstruction decoded_instr(instruction);
        dpu_2cycle_mac_obj->handleREPX(decoded_instr);
      }
    },    {(uint32_t)OpCode::FSM,
      [dpu_2cycle_mac_obj](uint32_t instr) {
        auto segment_defs = getIsaDefinitions().at(OpCode::FSM);
        Instruction instruction(instr, dpu_2cycle_mac_obj->format, segment_defs);
        FSMInstruction decoded_instr(instruction);
        dpu_2cycle_mac_obj->handleFSM(decoded_instr);
      }
    }  };
}