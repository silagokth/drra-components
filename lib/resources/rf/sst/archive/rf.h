#ifndef _RF_H
#define _RF_H

#include "drra_resource.h"

using namespace std;
using namespace SST;

class RegisterFile : public DRRAResource {
public:
  /* Element Library Info */
  SST_ELI_REGISTER_COMPONENT(RegisterFile, // Class name
                             "drra",       // Name of library
                             "rf",         // Lookup name for component
                             SST_ELI_ELEMENT_VERSION(1, 0,
                                                     0), // Component version
                             "RegisterFile component",   // Description
                             COMPONENT_CATEGORY_MEMORY   // Category
  )

  /* Element Library Params */
  static vector<ElementInfoParam> getComponentParams() {
    auto params = DRRAResource::getBaseParams();
    params.push_back({"access_time", "Time to access the IO buffer", "0ns"});
    params.push_back({"register_file_size",
                      "Size of the register file in number of words", "1024"});
    return params;
  }
  SST_ELI_DOCUMENT_PARAMS(getComponentParams())

  /* Element Library Ports */
  static vector<ElementInfoPort> getComponentPorts() {
    auto ports = DRRAResource::getBasePorts();
    return ports;
  }
  SST_ELI_DOCUMENT_PORTS(getComponentPorts())
  SST_ELI_DOCUMENT_STATISTICS()
  SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS()

  /* Constructor */
  RegisterFile(ComponentId_t id, Params &params);

private:
  // Supported opcodes
  enum OpCode { REP = 0, REPX = 1, DSU = 6 };
  inline const map<OpCode, vector<SegmentRange>> &getIsaDefinitions() {
    static const map<OpCode, vector<SegmentRange>> segmentsDef = {
        {OpCode::REP,
         {SegmentRange("port", 2, 22), SegmentRange("level", 4, 18),
          SegmentRange("iter", 6, 12), SegmentRange("step", 6, 6),
          SegmentRange("delay", 6, 0)}},
        {OpCode::REPX,
         {SegmentRange("port", 2, 22), SegmentRange("level", 4, 18),
          SegmentRange("iter_msb", 6, 12), SegmentRange("step_msb", 6, 6),
          SegmentRange("delay_msb", 6, 0)}},
        {OpCode::DSU,
         {SegmentRange("init_addr_sd", 1, 23), SegmentRange("init_addr", 16, 7),
          SegmentRange("port", 2, 5)}}};
    return segmentsDef;
  }

  struct REPInstruction {
    uint32_t slot, port, level, iter, step, delay;
    REPInstruction(Instruction instr) {
      slot = instr.slot;
      port = instr.get("port").value;
      level = instr.get("level").value;
      iter = instr.get("iter").value;
      step = instr.get("step").value;
      delay = instr.get("delay").value;
    }
  };

  struct REPXInstruction {
    uint32_t slot, port, level, iter_msb, step_msb, delay_msb;
    REPXInstruction(Instruction instr) {
      slot = instr.slot;
      port = instr.get("port").value;
      level = instr.get("level").value;
      iter_msb = instr.get("iter_msb").value;
      step_msb = instr.get("step_msb").value;
      delay_msb = instr.get("delay_msb").value;
    }
  };

  struct DSUInstruction {
    uint32_t slot, init_addr_sd, init_addr, port;
    DSUInstruction(Instruction instr) {
      slot = instr.slot;
      init_addr_sd = instr.get("init_addr_sd").value;
      init_addr = instr.get("init_addr").value;
      port = instr.get("port").value;
    }
  };
  void handleREP(REPInstruction rep);
  void handleREPX(REPXInstruction repx);
  void handleDSU(DSUInstruction dsu);

  // Register File
  uint32_t register_file_size;
  std::string access_time;
  map<uint32_t, vector<uint8_t>> registers;

  void readWide();
  void readNarrow();
  void writeWide();
  void writeNarrow();

  uint32_t current_event_number = 0;
  map<uint32_t, uint32_t> port_agus_init;
  map<uint32_t, uint32_t> port_agus;

  void updatePortAGUs(uint32_t port) {
    port_agus[port] = port_agus_init[port] +
                      current_timing_states[port].getRepIncrementForCycle(
                          getPortActiveCycle(port));
    if (port_agus[port] >= register_file_size) {
      out.fatal(CALL_INFO, -1, "Invalid AGU address (greater than RF size)\n");
    }
  }
};

#endif // _RF_H
