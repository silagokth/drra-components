#ifndef _DPU_H
#define _DPU_H

#include "dpu_pkg.h"
#include "drra_resource.h"

class Dpu : public DRRAResource {
public:
  /* Element Library Info */
  SST_ELI_REGISTER_COMPONENT(Dpu, "drra", "dpu",
                             SST_ELI_ELEMENT_VERSION(1, 0, 0), "Dpu component",
                             COMPONENT_CATEGORY_PROCESSOR)

  /* Element Library Params */
  static std::vector<SST::ElementInfoParam> getComponentParams() {
    auto params = DRRAResource::getBaseParams();
    params.push_back({"FRACTIONAL_BITWIDTH",
                      "Number of fractional bits for fixed-point operations",
                      "0"});
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
  Dpu(SST::ComponentId_t id, SST::Params &params);

  /* Destructor */
  ~Dpu() {};

  bool clockTick(SST::Cycle_t currentCycle) override;
  void handleEventWithSlotID(SST::Event *event, uint32_t slot_id);

  void handleOperation(std::string name,
                       std::function<int64_t(int64_t, int64_t)> operation);

  std::vector<uint8_t> &getAccumulateRegister() { return accumulate_register; }

  using DRRAResource::int64ToVector;
  using DRRAResource::uint64ToVector;
  using DRRAResource::vectorToInt64;

  // Instruction format
  using DRRAResource::format;
  void handleEVT(const DPU_PKG::EVTInstruction &instr);
  void handleREP(const DPU_PKG::REPInstruction &instr);
  void handleREPX(const DPU_PKG::REPXInstruction &instr);
  void handleTRANS(const DPU_PKG::TRANSInstruction &instr);
  void handleDPU(const DPU_PKG::DPUInstruction &instr);

  void handleActivation(uint32_t slot_id, uint32_t ports) override;
  std::unordered_map<uint32_t, uint32_t> portsToActivate;

  using DRRAResource::out;
  uint32_t fractional_bitwidth;

private:
  void refreshAllFsmHandlers();

  std::vector<uint8_t> accumulate_register;

  std::unordered_map<DPU_PKG::DPU_MODE, std::function<void()>> dpuHandlers;

  std::vector<std::function<void()>> eventsHandlers;

  uint32_t current_fsm = 0;
  std::vector<DPU_PKG::DPU_MODE> fsmModes;
  std::vector<std::function<void()>> fsmHandlers;
  std::vector<std::vector<uint8_t>> imm_buffers;

  std::map<uint32_t, size_t> current_config_option;
  int32_t last_config_level = -1;
  int32_t last_config_trans = -1;
};

#endif // _DPU_H
