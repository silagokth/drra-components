#ifndef _VPU_H
#define _VPU_H

#include "vpu_pkg.h"
#include "drra_resource.h"

class Vpu : public DRRAResource {
public:
  /* Element Library Info */
  SST_ELI_REGISTER_COMPONENT(
    Vpu,
    "drra",
    "vpu",
    SST_ELI_ELEMENT_VERSION(1, 0, 0),
    "Vpu component",
    COMPONENT_CATEGORY_PROCESSOR
  )

  /* Element Library Params */
  static std::vector<SST::ElementInfoParam> getComponentParams() {
    auto params = DRRAResource::getBaseParams();
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
  Vpu(SST::ComponentId_t id, SST::Params &params);

  /* Destructor */
  ~Vpu() {};

  bool clockTick(SST::Cycle_t currentCycle) override;

  // Instruction format
  using DRRAResource::format;
  void handleVPU(const VPU_PKG::VPUInstruction &instr);
  void handleREP(const VPU_PKG::REPInstruction &instr);
  void handleTRANS(const VPU_PKG::TRANSInstruction &instr);

  using DRRAResource::out;

private:
  // Define private members here
};

#endif // _VPU_H