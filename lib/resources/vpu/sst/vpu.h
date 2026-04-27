#ifndef _VPU_H
#define _VPU_H

#include "drra_resource.h"
#include "vpu_pkg.h"

class Vpu : public DRRAResource {
public:
  /* Element Library Info */
  SST_ELI_REGISTER_COMPONENT(Vpu, "drra", "vpu",
                             SST_ELI_ELEMENT_VERSION(1, 0, 0), "Vpu component",
                             COMPONENT_CATEGORY_PROCESSOR)

  /* Element Library Params */
  static std::vector<SST::ElementInfoParam> getComponentParams() {
    auto params = DRRAResource::getBaseParams();
    params.push_back({"FRACTIONAL_BITWIDTH", "", "0"});
    params.push_back({"IS_SIGNED", "", "1"});
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

  void handleEventWithSlotID(SST::Event *event, uint32_t slot_id);

  void handleOperation(std::string name,
                       std::function<std::vector<int64_t>(std::vector<int64_t>,
                                                          std::vector<int64_t>)>
                           operation);

  std::vector<int64_t> &getAccumulateRegister() { return accumulate_register; }

  using DRRAResource::int64ToVector;
  using DRRAResource::uint64ToVector;
  using DRRAResource::vectorToInt64;

  // Instruction format
  using DRRAResource::format;
  void handleVPU(const VPU_PKG::VPUInstruction &instr);
  void handleEVT(const VPU_PKG::EVTInstruction &instr);
  void handleREP(const VPU_PKG::REPInstruction &instr);
  void handleREPX(const VPU_PKG::REPXInstruction &instr);
  void handleTRANS(const VPU_PKG::TRANSInstruction &instr);

  void handleActivation(uint32_t slot_id, uint32_t ports) override;
  std::unordered_map<uint32_t, uint32_t> portsToActivate;

  using DRRAResource::out;
  DRRAOutput &getOutput() { return out; }

  uint32_t fractional_bitwidth;

private:
  std::vector<int64_t> accumulate_register;
  std::vector<int64_t>
  byteVectorToInt64Vector(const std::vector<uint8_t> &byte_vector);
  std::vector<uint8_t>
  int64VectorToByteVector(const std::vector<int64_t> &int_vector);
  std::string int64VectorToString(const std::vector<int64_t> &vec);

  std::unordered_map<VPU_PKG::VPU_MODE, std::function<void()>> vpuHandlers;

  std::vector<std::function<void()>> eventsHandlers;

  uint32_t current_fsm = 0;
  std::vector<std::function<void()>> fsmHandlers;
  std::vector<std::vector<uint8_t>> imm_buffers;

  std::vector<uint8_t> input_latch_0;
  std::vector<uint8_t> input_latch_1;

  std::map<uint32_t, size_t> current_config_option;
  int32_t last_config_level = -1;
  int32_t last_config_trans = -1;
};

#endif // _VPU_H
