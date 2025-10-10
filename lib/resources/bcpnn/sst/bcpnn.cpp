#include "bcpnn.h"

using namespace SST;

Bcpnn::Bcpnn(SST::ComponentId_t id, SST::Params &params)
    : DRRAResource(id, params) {
      instructionHandlers = BCPNN_PKG::createInstructionHandlers(this);
}

bool Bcpnn::clockTick(SST::Cycle_t currentCycle) {
  return DRRAResource::clockTick(currentCycle);
}

void Bcpnn::handleDSU(const BCPNN_PKG::DSUInstruction &instr) {
  out.output(
    "dsu (slot=%d, init_addr_sd=%d, init_addr=%d, port=%d)\n",
    instr.slot, instr.init_addr_sd, instr.init_addr, instr.port);

  // TODO: implement instruction behavior
}

void Bcpnn::handleREP(const BCPNN_PKG::REPInstruction &instr) {
  out.output(
    "rep (slot=%d, port=%d, level=%d, iter=%d, step=%d, delay=%d)\n",
    instr.slot, instr.port, instr.level, instr.iter, instr.step, instr.delay);

  // TODO: implement instruction behavior
}

void Bcpnn::handleREPX(const BCPNN_PKG::REPXInstruction &instr) {
  out.output(
    "repx (slot=%d, port=%d, level=%d, iter=%d, step=%d, delay=%d)\n",
    instr.slot, instr.port, instr.level, instr.iter, instr.step, instr.delay);

  // TODO: implement instruction behavior
}

