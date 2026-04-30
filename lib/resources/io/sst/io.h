#ifndef _IO_H
#define _IO_H

#include "drra_resource.h"
#include "io_pkg.h"

class ScratchBackendConvertor;

class Io : public DRRAResource {
public:
  /* Element Library Info */
  SST_ELI_REGISTER_COMPONENT(Io, "drra", "io", SST_ELI_ELEMENT_VERSION(1, 0, 0),
                             "Io component", COMPONENT_CATEGORY_NETWORK)

  /* Element Library Params */
  static std::vector<SST::ElementInfoParam> getComponentParams() {
    auto params = DRRAResource::getBaseParams();
    params.push_back({"IO_ADDR_WIDTH", "", "6"});
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
  Io(SST::ComponentId_t id, SST::Params &params);

  /* Destructor */
  ~Io() {};

  void complete(unsigned int phase) override {
    logTraceEvent("memory", slot_id, true, 'E', {});
  }

  bool clockTick(SST::Cycle_t currentCycle) override;
  void handleActivation(uint32_t slot_id, uint32_t ports) override;

  // Instruction format
  using DRRAResource::format;
  void handleDSU(const IO_PKG::DSUInstruction &instr);
  void handleREP(const IO_PKG::REPInstruction &instr);
  void handleREPX(const IO_PKG::REPXInstruction &instr);
  void handleTRANS(const IO_PKG::TRANSInstruction &instr);

  using DRRAResource::out;

private:
  SST::Link *self_link = nullptr;
  int64_t read_from_io_address_buffer = -1;
  int64_t write_to_io_address_buffer = -1;

  // Separate input/output staging buffers so that simultaneous use of
  // DSU_PORT_INPUT_BUFFER (read path) and DSU_PORT_OUTPUT_BUFFER (write path)
  // does not race on a shared variable.
  // - Input path: readFromIO() -> bulkOutput() stages the IO response in
  //   io_input_data_buffer before forwarding on the bulk output port.
  // - Output path: bulkInput() stages the bulk-port payload in
  //   io_output_data_buffer; writeToIO() then forwards it to the IO subsystem.
  std::vector<uint8_t> io_input_data_buffer;
  std::vector<uint8_t> io_output_data_buffer;

  void readFromIO();
  void writeToIO();
  void bulkOutput();
  void bulkInput();

  uint32_t current_event_number = 0;
  std::map<uint32_t, size_t> current_option_config;
  std::map<uint32_t, uint32_t> port_agus_init;
  std::map<uint32_t, uint32_t> port_agus;

  std::unordered_map<uint32_t, uint32_t> portsToActivate;

  void updatePortAGUs(uint32_t port) {
    int64_t address_offset =
        agus[port].getAddressForCycle(getPortActiveCycle(port));
    if (address_offset < 0) {
      out.fatal(CALL_INFO, -1,
                "AGU for port %u returned negative address %d for cycle %d\n",
                port, address_offset, getPortActiveCycle(port));
    }
    port_agus[port] = port_agus_init[port] + address_offset;
  }
};

#endif // _IO_H
