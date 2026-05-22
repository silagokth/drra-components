#ifndef _NCC_CMP_H
#define _NCC_CMP_H

#include "drra_resource.h"
#include "ncc_cmp_pkg.h"
#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

class Ncc_cmp : public DRRAResource {
public:
  /* Element Library Info */
  SST_ELI_REGISTER_COMPONENT(Ncc_cmp, "drra", "ncc_cmp",
                             SST_ELI_ELEMENT_VERSION(1, 0, 0),
                             "NCC comparator component",
                             COMPONENT_CATEGORY_PROCESSOR)

  /* Element Library Params */
  static std::vector<SST::ElementInfoParam> getComponentParams() {
    auto params = DRRAResource::getBaseParams();
    params.push_back({"bulk_bitwidth", "Width of the bulk input bus", "64"});
    params.push_back(
        {"K_LOG2", "log2(K) — K is a power-of-two scale factor", "5"});
    params.push_back({"LINEAR_BITWIDTH",
                      "Width of the linear sum registers (S_A, S_B)", "28"});
    params.push_back({"QUAD_BITWIDTH",
                      "Width of the quadratic sum registers (S_A2, S_AB)",
                      "42"});
    params.push_back(
        {"LINEAR_RSHIFT",
         "Right-shift applied to linear terms before multiplication", "5"});
    params.push_back({"PRODUCT_BITWIDTH",
                      "Saturation width for the cross-product comparison",
                      "144"});
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
  Ncc_cmp(SST::ComponentId_t id, SST::Params &params);

  /* Destructor */
  ~Ncc_cmp() {};

  bool clockTick(SST::Cycle_t currentCycle) override;

  void handleEventWithSlotID(SST::Event *event, uint32_t slot_id);

  // Instruction handlers (called from auto-generated pkg dispatcher)
  using DRRAResource::format;
  void handleNCC(const NCC_CMP_PKG::NCCInstruction &instr);
  void handleEVT(const NCC_CMP_PKG::EVTInstruction &instr);
  void handleREP(const NCC_CMP_PKG::REPInstruction &instr);
  void handleREPX(const NCC_CMP_PKG::REPXInstruction &instr);
  void handleTRANS(const NCC_CMP_PKG::TRANSInstruction &instr);

  void handleActivation(uint32_t slot_id, uint32_t ports) override;

  using DRRAResource::out;

  // Per-cycle dispatch helpers (called from clockTick).
  void doNcc(uint32_t mode);
  void doLoad(uint32_t mode);
  void doCompare();
  void doReset();

  uint64_t getLocalCnt() const { return local_cnt; }
  uint64_t getMaxCnt() const { return max_cnt; }
  void incrementLocalCnt() { local_cnt++; }
  void setMaxCnt(uint64_t v) { max_cnt = v; }
  void clearAllState();
  void emitMaxCount();

  size_t getLinearBitwidth() const { return linear_bitwidth; }
  size_t getQuadBitwidth() const { return quad_bitwidth; }
  size_t getLinearRshift() const { return linear_rshift; }
  size_t getQuadRshift() const { return 2 * linear_rshift; }
  size_t getKLog2() const { return k_log2; }
  size_t getProductBitwidth() const { return product_bitwidth; }

  size_t bulk_bitwidth;

private:
  bool currentBeatsBest() const;
  void promoteBest();

  // Candidate / best running max. Values are sliced and shifted at load time,
  // matching ncc_cmp.sv's [WIDTH-1:RSHIFT] bulk input selection.
  int64_t s_a_cur = 0, s_a2_cur = 0, s_ab_cur = 0;
  int64_t s_a_best = 0, s_a2_best = 0, s_ab_best = 0;
  int64_t s_b = 0;
  bool best_valid = false;

  uint64_t local_cnt = 0;
  uint64_t max_cnt = 0;

  size_t linear_bitwidth;
  size_t quad_bitwidth;
  size_t linear_rshift;
  size_t k_log2;
  size_t product_bitwidth;

  std::array<uint32_t,
             1u << NCC_CMP_PKG::NCC_CMP_INSTR_NCC_CONFIG_BITWIDTH>
      ncc_modes;
  std::unordered_map<uint32_t, uint32_t> portsToActivate;
};

#endif // _NCC_CMP_H
