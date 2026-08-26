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

  // Instruction format
  using DRRAResource::format;
  void handleCONF(const IO_PKG::CONFInstruction &instr);

  using DRRAResource::out;

private:
  SST::Link *self_link = nullptr;

  // Separate input/output staging buffers so that simultaneous use of
  // EVT_PORT_INPUT_BUFFER (read path) and EVT_PORT_OUTPUT_BUFFER (write path)
  // does not race on a shared variable.
  // - Input path: readFromIO() -> bulkOutput() stages the IO response in
  //   io_input_data_buffer before forwarding on the bulk output port.
  // - Output path: bulkInput() stages the bulk-port payload in
  //   io_output_data_buffer; writeToIO() then forwards it to the IO subsystem.
  std::vector<uint8_t> io_input_data_buffer;
  std::vector<uint8_t> io_output_data_buffer;

  // Datapath. Each takes the address its AGU generated for this cycle.
  //
  // bulkOutput and bulkInput are the second half of each port's transfer, so
  // they are registered as port actions too. Being driven by addr_valid is
  // what keeps them out of the gap cycles between outer repetitions, where the
  // port is still active but the AGU produces no address -- firing them there
  // used to recv() data that was never requested.
  void readFromIO(int64_t address);
  void writeToIO(int64_t address);
  void bulkOutput(int64_t address);
  void bulkInput(int64_t address);
};

#endif // _IO_H
