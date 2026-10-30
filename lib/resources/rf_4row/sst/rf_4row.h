#ifndef _RF_4ROW_H
#define _RF_4ROW_H

#include "register_file.h"
#include "rf_4row_pkg.h"
#include <vector>

// The 64-word, four-row register file. The model itself is RegisterFile
// (lib/common/sst), shared with rf_1row and rf_2row.
class Rf_4row : public RegisterFile {
public:
  /* Element Library Info */
  SST_ELI_REGISTER_COMPONENT(Rf_4row, "drra", "rf_4row", SST_ELI_ELEMENT_VERSION(1, 0, 0),
                             "Rf_4row component", COMPONENT_CATEGORY_MEMORY)

  /* Element Library Params */
  static std::vector<SST::ElementInfoParam> getComponentParams() {
    return RegisterFile::getRegisterFileParams("64", "6", "2");
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
  Rf_4row(SST::ComponentId_t id, SST::Params &params);

  /* Destructor */
  ~Rf_4row() {};

  // Instruction format
  using DRRAResource::format;
  void handleCONF(const RF_4ROW_PKG::CONFInstruction &instr);

  using DRRAResource::out;
};

#endif // _RF_4ROW_H
