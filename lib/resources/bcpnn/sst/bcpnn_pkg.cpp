#include "bcpnn_pkg.h"
#include "bcpnn.h"

std::unordered_map<uint32_t, std::function<void(uint32_t)>>
BCPNN_PKG::createInstructionHandlers(Bcpnn *bcpnn_obj) {
  return {
    {(uint32_t)OpCode::DSU,
      [bcpnn_obj](uint32_t instr) {
        auto segment_defs = getIsaDefinitions().at(OpCode::DSU);
        Instruction instruction(instr, bcpnn_obj->format, segment_defs);
        DSUInstruction decoded_instr(instruction);
        bcpnn_obj->handleDSU(decoded_instr);
      }
    },    {(uint32_t)OpCode::REP,
      [bcpnn_obj](uint32_t instr) {
        auto segment_defs = getIsaDefinitions().at(OpCode::REP);
        Instruction instruction(instr, bcpnn_obj->format, segment_defs);
        REPInstruction decoded_instr(instruction);
        bcpnn_obj->handleREP(decoded_instr);
      }
    },    {(uint32_t)OpCode::REPX,
      [bcpnn_obj](uint32_t instr) {
        auto segment_defs = getIsaDefinitions().at(OpCode::REPX);
        Instruction instruction(instr, bcpnn_obj->format, segment_defs);
        REPXInstruction decoded_instr(instruction);
        bcpnn_obj->handleREPX(decoded_instr);
      }
    }  };
}