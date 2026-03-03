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

  bool clockTick(SST::Cycle_t currentCycle) override;
  void handleActivation(uint32_t slot_id, uint32_t ports) override;

  std::unordered_map<uint32_t, uint32_t> portsToActivate;

  // Instruction format
  using DRRAResource::format;
  void handleDSU(const IOSRAM_TOP_PKG::DSUInstruction &instr);
  void handleREP(const IOSRAM_TOP_PKG::REPInstruction &instr);
  void handleREPX(const IOSRAM_TOP_PKG::REPXInstruction &instr);
  void handleTRANS(const IOSRAM_TOP_PKG::TRANSInstruction &instr);

  using DRRAResource::out;

private:
  enum DSU_RELATIVE_PORT {
    DSU_PORT_SRAM_READ_FROM_IO = IOSRAM_TOP_PKG::DSU_PORT_INPUT_BUFFER,
    DSU_PORT_SRAM_WRITE_TO_IO = IOSRAM_TOP_PKG::DSU_PORT_OUTPUT_BUFFER,
    DSU_PORT_IO_WRITE_TO_SRAM = IOSRAM_TOP_PKG::DSU_PORT_SRAM_WRITE,
    DSU_PORT_IO_READ_FROM_SRAM = IOSRAM_TOP_PKG::DSU_PORT_SRAM_READ,
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

  SST::Link *self_link = nullptr;
  int64_t sram_read_from_io_address_buffer = -1;
  int64_t sram_read_from_io_initial_addr = -1;
  int64_t sram_write_to_io_address_buffer = -1;
  int64_t sram_write_to_io_initial_addr = -1;
  std::vector<uint8_t> from_io_data_buffer;
  std::vector<uint8_t> to_io_data_buffer;
  int64_t io_write_to_sram_address_buffer = -1;
  int64_t io_write_to_sram_initial_addr = -1;
  int64_t io_read_from_sram_address_buffer = -1;
  int64_t io_read_from_sram_initial_addr = -1;

  // Bulk read/write
  int64_t read_bulk_address_buffer = -1;
  int64_t write_bulk_address_buffer = -1;
  int64_t read_bulk_initial_addr = -1;
  int64_t write_bulk_initial_addr = -1;

  void readFromIO();
  void writeToIO();
  void writeToSRAM();
  void readFromSRAM();
  void writeBulk();
  void readBulk();

  int32_t agu_initial_addr = -1;
  uint32_t current_event_number = 0;
  std::map<uint32_t, size_t> current_option_config;
};

#endif // _IOSRAM_TOP_H
