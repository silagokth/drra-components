#pragma once

#include "agu_array.h"
#include "drra_component.h"
#include "timingModel.h"

#include <algorithm>
#include <deque>
#include <functional>
#include <sst/core/link.h>
#include <sst/core/params.h>
#include <sst/core/timeConverter.h>

using namespace SST;

class DRRAResource : public DRRAComponent {
public:
  DRRAResource(ComponentId_t id, Params &params);

  virtual ~DRRAResource() {}

  // The subcycle contract every resource runs on. One DRRA cycle is ten SST
  // subcycles; the base owns the first three and the resource's datapath uses
  // the rest.
  //
  //   sub 0  activation strobes latched since the last cycle are applied
  //   sub 1  an instruction sitting at the instruction port is decoded, so
  //          EVT / REP / TRANS reach the AGU array and CONF reaches the
  //          resource
  //   sub 2  the AGU array advances one cycle and drives its output ports
  //   sub 3+ addresses are stable: logic() may poll any AGU and get this
  //          cycle's value
  //
  // Both the ACT and the instruction arrive on the controller link, which is
  // wired at 0ns. SST runs clock handlers (CLOCKPRIORITY) before link
  // deliveries (EVENTPRIORITY) at the same simulated time, so an event sent by
  // the sequencer at subcycle 0 of a cycle lands after this component's own
  // subcycle-0 tick. Buffering both and draining them at a fixed subcycle
  // keeps that out of the resource's business.
  static constexpr uint32_t SUB_ACTIVATE = 0;
  static constexpr uint32_t SUB_INSTRUCTION = 1;
  static constexpr uint32_t SUB_AGU_UPDATE = 2;
  static constexpr uint32_t SUB_FIRST_ACTION = 3;

  virtual bool clockTick(Cycle_t currentCycle) override;

  virtual void decodeInstr(uint32_t instr);

  virtual void handleActivation(uint32_t slot_id, uint32_t ports);

  void handleEventBase(Event *event);

  // EVT, REP and TRANS are pure AGU programming: every resource does the same
  // thing with them, so the base owns the behaviour and the generated
  // instruction dispatch calls these directly. They take plain fields rather
  // than a resource's generated EVTInstruction / REPInstruction /
  // TRANSInstruction type, which is what lets one implementation serve every
  // resource. A resource that needs something different overrides -- using
  // this signature, or it will hide these rather than replace them.
  //
  // A resource whose ISA declares no option or init_addr segment (dpu, swb)
  // gets 0 for those.
  virtual void handleEVT(uint32_t slot, uint32_t port, uint32_t option,
                         uint64_t init_addr);

  // A base REP (ext = 0) carries the low half of iter / step / delay and a
  // REPX (ext = 1) folds the high half into the repetition it follows, so the
  // shift widths are the resource's own ISA segment widths, passed in by the
  // generated dispatch.
  virtual void handleREP(uint32_t slot, uint32_t port, bool ext, uint32_t iter,
                         uint32_t step, uint32_t delay, uint32_t iter_bits,
                         uint32_t step_bits, uint32_t delay_bits);

  virtual void handleTRANS(uint32_t slot, uint32_t port, uint32_t delay);

protected:
  static inline std::vector<SST::ElementInfoParam> getBaseParams() {
    std::vector<SST::ElementInfoParam> params = DRRAComponent::getBaseParams();
    params.push_back({"slot_id",
                      "Slot ID for the resource. If the resource occupies "
                      "multiple slots, slot ID of the first slot occupied.",
                      "-1"});
    params.push_back(
        {"resource_size",
         "Number of slots occupied by the resource (default to 1 slot)", "1"});
    params.push_back({"num_agus", "Number of AGUs in the resource", "1"});
    params.push_back(
        {"has_io_input_connection", "Has IO input connection", "0"});
    params.push_back(
        {"has_io_output_connection", "Has IO output connection", "0"});
    return params;
  }

  static inline std::vector<SST::ElementInfoPort> getBasePorts() {
    std::vector<SST::ElementInfoPort> ports;
    ports.push_back(
        {"controller_port%(portnum)d", "Link to the controller port", {""}});
    ports.push_back({"data_port%(portnum)d", "Link to the data port", {""}});
    ports.push_back({"io_input_port", "Link to the IO input port", {""}});
    ports.push_back({"io_output_port", "Link to the IO output port", {""}});
    return ports;
  }

  static inline std::vector<SST::ElementInfoStatistic> getBaseStatistics() {
    std::vector<SST::ElementInfoStatistic> stats =
        DRRAComponent::getBaseStatistics();
    return stats;
  }

  // The resource's datapath. Called every subcycle, after that subcycle's
  // phase work. The default runs the registered port actions, which is all
  // most resources need; a resource with behaviour that is not driven by an
  // AGU address (dpu's FSM, swb's option tracking) overrides this and calls
  // DRRAResource::logic() first if it also wants the table.
  //
  // AGU output ports are stable from SUB_AGU_UPDATE onwards.
  virtual void logic(uint32_t subcycle);

  // One thing a resource does when an AGU produces an address. Registered
  // once in the constructor; the base fires it at `subcycle` on every cycle
  // the port's AGU has a valid address, and hands it that address.
  struct PortAction {
    uint32_t port;
    uint32_t subcycle;
    std::string name;
    std::function<void(int64_t)> action;
    uint64_t max_address; // 0 = no bound
  };

  void registerPortAction(uint32_t port, uint32_t subcycle,
                          const std::string &name,
                          std::function<void(int64_t)> action,
                          uint64_t max_address = 0);

  // Reject an EVT on a port this resource does not implement. Used by the
  // iosram variants, which share one ISA but only one IO direction each.
  void forbidPort(uint32_t port, const std::string &reason);

  bool isPortActive(uint32_t port) { return agu_array.active(port); }
  bool isPortAddressValid(uint32_t port) { return agu_array.addrValid(port); }
  int64_t getPortAddress(uint32_t port) { return agu_array.addr(port); }

  void activatePort(uint32_t port) { agu_array.activate(port); }

  void activatePortsForSlot(uint32_t slot_id, uint32_t ports);

  uint32_t getRelativePortNum(uint32_t slot_id, uint32_t port_id) {
    uint8_t slot_pos = std::distance(
        slot_ids.begin(), std::find(slot_ids.begin(), slot_ids.end(), slot_id));
    return slot_pos * PORTS_PER_SLOT + port_id;
  }

  // Idle when nothing is running and nothing is waiting to be applied.
  // (DPU opts out.)
  bool isIdle() override {
    return !agu_array.anyActive() && portsToActivate.empty() &&
           instruction_queue.empty();
  }

  uint64_t vectorToUint64(std::vector<uint8_t> data);
  int64_t vectorToInt64(std::vector<uint8_t> data);
  std::vector<uint8_t> uint64ToVector(uint64_t data, bool saturate = true);
  std::vector<uint8_t> int64ToVector(int64_t data, bool saturate = true);

  // Links
  std::vector<Link *> controller_links;
  std::vector<Link *> data_links;
  Link *io_input_link = nullptr;
  Link *io_output_link = nullptr;

  // Activations arrive mid-cycle and are applied at SUB_ACTIVATE.
  std::unordered_map<uint32_t, uint32_t> portsToActivate;

  // Instructions arrive mid-cycle and are decoded at SUB_INSTRUCTION.
  std::deque<uint32_t> instruction_queue;

  // IO settings
  uint32_t io_data_width; // in bits
  bool has_io_input_connection, has_io_output_connection;

  // Slot settings
  int32_t slot_id;
  std::vector<int32_t> slot_ids;
  uint8_t resource_size = 1; // Default to 1 slot
  uint32_t num_fsms;

  // Data buffers
  std::map<uint32_t, std::vector<uint8_t>> data_buffers;

  // The shared AGU subsystem (lib/common/sst agu_array), one AGU per port.
  uint8_t num_agus;
  AGUArray agu_array;

private:
  void applyPendingActivations();
  void drainInstructionPort();

  std::vector<PortAction> port_actions;
  std::map<uint32_t, std::string> forbidden_ports;
};
