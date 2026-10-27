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
    params.push_back({"SATURATE",
                      "1: products and sums saturate to the word; 0: they wrap",
                      "1"});
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

  void handleEventWithSlotID(SST::Event *event, uint32_t slot_id);

  void handleOperation(std::string name,
                       std::function<int64_t(int64_t, int64_t)> operation);

  // This cycle's operands, from the two slots' buffers.
  void readOperands(int64_t &data0, int64_t &data1);

  // Stage 1 of MAC: the product is added to the accumulator next cycle, in
  // stage 2 (finishMAC), mirroring the RTL's registered product.
  void issueMACProduct(int64_t product);

  // Sends a word on the second slot's output (the RTL's word_data_out_1).
  void sendOut1(int64_t value);

  std::vector<uint8_t> &getAccumulateRegister() { return accumulate_register; }

  using DRRAResource::int64ToVector;
  using DRRAResource::vectorToInt64;

  // Instruction format
  using DRRAResource::format;
  void handleCONF(const DPU_PKG::CONFInstruction &instr);

  // DPU drives its FSM/output every active cycle; the idle-skip pause/resume
  // desyncs that output, so it never idle-skips.
  bool isIdle() override { return false; }

  using DRRAResource::out;
  uint32_t fractional_bitwidth;
  bool saturate;

private:
  // The DPU's datapath is not driven by an AGU address: it runs its configured
  // operation on every cycle and only *selects* which one from the AGU.
  void logic(uint32_t subcycle) override;

  // Stage 2 of MAC: adds last cycle's product to the accumulator and drives
  // the sum out.
  void finishMAC();

  std::vector<uint8_t> accumulate_register;

  std::unordered_map<DPU_PKG::CONF_MODE, std::function<void()>> dpuHandlers;

  uint32_t current_fsm = 0;
  std::vector<std::function<void()>> fsmHandlers;
  // The mode each FSM's handler runs, to tell MAC's two stages apart.
  std::vector<DPU_PKG::CONF_MODE> fsmModes;

  // The RTL's MAC pipeline register (logic.sv.j2: mac_q, mult_out_q,
  // reset_accumulator_q).
  bool mac_pending = false;
  int64_t mac_product = 0;
  bool mac_reset_pending = false;
  // Set by the rst port this cycle; logic() decides which stage it clears for.
  bool reset_requested = false;
};

#endif // _DPU_H
