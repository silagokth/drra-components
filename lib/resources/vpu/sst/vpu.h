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

  void handleEventWithSlotID(SST::Event *event, uint32_t slot_id);

  void handleOperation(std::string name,
                       std::function<std::vector<int64_t>(std::vector<int64_t>,
                                                          std::vector<int64_t>)>
                           operation,
                       bool use_immediate = false);

  // *_imm modes take their second operand from the VPU instruction's
  // `immediate` field (see rtl/alu_wrapper.sv.j2), not from the in1 latch.
  void handleImmOperation(std::string name,
                          std::function<std::vector<int64_t>(
                              std::vector<int64_t>, std::vector<int64_t>)>
                              operation) {
    handleOperation(std::move(name), std::move(operation), true);
  }

  std::vector<int64_t> &getAccumulateRegister() { return accumulate_register; }

  using DRRAResource::int64ToVector;
  using DRRAResource::uint64ToVector;
  using DRRAResource::vectorToInt64;

  // Instruction format
  using DRRAResource::format;
  void handleVPU(const VPU_PKG::VPUInstruction &instr);

  // The vpu ISA keeps REPX as its own opcode instead of an `ext` bit on REP;
  // it is the ext = 1 half of the base REP.
  void handleREPX(const VPU_PKG::REPXInstruction &instr);

  // VPU drives its FSM/output every cycle, like the dpu, so it never
  // idle-skips.
  bool isIdle() override { return false; }

  using DRRAResource::out;
  DRRAOutput &getOutput() { return out; }

  uint32_t fractional_bitwidth;

private:
  // The datapath is not driven by an AGU address: it runs the configured
  // operation every cycle and only *selects* which one from the AGU.
  void logic(uint32_t subcycle) override;

  std::vector<int64_t> accumulate_register;
  std::vector<int64_t>
  byteVectorToInt64Vector(const std::vector<uint8_t> &byte_vector);
  std::vector<uint8_t>
  int64VectorToByteVector(const std::vector<int64_t> &int_vector);
  std::string int64VectorToString(const std::vector<int64_t> &vec);

  std::unordered_map<VPU_PKG::VPU_MODE, std::function<void()>> vpuHandlers;

  uint32_t current_fsm = 0;
  std::vector<std::function<void()>> fsmHandlers;
  std::vector<std::vector<uint8_t>> imm_buffers;

  std::vector<uint8_t> input_latch_0;
  std::vector<uint8_t> input_latch_1;
};

#endif // _VPU_H
