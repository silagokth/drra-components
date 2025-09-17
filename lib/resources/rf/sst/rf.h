#ifndef _RF_H
#define _RF_H

#include "drra_resource.h"
#include "rf_pkg.h"
#include <cstdint>
#include <map>
#include <vector>

class Rf : public DRRAResource {
public:
  /* Element Library Info */
  SST_ELI_REGISTER_COMPONENT(Rf, "drra", "rf", SST_ELI_ELEMENT_VERSION(1, 0, 0),
                             "Rf component", COMPONENT_CATEGORY_MEMORY)

  /* Element Library Params */
  static std::vector<SST::ElementInfoParam> getComponentParams() {
    auto params = DRRAResource::getBaseParams();
    params.push_back({"RF_DEPTH", "", "64"});
    params.push_back({"WORD_ADDR_WIDTH", "", "6"});
    params.push_back({"BULK_ADDR_WIDTH", "", "2"});
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
  Rf(SST::ComponentId_t id, SST::Params &params);

  /* Destructor */
  ~Rf() {};

  bool clockTick(SST::Cycle_t currentCycle) override;

  // Instruction format
  using DRRAResource::format;
  void handleDSU(const RF_PKG::DSUInstruction &instr);
  void handleREP(const RF_PKG::REPInstruction &instr);
  void handleREPX(const RF_PKG::REPXInstruction &instr);

  using DRRAResource::out;

private:
  // Define private members here
  // Register File
  uint32_t register_file_size;
  std::string access_time;
  std::map<uint32_t, std::vector<uint8_t>> registers;

  void readWide();
  void readNarrow();
  void writeWide();
  void writeNarrow();

  uint32_t current_event_number = 0;
  std::map<uint32_t, uint32_t> port_agus_init;
  std::map<uint32_t, uint32_t> port_agus;

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
