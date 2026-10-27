#ifndef _RF_ROW_H
#define _RF_ROW_H

#include "register_file.h"
#include "rf_row_pkg.h"
#include <vector>

// The 16-word register file: a single row, so every bulk access covers all of
// it. Same ISA and ports as rf; the model itself is RegisterFile
// (lib/common/sst), shared with rf. The class name is fixed by the generated
// package, which capitalizes the kind.
class Rf_row : public RegisterFile {
public:
  /* Element Library Info */
  SST_ELI_REGISTER_COMPONENT(Rf_row, "drra", "rf_row",
                             SST_ELI_ELEMENT_VERSION(1, 0, 0),
                             "Single-row register file component",
                             COMPONENT_CATEGORY_MEMORY)

  /* Element Library Params */
  static std::vector<SST::ElementInfoParam> getComponentParams() {
    return RegisterFile::getRegisterFileParams("16", "4", "0");
  }
  SST_ELI_DOCUMENT_PARAMS(getComponentParams())

  /* Element Library Ports */
  static std::vector<SST::ElementInfoPort> getComponentPorts() {
    auto ports = DRRAResource::getBasePorts();
    return ports;
  }
  SST_ELI_DOCUMENT_PORTS(getComponentPorts())

  /* Element Library Statistics */
  static std::vector<SST::ElementInfoStatistic> getComponentStatistics() {
    auto stats = DRRAResource::getBaseStatistics();
    return stats;
  }
  SST_ELI_DOCUMENT_STATISTICS(getComponentStatistics())

  /* Constructor */
  Rf_row(SST::ComponentId_t id, SST::Params &params);

  /* Destructor */
  ~Rf_row() {};

  // Instruction format
  using DRRAResource::format;
  void handleCONF(const RF_ROW_PKG::CONFInstruction &instr);

  using DRRAResource::out;
};

#endif // _RF_ROW_H
