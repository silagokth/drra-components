#ifndef _DPU_H
#define _DPU_H

#include "dpu_pkg.h"
#include "drra_resource.h"

class DPU : public DRRAResource {
public:
  /* Element Library Info */
  SST_ELI_REGISTER_COMPONENT(DPU, "drra", "dpu",
                             SST_ELI_ELEMENT_VERSION(1, 0, 0), "DPU component",
                             COMPONENT_CATEGORY_PROCESSOR)

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
  static const std::vector<SST::ElementInfoStatistic> getComponentStatistics() {
    auto stats = DRRAResource::getBaseStatistics();
    return stats;
  }
  SST_ELI_DOCUMENT_STATISTICS(getComponentStatistics())
  SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS()

  DPU(SST::ComponentId_t id, SST::Params &params);

  bool clockTick(SST::Cycle_t currentCycle) override;
  void handleEventWithSlotID(SST::Event *event, uint32_t slot_id);

  void handleOperation(std::string name,
                       std::function<int64_t(int64_t, int64_t)> operation);

  std::vector<uint8_t> &getAccumulateRegister() { return accumulate_register; }

  using DRRAResource::int64ToVector;
  using DRRAResource::uint64ToVector;
  using DRRAResource::vectorToInt64;

  using DRRAResource::format;
  void handleREP(const DPU_PKG::REPInstruction &rep);
  void handleREPX(const DPU_PKG::REPXInstruction &repx);
  void handleFSM(const DPU_PKG::FSMInstruction &fsm);
  void handleDPU(const DPU_PKG::DPUInstruction &dpu);

  using DRRAResource::out;

private:
  std::vector<uint8_t> accumulate_register;

  std::unordered_map<DPU_PKG::DPU_MODE, std::function<void()>> dpuHandlers;

  uint32_t current_event_number = 0;
  std::vector<std::function<void()>> eventsHandlers;

  uint32_t current_fsm = 0;
  std::vector<std::function<void()>> fsmHandlers;
};

#endif // _DPU_H
