#ifndef _SEQUENCER_H
#define _SEQUENCER_H

#include "drra_controller.h"
#include "sequencer_pkg.h"

class Sequencer : public DRRAController {
public:
  /* Element Library Info */
  SST_ELI_REGISTER_COMPONENT(Sequencer, "drra", "sequencer",
                             SST_ELI_ELEMENT_VERSION(1, 0, 0),
                             "Sequencer component",
                             COMPONENT_CATEGORY_UNCATEGORIZED)

  /* Element Library Params */
  static std::vector<SST::ElementInfoParam> getComponentParams() {
    auto params = DRRAController::getBaseParams();
    params.push_back({"NUM_SLOTS", "", "16"});
    params.push_back({"NUM_SCALAR_REGS", "", "16"});
    params.push_back({"SCALAR_REG_WIDTH", "", "16"});
    params.push_back({"IRAM_DEPTH", "", "64"});
    params.push_back(
        {"assembly_program_path", "Path to the assembly program file", ""});
    params.push_back({"instr_addr_width", "Instruction address width", "6"});
    params.push_back({"instr_hops_width", "Instruction hops width", "4"});
    return params;
  }
  SST_ELI_DOCUMENT_PARAMS(getComponentParams())

  /* Element Library Ports */
  static std::vector<SST::ElementInfoPort> getComponentPorts() {
    auto ports = DRRAController::getBasePorts();
    return ports;
  }
  SST_ELI_DOCUMENT_PORTS(getComponentPorts())

  /* Element Library Statistics */
  static std::vector<SST::ElementInfoStatistic> getComponentStatistics() {
    auto stats = DRRAController::getBaseStatistics();
    return stats;
  }
  SST_ELI_DOCUMENT_STATISTICS(getComponentStatistics())

  /* Constructor */
  Sequencer(SST::ComponentId_t id, SST::Params &params);

  /* Destructor */
  ~Sequencer() {};

  // SST lifecycle methods
  virtual void init(unsigned int phase) override;

  bool clockTick(SST::Cycle_t currentCycle) override;

  // Instruction format
  using DRRAController::format;
  void handleHALT(const SEQUENCER_PKG::HALTInstruction &instr);
  void handleWAIT(const SEQUENCER_PKG::WAITInstruction &instr);
  void handleACT(const SEQUENCER_PKG::ACTInstruction &instr);
  void handleCALC(const SEQUENCER_PKG::CALCInstruction &instr);
  void handleBRN(const SEQUENCER_PKG::BRNInstruction &instr);

  using DRRAController::out;

private:
  bool readyToFinish = false;

  // uint32_t cell_coordinates[2] = {0, 0};
  uint32_t cyclesToWait = 0;

  std::string assemblyProgramPath;
  std::vector<uint32_t> assemblyProgram;

  // Params (from arch.json file)
  uint32_t resourceInstrWidth;
  uint32_t fsmPerSlot;
  uint32_t instrDataWidth;
  uint32_t instrAddrWidth;
  uint32_t instrHopsWidth;
  uint32_t registerSize;
  uint32_t numRegisters;

  // Add scalar and bool registers
  std::vector<uint32_t> scalarRegisters;

  // Add fetch_decode method
  void fetch_decode(uint32_t instruction);
  void load_assembly_program(std::string);

  // Helper methods
  void sendActEvent(uint32_t, uint32_t);
  void handle_continuous_port_mode(uint32_t ports, uint32_t param);
  void handle_all_port_X_mode(uint32_t ports, uint32_t param);
  void handle_activation_vector_mode(uint32_t ports, uint32_t param);
};

#endif // _SEQUENCER_H
