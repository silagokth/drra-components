#ifndef _REGISTER_FILE_H
#define _REGISTER_FILE_H

#include "drra_resource.h"
#include <cstdint>
#include <map>
#include <string>
#include <vector>

// The register file model shared by rf and rf_row, the SST twin of
// lib/common/jinja/register_file. The two resources differ only in geometry
// (RF_DEPTH, WORD_ADDR_WIDTH, BULK_ADDR_WIDTH), which arrives as params, so
// everything but the SST registration and the generated instruction package
// lives here. Each resource registers itself under its own name, wires its
// generated instruction handlers, and names its access ports.
class RegisterFile : public DRRAResource {
public:
  // The params both resources take; only the geometry defaults differ.
  static std::vector<SST::ElementInfoParam>
  getRegisterFileParams(const char *rf_depth, const char *word_addr_width,
                        const char *bulk_addr_width) {
    auto params = DRRAResource::getBaseParams();
    params.push_back({"RF_DEPTH", "", rf_depth});
    params.push_back({"WORD_ADDR_WIDTH", "", word_addr_width});
    params.push_back({"BULK_ADDR_WIDTH", "", bulk_addr_width});
    return params;
  }

  RegisterFile(SST::ComponentId_t id, SST::Params &params);
  ~RegisterFile() {};

  void complete(unsigned int phase) override {
    logTraceEvent("registers", slot_id, true, 'E', {});
  }

protected:
  // The port numbers of the four access paths, from the resource's generated
  // package. They are part of the contract with the RTL and pasm, so each
  // resource states them rather than this class assuming them.
  struct AccessPorts {
    uint32_t word_write;
    uint32_t word_read;
    uint32_t bulk_write;
    uint32_t bulk_read;
  };
  void registerAccessPorts(const AccessPorts &ports);

  // CONF initialises one register directly from the instruction stream.
  void conf(uint32_t slot, uint64_t address, uint64_t value);

private:
  // Register File
  uint32_t register_file_size;
  std::string access_time;
  std::map<uint32_t, std::vector<uint8_t>> registers;

  // Datapath. Each takes the address its AGU generated for this cycle.
  void readWide(int64_t address);
  void readNarrow(int64_t address);
  void writeWide(int64_t address);
  void writeNarrow(int64_t address);

  void logRegisters();
};

#endif // _REGISTER_FILE_H
