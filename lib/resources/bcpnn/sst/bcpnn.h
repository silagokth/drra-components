#ifndef _BCPNN_H
#define _BCPNN_H

#include "bcpnn_pkg.h"
#include "drra_resource.h"

class Bcpnn : public DRRAResource {
public:
  /* Element Library Info */
  SST_ELI_REGISTER_COMPONENT(
    Bcpnn,
    "drra",
    "bcpnn",
    SST_ELI_ELEMENT_VERSION(1, 0, 0),
    "Bcpnn component",
    COMPONENT_CATEGORY_UNCATEGORIZED
  )

  /* Element Library Params */
  static std::vector<SST::ElementInfoParam> getComponentParams() {
    auto params = DRRAResource::getBaseParams();
    params.push_back({"RF_DEPTH", "", "64"});
    params.push_back({"WORD_ADDR_WIDTH", "", "6"});
    params.push_back({"BULK_ADDR_WIDTH", "", "2"});
    return params;
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
  Bcpnn(SST::ComponentId_t id, SST::Params &params);

  /* Destructor */
  ~Bcpnn() {};

  bool clockTick(SST::Cycle_t currentCycle) override;

  // Instruction format
  using DRRAResource::format;
  void handleDSU(const BCPNN_PKG::DSUInstruction &instr);
  void handleREP(const BCPNN_PKG::REPInstruction &instr);
  void handleREPX(const BCPNN_PKG::REPXInstruction &instr);

  using DRRAResource::out;

private:
  // Define private members here
};

#endif // _BCPNN_H