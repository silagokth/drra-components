#include "rf_2row.h"
#include "rf_2row_pkg.h"

using namespace SST;

Rf_2row::Rf_2row(SST::ComponentId_t id, SST::Params &params)
    : RegisterFile(id, params) {
  instructionHandlers = RF_2ROW_PKG::createInstructionHandlers(this);
  registerAccessPorts(
      {RF_2ROW_PKG::EVT_PORT_WORD_WRITE, RF_2ROW_PKG::EVT_PORT_WORD_READ,
       RF_2ROW_PKG::EVT_PORT_BULK_WRITE, RF_2ROW_PKG::EVT_PORT_BULK_READ});
}

void Rf_2row::handleCONF(const RF_2ROW_PKG::CONFInstruction &instr) {
  conf(instr.slot, instr.address, instr.value);
}
