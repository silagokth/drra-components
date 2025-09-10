#ifndef _DPU_H
#define _DPU_H

#include "drra.h"
#include <climits>

class DPU : public DRRAResource {
public:
  /* Element Library Info */
  SST_ELI_REGISTER_COMPONENT(DPU, "drra", "dpu",
                             SST_ELI_ELEMENT_VERSION(1, 0, 0), "DPU component",
                             COMPONENT_CATEGORY_PROCESSOR)

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

  DPU(SST::ComponentId_t id, SST::Params &params);
  ~DPU() {};

  bool clockTick(SST::Cycle_t currentCycle) override;
  void handleEventWithSlotID(SST::Event *event, uint32_t slot_id);

private:
  // buffers
  std::vector<uint8_t> accumulate_register;

  // Supported opcodes
  void decodeInstr(uint32_t instr) override;
  enum OpCode { REP, REPX, FSM, DPU_OP };
  void handleRep(uint32_t instr);
  void handleRepx(uint32_t instr);
  void handleFSM(uint32_t instr);
  void handleDPU(uint32_t instr);
  void handleOperation(std::string name,
                       std::function<int64_t(int64_t, int64_t)> operation);

  // DPU modes
  enum DPU_MODE {
    IDLE,
    ADD,
    SUM_ACC,
    ADD_CONST,
    SUBT,
    SUBT_ABS,
    MODE_6,
    MULT,
    MULT_ADD,
    MULT_CONST,
    MAC,
    LD_IR,
    AXPY,
    MAX_MIN_ACC,
    MAX_MIN_CONST,
    MODE_15,
    MAX_MIN,
    SHIFT_L,
    SHIFT_R,
    SIGM,
    TANHYP,
    EXPON,
    LK_RELU,
    RELU,
    DIV,
    ACC_SOFTMAX,
    DIV_SOFTMAX,
    LD_ACC,
    SCALE_DW,
    SCALE_UP,
    MAC_INTER,
    MODE_31
  };

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

  // Map of DPU modes to handlers
  std::map<DPU_MODE, std::function<void()>> dpuHandlers = {
      {IDLE, [this] { out.output("IDLE\n"); }},
      {ADD,
       [this] {
         handleOperation(
             "ADD", [this](int64_t a, int64_t b) { return add_sat(a, b); });
       }},
      {ADD_CONST,
       [this] {
         handleOperation("ADD_CONST", [this](int64_t a, int64_t b) {
           return add_sat(a, b);
         });
       }},
      {SUBT,
       [this] {
         handleOperation(
             "SUBT", [this](int64_t a, int64_t b) { return add_sat(a, -b); });
       }},
      {SUBT_ABS,
       [this] {
         handleOperation("SUBT_ABS", [this](int64_t a, int64_t b) {
           return add_sat(a, -b);
         });
       }},
      {MULT,
       [this] {
         handleOperation(
             "MULT", [this](int64_t a, int64_t b) { return mul_sat(a, b); });
       }},
      {MULT_CONST,
       [this] {
         handleOperation("MULT_CONST", [this](int64_t a, int64_t b) {
           return mul_sat(a, b);
         });
       }},
      {LD_IR,
       [this] {
         handleOperation("LD_IR", [](int64_t a, int64_t b) { return b; });
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

  std::function<void()> getDPUHandler(DPU_MODE mode) {
    if (dpuHandlers.find(mode) == dpuHandlers.end())
      out.fatal(CALL_INFO, -1, "DPU mode %d not implemented\n", mode);

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

#endif // _DPU_H
