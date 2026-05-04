#ifndef _ACC_H
#define _ACC_H

#include "acc_pkg.h"
#include "drra_resource.h"
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

class Acc : public DRRAResource {
public:
  /* Element Library Info */
  SST_ELI_REGISTER_COMPONENT(Acc, "drra", "acc",
                             SST_ELI_ELEMENT_VERSION(1, 0, 0),
                             "Accumulator component",
                             COMPONENT_CATEGORY_PROCESSOR)

  /* Element Library Params */
  static std::vector<SST::ElementInfoParam> getComponentParams() {
    auto params = DRRAResource::getBaseParams();
    params.push_back({"bulk_bitwidth", "Width of the bulk output", "64"});
    params.push_back({"ACC_REG_BITWIDTH", "Width of the accumulator register",
                      "64"});
    params.push_back({"IS_SIGNED", "Whether the accumulator is signed", "1"});
    params.push_back({"INCLUDE_MULTIPLIER",
                      "Include vmac/square/capture multiplier circuits", "1"});
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
  Acc(SST::ComponentId_t id, SST::Params &params);

  /* Destructor */
  ~Acc() {};

  bool clockTick(SST::Cycle_t currentCycle) override;

  void handleEventWithSlotID(SST::Event *event, uint32_t slot_id);
  void handleOperation(std::string name, std::function<void(int64_t)> operation);
  void handleVectorOperation(std::string name,
                             std::function<void(std::vector<int64_t>)> operation);
  std::vector<int64_t> byteVectorToInt64Vector(const std::vector<uint8_t> &buf) const;
  void clearAccumulator();
  void emitAccumulator();
  void normalizeAccumulator();
  int64_t getAccumulateRegister() const { return accumulate_register; }
  void setAccumulateRegister(int64_t value) { accumulate_register = value; }

  const std::vector<uint8_t> &getDataBuffer(uint32_t slot_id) const;
  const std::vector<int64_t> &getOperandRegister() const;
  void setOperandRegister(std::vector<int64_t> value);

  // Instruction handlers (called from auto-generated pkg dispatcher)
  using DRRAResource::format;
  void handleACC(const ACC_PKG::ACCInstruction &instr);
  void handleEVT(const ACC_PKG::EVTInstruction &instr);
  void handleREP(const ACC_PKG::REPInstruction &instr);
  void handleREPX(const ACC_PKG::REPXInstruction &instr);
  void handleTRANS(const ACC_PKG::TRANSInstruction &instr);

  void handleActivation(uint32_t slot_id, uint32_t ports) override;

  using DRRAResource::out;
  using DRRAResource::vectorToInt64;
  DRRAOutput &getOutput() { return out; }

  size_t bulk_bitwidth;
  size_t acc_reg_bitwidth;
  bool include_multiplier;

private:
  // Accumulator register, normalized to ACC_REG_BITWIDTH after each update.
  // Stored as a single signed int64 for the model; the emitted payload is
  // sized to bulk_bitwidth.
  int64_t accumulate_register = 0;

  // Operand register for capture/vmac modes: stores the captured vector.
  std::vector<int64_t> operand_register;

  std::unordered_map<ACC_PKG::ACC_MODE, std::function<void()>> accHandlers;
  uint32_t current_fsm = 0;
  std::vector<std::function<void()>> fsmHandlers;

  std::unordered_map<uint32_t, uint32_t> portsToActivate;

  // Helpers for bulk sized byte vectors.
  std::vector<uint8_t> int64ToBulkVector(int64_t data) const;

  std::map<uint32_t, size_t> current_config_option;
  int32_t last_config_level = -1;
  int32_t last_config_trans = -1;
};

#endif // _ACC_H
