#ifndef _RF_H
#define _RF_H

#include "register_file.h"
#include "rf_pkg.h"
#include <vector>

// The 64-word, four-row register file. The model itself is RegisterFile
// (lib/common/sst), shared with rf_row.
class Rf : public RegisterFile {
public:
  /* Element Library Info */
  SST_ELI_REGISTER_COMPONENT(Rf, "drra", "rf", SST_ELI_ELEMENT_VERSION(1, 0, 0),
                             "Rf component", COMPONENT_CATEGORY_MEMORY)

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
  Rf(SST::ComponentId_t id, SST::Params &params);

  /* Destructor */
  ~Rf() {};

  // Instruction format
  using DRRAResource::format;
  void handleCONF(const RF_PKG::CONFInstruction &instr);

  using DRRAResource::out;
};

#endif // _RF_H
