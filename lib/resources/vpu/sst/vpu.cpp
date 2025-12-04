#include "vpu.h"

using namespace SST;

Vpu::Vpu(SST::ComponentId_t id, SST::Params &params)
    : DRRAResource(id, params) {
      instructionHandlers = VPU_PKG::createInstructionHandlers(this);
}

bool Vpu::clockTick(SST::Cycle_t currentCycle) {
  return DRRAResource::clockTick(currentCycle);
}

void Vpu::handleVPU(const VPU_PKG::VPUInstruction &instr) {
  out.output(
    "vpu (slot=%d, option=%d, mode=%d, immediate=%d)\n",
    instr.slot, instr.option, instr.mode, instr.immediate);

  // TODO: implement instruction behavior
}

void Vpu::handleREP(const VPU_PKG::REPInstruction &instr) {
  out.output(
    "rep (slot=%d, port=%d, level=%d, iter=%d, step=%d, delay=%d)\n",
    instr.slot, instr.port, instr.level, instr.iter, instr.step, instr.delay);

  // TODO: implement instruction behavior
}

void Vpu::handleTRANS(const VPU_PKG::TRANSInstruction &instr) {
  out.output(
    "trans (slot=%d, port=%d, delay=%d, level=%d)\n",
    instr.slot, instr.port, instr.delay, instr.level);

  // TODO: implement instruction behavior
}

