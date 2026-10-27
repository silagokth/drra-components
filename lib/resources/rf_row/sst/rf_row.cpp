#include "rf_row.h"
#include "rf_row_pkg.h"

using namespace SST;

Rf_row::Rf_row(SST::ComponentId_t id, SST::Params &params)
    : RegisterFile(id, params) {
  instructionHandlers = RF_ROW_PKG::createInstructionHandlers(this);
  registerAccessPorts(
      {RF_ROW_PKG::EVT_PORT_WORD_WRITE, RF_ROW_PKG::EVT_PORT_WORD_READ,
       RF_ROW_PKG::EVT_PORT_BULK_WRITE, RF_ROW_PKG::EVT_PORT_BULK_READ});
}

void Rf_row::handleCONF(const RF_ROW_PKG::CONFInstruction &instr) {
  conf(instr.slot, instr.address, instr.value);
}
