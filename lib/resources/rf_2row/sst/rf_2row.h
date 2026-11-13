#ifndef _RF_2ROW_H
#define _RF_2ROW_H

#include "register_file.h"
#include "rf_2row_pkg.h"
#include <vector>

// The 32-word, two-row register file. The model itself is RegisterFile
// (lib/common/sst), shared with rf_1row and rf_4row. The class name is fixed
// by the generated package, which capitalizes the kind.
class Rf_2row : public RegisterFile {
public:
  /* Element Library Info */
  SST_ELI_REGISTER_COMPONENT(Rf_2row, "drra", "rf_2row",
                             SST_ELI_ELEMENT_VERSION(1, 0, 0),
                             "Two-row register file component",
                             COMPONENT_CATEGORY_MEMORY)

  /* Element Library Params */
  static std::vector<SST::ElementInfoParam> getComponentParams() {
    return RegisterFile::getRegisterFileParams("32", "5", "1");
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
  Rf_2row(SST::ComponentId_t id, SST::Params &params);

  /* Destructor */
  ~Rf_2row() {};

  // Instruction format
  using DRRAResource::format;
  void handleCONF(const RF_2ROW_PKG::CONFInstruction &instr);

  using DRRAResource::out;
};

#endif // _RF_2ROW_H
