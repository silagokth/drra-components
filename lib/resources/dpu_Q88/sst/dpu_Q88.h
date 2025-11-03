#ifndef _DPU_Q88_H
#define _DPU_Q88_H

#include "dpu_Q88_pkg.h"
#include "drra_resource.h"

class Dpu_q88 : public DRRAResource {
public:
  /* Element Library Info */
  SST_ELI_REGISTER_COMPONENT(Dpu_q88, "drra", "dpu_Q88",
                             SST_ELI_ELEMENT_VERSION(1, 0, 0),
                             "Dpu_q88 component", COMPONENT_CATEGORY_PROCESSOR)

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
  Dpu_q88(SST::ComponentId_t id, SST::Params &params);

  /* Destructor */
  ~Dpu_q88() {};

  bool clockTick(SST::Cycle_t currentCycle) override;
  void handleEventWithSlotID(SST::Event *event, uint32_t slot_id);

  void handleOperation(std::string name,
                       std::function<int16_t(int16_t, int16_t)> operation);

  int32_t &getAccumulateRegister() { return accumulate_register; }

  // Instruction format
  using DRRAResource::format;
  void handleDPU(const DPU_Q88_PKG::DPUInstruction &instr);
  void handleREP(const DPU_Q88_PKG::REPInstruction &instr);
  void handleREPX(const DPU_Q88_PKG::REPXInstruction &instr);
  void handleFSM(const DPU_Q88_PKG::FSMInstruction &instr);

  using DRRAResource::out;

private:
  int32_t accumulate_register;

  std::unordered_map<DPU_Q88_PKG::DPU_MODE, std::function<void()>> dpuHandlers;

  uint32_t current_event_number = 0;
  std::vector<std::function<void()>> eventsHandlers;

  uint32_t current_fsm = 0;
  std::vector<std::function<void()>> fsmHandlers;
};

#endif // _DPU_Q88_H
