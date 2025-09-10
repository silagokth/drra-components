#ifndef _FPU16_H
#define _FPU16_H

#include "drra.h"
#include <climits>
#include <sst/core/component.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/output.h>
#include <sst/core/sst_types.h>
#include <sst/core/timeConverter.h>

class FPU16 : public DRRAResource {
public:
  /* Element Library Info */
  SST_ELI_REGISTER_COMPONENT(FPU16, "drra", "fpu16",
                             SST_ELI_ELEMENT_VERSION(1, 0, 0),
                             "FPU16 component", COMPONENT_CATEGORY_PROCESSOR)

  /* Element Library Params */
  static std::vector<SST::ElementInfoParam> getComponentParams() {
    auto params = DRRAResource::getBaseParams();
    return params;
  }
  SST_ELI_DOCUMENT_PARAMS(getComponentParams())

  /* Element Library Ports */
  static std::vector<SST::ElementInfoPort> getComponentPorts() {
    auto ports = DRRAResource::getBasePorts();
    return ports;
  }
  SST_ELI_DOCUMENT_PORTS(getComponentPorts())

  SST_ELI_DOCUMENT_STATISTICS()
  SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS()

  FPU16(SST::ComponentId_t id, SST::Params &params);
  ~FPU16() {};

  // SST lifecycle methods
  virtual void init(unsigned int phase) override;
  virtual void setup() override;
  virtual void complete(unsigned int phase) override;
  virtual void finish() override;

  bool clockTick(SST::Cycle_t currentCycle) override;
  void handleEventWithSlotID(SST::Event *event, uint32_t slot_id);

private:
  // buffers
  std::vector<uint8_t> accumulate_register;

  // Supported opcodes
  void decodeInstr(uint32_t instr) override;
  enum OpCode { REP, REPX, FSM, FPU_OP };
  void handleRep(uint32_t instr);
  void handleRepx(uint32_t instr);
  void handleFSM(uint32_t instr);
  void handleFPU(uint32_t instr);
  void handleOperation(std::string name,
                       std::function<int64_t(int64_t, int64_t)> operation);

  // FPU16 modes
  enum FPU16_MODE { IDLE, ADD, MUL, MAC };

  const int64_t add_sat(int64_t a, int64_t b) {
    if (a > 0) {
      if (b > INT64_MAX - a) {
        return INT64_MAX;
      }
    } else if (b < INT64_MIN - a) {
      return INT64_MIN;
    }
    return a + b;
  }

  const int64_t mul_sat(int64_t a, int64_t b) {
    if (a == 0 || b == 0) {
      return 0;
    }

    if (a > 0) {
      if (b > 0) {
        if (a > INT64_MAX / b) {
          return INT64_MAX;
        }
      } else {
        if (b < INT64_MIN / a) {
          return INT64_MIN;
        }
      }
    } else {
      if (b > 0) {
        if (a < INT64_MIN / b) {
          return INT64_MIN;
        }
      } else {
        if (a != INT64_MIN && b < INT64_MAX / a) {
          return INT64_MAX;
        }
      }
    }

    return a * b;
  }

  // Map of FPU16 modes to handlers
  std::map<FPU16_MODE, std::function<void()>> dpuHandlers = {
      {IDLE, [this] { out.output("IDLE\n"); }},
      {ADD,
       [this] {
         handleOperation(
             "ADD", [this](int64_t a, int64_t b) { return add_sat(a, b); });
       }},
      {MUL,
       [this] {
         handleOperation(
             "MULT", [this](int64_t a, int64_t b) { return mul_sat(a, b); });
       }},
      {MAC,
       [this] {
         handleOperation("MAC", [this](int64_t a, int64_t b) {
           out.output("MAC\n");
           int64_t result =
               add_sat(vectorToInt64(accumulate_register), mul_sat(a, b));
           accumulate_register = int64ToVector(result);
           out.output("accumulate_register[0] = %ld\n", result);
           return result;
         });
       }},
  };

  std::function<void()> getFPU16Handler(FPU16_MODE mode) {
    if (dpuHandlers.find(mode) == dpuHandlers.end())
      out.fatal(CALL_INFO, -1, "FPU16 mode %d not implemented\n", mode);

    return dpuHandlers[mode];
  }

  // Events handlers list
  std::vector<std::function<void()>> eventsHandlers;
  uint32_t current_event_number = 0;

  // FSMs
  static const uint32_t num_fsms = 4;
  uint32_t current_fsm = 0;
  std::function<void()> fsmHandlers[num_fsms];
};

#endif // _FPU16_H
