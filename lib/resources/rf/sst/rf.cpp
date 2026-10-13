#include "rf.h"
#include "rf_pkg.h"

using namespace SST;

Rf::Rf(SST::ComponentId_t id, SST::Params &params) : RegisterFile(id, params) {
  instructionHandlers = RF_PKG::createInstructionHandlers(this);
  registerAccessPorts({RF_PKG::EVT_PORT_WORD_WRITE, RF_PKG::EVT_PORT_WORD_READ,
                       RF_PKG::EVT_PORT_BULK_WRITE,
                       RF_PKG::EVT_PORT_BULK_READ});
}

void Rf::handleCONF(const RF_PKG::CONFInstruction &instr) {
  conf(instr.slot, instr.address, instr.value);
}
