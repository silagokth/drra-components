#ifndef _WIN2_H
#define _WIN2_H

#include "drra_resource.h"
#include "win2_pkg.h"
#include <cstdint>
#include <unordered_map>
#include <vector>

class Win2 : public DRRAResource {
public:
  /* Element Library Info */
  SST_ELI_REGISTER_COMPONENT(Win2, "drra", "win2",
                             SST_ELI_ELEMENT_VERSION(1, 0, 0),
                             "win2 component: 2-row sliding-window stream "
                             "aligner with programmable per-cycle lane offset",
                             COMPONENT_CATEGORY_PROCESSOR)

  /* Element Library Params */
  static std::vector<SST::ElementInfoParam> getComponentParams() {
    auto params = DRRAResource::getBaseParams();
    params.push_back({"bulk_bitwidth", "Width of the bulk data bus", "256"});
    params.push_back({"word_bitwidth", "Width of one lane", "16"});
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
  Win2(SST::ComponentId_t id, SST::Params &params);

  /* Destructor */
  ~Win2() {};

  bool clockTick(SST::Cycle_t currentCycle) override;
  void handleActivation(uint32_t slot_id, uint32_t ports) override;

  // Instruction handlers (called from auto-generated pkg dispatcher).
  using DRRAResource::format;
  void handleDSU(const WIN2_PKG::DSUInstruction &instr);
  void handleREP(const WIN2_PKG::REPInstruction &instr);
  void handleREPX(const WIN2_PKG::REPXInstruction &instr);
  void handleTRANS(const WIN2_PKG::TRANSInstruction &instr);

  using DRRAResource::out;

private:
  // AGU indices, matching isa.json port verbo_map.
  static constexpr uint32_t PORT_INPUT  = 0;
  static constexpr uint32_t PORT_OFFSET = 1;

  // Two-line buffer. Byte vectors, little-endian within each line (lane 0 at
  // bytes [0..word_bytes-1]), matching the wire-level convention used by
  // DataEvent payloads across the project.
  std::vector<uint8_t> line_A;
  std::vector<uint8_t> line_B;

  // Sticky lane offset: latched whenever the offset AGU fires, retained
  // between fires so the bulk output is continuously driven (mirrors the
  // RTL's offset_reg).
  uint32_t offset_reg = 0;

  size_t bulk_bitwidth = 256;
  size_t word_bitwidth = 16;
  size_t bulk_bytes    = 32;
  size_t word_bytes    = 2;
  size_t num_lanes     = 16;

  std::unordered_map<uint32_t, uint32_t> portsToActivate;

  uint32_t current_event_number = 0;

  // AGU event lambdas — called by the base class on cycles where the AGU has
  // a valid address. Hold the AGU-gated behavior.
  void absorbAndShift(); // input  AGU: pull bulk_in + shift the 2-line buffer
  void latchOffset();    // offset AGU: latch new offset into offset_reg

  // Per-cycle output (not AGU-gated): runs from clockTick every logical
  // cycle, mirroring the RTL's continuous bulk_data_out_0 assign.
  void emitSlice();

  // Returns a BULK_BITWIDTH-wide slice from {line_B, line_A} starting at
  // `offset` lanes from the LSB of line_A.
  std::vector<uint8_t> takeAlignedSlice(uint32_t offset) const;
};

#endif // _WIN2_H
