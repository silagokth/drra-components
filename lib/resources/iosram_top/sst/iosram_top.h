#ifndef _IOSRAM_TOP_H
#define _IOSRAM_TOP_H

#include "drra_resource.h"
#include "iosram_top_pkg.h"

#include "sst/elements/memHierarchy/membackend/backing.h"

class ScratchBackendConvertor;

class Iosram_top : public DRRAResource {
public:
  /* Element Library Info */
  SST_ELI_REGISTER_COMPONENT(Iosram_top, "drra", "iosram_top",
                             SST_ELI_ELEMENT_VERSION(1, 0, 0),
                             "Iosram_top component", COMPONENT_CATEGORY_MEMORY)

  /* Element Library Params */
  static std::vector<SST::ElementInfoParam> getComponentParams() {
    auto params = DRRAResource::getBaseParams();
    params.push_back({"SRAM_ADDR_WIDTH", "", "6"});
    params.push_back({"access_time", "Time to access the IO buffer", "0ns"});
    params.push_back(
        {"backing", "Type of backing store (malloc, mfile)", "malloc"});
    params.push_back(
        {"backing_size_unit", "Size of the backing store", "1MiB"});
    params.push_back({"memory_file", "Memory file for mfile backing", ""});
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
  Iosram_top(SST::ComponentId_t id, SST::Params &params);

  /* Destructor */
  ~Iosram_top() {
    if (backend)
      delete backend;
  };

  void complete(unsigned int phase) override {
    logTraceEvent("memory", slot_id, true, 'E', {});
  }

  // Instruction format
  using DRRAResource::format;
  void handleCONF(const IOSRAM_TOP_PKG::CONFInstruction &instr);

  using DRRAResource::out;

private:
  // The AGU index each of this resource's transfers uses. Ports 0..3 are
  // slot 0's, ports 4..7 slot 1's, so the bulk pair is slot 1 ports 2 and 3 --
  // the same (slot, port) pairs the RTL top maps onto its four AGUs.
  enum DSU_RELATIVE_PORT {
    DSU_PORT_SRAM_READ_FROM_IO = IOSRAM_TOP_PKG::EVT_PORT_INPUT_BUFFER,
    DSU_PORT_SRAM_WRITE_TO_IO = IOSRAM_TOP_PKG::EVT_PORT_OUTPUT_BUFFER,
    DSU_PORT_IO_WRITE_TO_SRAM = IOSRAM_TOP_PKG::EVT_PORT_SRAM_WRITE,
    DSU_PORT_IO_READ_FROM_SRAM = IOSRAM_TOP_PKG::EVT_PORT_SRAM_READ,
    DSU_PORT_WRITE_BULK = 6,
    DSU_PORT_READ_BULK = 7
  };

  std::string access_time;

  std::string dumpBackendContent();
  SST::MemHierarchy::Backend::Backing *backend = nullptr;
  ScratchBackendConvertor *backendConvertor = nullptr;

  // Backing store parameters
  uint64_t iosram_depth;
  bool read_only;

  // Staging buffers shared between the two halves of a transfer.
  std::vector<uint8_t> from_io_data_buffer;
  std::vector<uint8_t> to_io_data_buffer;

  // Datapath. Each takes the address its AGU generated for this cycle.
  void readFromIO(int64_t address);
  void writeToIO(int64_t address);
  void writeToSRAM(int64_t address);
  void readFromSRAM(int64_t address);
  void writeBulk(int64_t address);
  void readBulk(int64_t address);
};

#endif // _IOSRAM_TOP_H
