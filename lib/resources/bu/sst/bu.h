#ifndef _BU_H
#define _BU_H

#include "drra.h" // Contains DRRAResource class definition

class BU : public DRRAResource {
public:
  /* SST Element Library Info */
  SST_ELI_REGISTER_COMPONENT(BU, "drra", "bu", SST_ELI_ELEMENT_VERSION(1, 0, 0),
                             "BU component", COMPONENT_CATEGORY_PROCESSOR)

  /* SST Element Library Params */
  static std::vector<SST::ElementInfoParam> getComponentParams() {
    auto params = DRRAResource::getBaseParams();
    return params;
  }
  SST_ELI_DOCUMENT_PARAMS(getComponentParams())

  /* SST Element Library Ports */
  static std::vector<SST::ElementInfoPort> getComponentPorts() {
    auto ports = DRRAResource::getBasePorts();
    // Additional ports can be added here
    // ports.push_back({"example_port", "Example port description"});
    return ports;
  }
  SST_ELI_DOCUMENT_PORTS(getComponentPorts())

  /* SST Element Library Statistics */
  SST_ELI_DOCUMENT_STATISTICS()

  /* SST Element Library Subcomponents */
  SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS()

  /* Constructor */
  BU(SST::ComponentId_t id, SST::Params &params);

  /* Destructor */
  ~BU() {};

  /* SST lifecycle methods */
  virtual void init(unsigned int phase) override;
  virtual void setup() override;
  virtual void complete(unsigned int phase) override;
  virtual void finish() override;

  /* SST Clock Handler Function */
  bool clockTick(SST::Cycle_t currentCycle) override;

private:
  /* Instruction decoding */
  void decodeInstr(uint32_t instr) override;
  enum OpCode { FSM = 2, BU_OP = 3 };
  void handleFSM(uint32_t instr);
  void handleBU(uint32_t instr);
  enum Radixes { RADIX_2, RADIX_4 };
  enum Decimation { DIT, DIF };

  /* Timing model variables */
  uint32_t current_event_number = 0;

  /* FSM variables */
  static const uint32_t num_fsms = 4; // Number of FSMs
  uint32_t current_fsm = 0;
  std::function<void()> fsmHandlers[num_fsms];

  std::function<void()> getBUHandler(Radixes radix, Decimation decimation) {
    return {};
  }
};

#endif // _BU_H
