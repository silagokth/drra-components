#include "rf_4row.h"
#include "rf_4row_pkg.h"

using namespace SST;

Rf_4row::Rf_4row(SST::ComponentId_t id, SST::Params &params) : RegisterFile(id, params) {
  instructionHandlers = RF_4ROW_PKG::createInstructionHandlers(this);
  registerAccessPorts({RF_4ROW_PKG::EVT_PORT_WORD_WRITE, RF_4ROW_PKG::EVT_PORT_WORD_READ,
                       RF_4ROW_PKG::EVT_PORT_BULK_WRITE,
                       RF_4ROW_PKG::EVT_PORT_BULK_READ});
}

void Rf_4row::handleCONF(const RF_4ROW_PKG::CONFInstruction &instr) {
  conf(instr.slot, instr.address, instr.value);
}
