#include "rf_1row.h"
#include "rf_1row_pkg.h"

using namespace SST;

Rf_1row::Rf_1row(SST::ComponentId_t id, SST::Params &params)
    : RegisterFile(id, params) {
  instructionHandlers = RF_1ROW_PKG::createInstructionHandlers(this);
  registerAccessPorts(
      {RF_1ROW_PKG::EVT_PORT_WORD_WRITE, RF_1ROW_PKG::EVT_PORT_WORD_READ,
       RF_1ROW_PKG::EVT_PORT_BULK_WRITE, RF_1ROW_PKG::EVT_PORT_BULK_READ});
}

void Rf_1row::handleCONF(const RF_1ROW_PKG::CONFInstruction &instr) {
  conf(instr.slot, instr.address, instr.value);
}
