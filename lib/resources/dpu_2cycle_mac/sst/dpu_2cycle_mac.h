#ifndef _DPU_2CYCLE_MAC_H
#define _DPU_2CYCLE_MAC_H

#include "dpu_2cycle_mac_pkg.h"
#include "drra_resource.h"

class Dpu_2cycle_mac : public DRRAResource {
public:
  /* Element Library Info */
  SST_ELI_REGISTER_COMPONENT(Dpu_2cycle_mac, "drra", "dpu_2cycle_mac",
                             SST_ELI_ELEMENT_VERSION(1, 0, 0),
                             "Dpu_2cycle_mac component",
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
  static std::vector<SST::ElementInfoStatistic> getComponentStatistics() {
    auto stats = DRRAResource::getBaseStatistics();
    return stats;
  }
  SST_ELI_DOCUMENT_STATISTICS(getComponentStatistics())

  /* Constructor */
  Dpu_2cycle_mac(SST::ComponentId_t id, SST::Params &params);

  /* Destructor */
  ~Dpu_2cycle_mac() {};

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
  void handleDPU(const DPU_2CYCLE_MAC_PKG::DPUInstruction &instr);
  void handleREP(const DPU_2CYCLE_MAC_PKG::REPInstruction &instr);
  void handleREPX(const DPU_2CYCLE_MAC_PKG::REPXInstruction &instr);
  void handleFSM(const DPU_2CYCLE_MAC_PKG::FSMInstruction &instr);

  using DRRAResource::out;

private:
  std::vector<uint8_t> accumulate_register;

  std::unordered_map<DPU_2CYCLE_MAC_PKG::DPU_MODE, std::function<void()>>
      dpuHandlers;

  uint32_t current_event_number = 0;
  std::vector<std::function<void()>> eventsHandlers;

  uint32_t current_fsm = 0;
  std::vector<std::function<void()>> fsmHandlers;
};

#endif // _DPU_2CYCLE_MAC_H
