#ifndef _IO_MUX_H
#define _IO_MUX_H

#include "drra_resource.h"
#include "io_mux_pkg.h"

#include <string>
#include <unordered_map>
#include <vector>

class Io_mux : public DRRAResource {
public:
  SST_ELI_REGISTER_COMPONENT(Io_mux, "drra", "io_mux",
                             SST_ELI_ELEMENT_VERSION(1, 0, 0),
                             "Io_mux component", COMPONENT_CATEGORY_NETWORK)

  static std::vector<SST::ElementInfoParam> getComponentParams() {
    auto params = DRRAResource::getBaseParams();
    params.push_back({"IO_ADDR_WIDTH", "", "6"});
    params.push_back({"NUM_INPUT_PATTERNS", "Input-side pattern AGUs", "2"});
    params.push_back({"NUM_OUTPUT_PATTERNS", "Output-side pattern AGUs", "1"});
    return params;
  }
  SST_ELI_DOCUMENT_PARAMS(getComponentParams())

  static std::vector<SST::ElementInfoPort> getComponentPorts() {
    auto ports = DRRAResource::getBasePorts();
    return ports;
  }
  SST_ELI_DOCUMENT_PORTS(getComponentPorts())

  static std::vector<SST::ElementInfoStatistic> getComponentStatistics() {
    auto stats = DRRAResource::getBaseStatistics();
    return stats;
  }
  SST_ELI_DOCUMENT_STATISTICS(getComponentStatistics())

  Io_mux(SST::ComponentId_t id, SST::Params &params);
  ~Io_mux() {};

  void complete(unsigned int phase) override {
    logTraceEvent("memory", slot_id, true, 'E', {});
  }

  bool clockTick(SST::Cycle_t currentCycle) override;
  void handleActivation(uint32_t slot_id, uint32_t ports) override;

  using DRRAResource::format;
  void handleDSU(const IO_MUX_PKG::DSUInstruction &instr);
  void handleREP(const IO_MUX_PKG::REPInstruction &instr);
  void handleREPX(const IO_MUX_PKG::REPXInstruction &instr);
  void handleTRANS(const IO_MUX_PKG::TRANSInstruction &instr);

  using DRRAResource::out;

private:
  static constexpr uint32_t INPUT_PORT = 0;
  static constexpr uint32_t OUTPUT_PORT = 1;

  uint32_t num_input_patterns = 2;
  uint32_t num_output_patterns = 1;
  uint32_t num_pattern_agus = 3;
  uint32_t num_selector_agus = 1;
  uint32_t required_physical_agus = 4;

  bool current_target_valid = false;
  uint32_t current_target_agu = 0;

  int64_t read_from_io_address_buffer = -1;
  int64_t write_to_io_address_buffer = -1;

  std::vector<uint8_t> io_input_data_buffer;
  std::vector<uint8_t> io_output_data_buffer;

  uint32_t current_event_number = 0;
  std::unordered_map<uint32_t, uint32_t> portsToActivate;

  bool hasInputSelector() const { return num_input_patterns > 1; }
  bool hasOutputSelector() const { return num_output_patterns > 1; }
  bool hasSelector(uint32_t port) const;

  uint32_t inputPatternBase() const { return 0; }
  uint32_t outputPatternBase() const { return num_input_patterns; }
  uint32_t selectorBase() const { return num_input_patterns + num_output_patterns; }
  uint32_t inputSelectorIndex() const { return selectorBase(); }
  uint32_t outputSelectorIndex() const {
    return selectorBase() + (hasInputSelector() ? 1 : 0);
  }

  uint32_t patternBase(uint32_t port) const;
  uint32_t patternCount(uint32_t port) const;
  uint32_t selectorIndex(uint32_t port) const;
  uint32_t activeRepresentativeAgu(uint32_t port) const;

  bool decodeDSUTarget(uint32_t port, uint32_t agu_idx, uint32_t &physical_agu,
                       bool &is_selector) const;
  void addDSUEvent(uint32_t port, uint32_t physical_agu, bool is_selector);
  void activateMuxPort(uint32_t port);

  uint32_t selectorIndexWidth(uint32_t count) const;
  bool selectedPatternIndex(uint32_t port, uint32_t cycle, uint32_t &pattern_idx);
  int64_t addressForLogicalPort(uint32_t port);
  bool isLogicalPortActive(uint32_t port);

  void readFromIO();
  void writeToIO();
  void bulkOutput();
  void bulkInput();
};

#endif // _IO_MUX_H
