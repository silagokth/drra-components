#ifndef _DPU_H
#define _DPU_H

#include "drra.h"
#include "dataEvent.h"
#include <climits>
#include <sst/core/component.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/output.h>
#include <sst/core/sst_types.h>
#include <sst/core/timeConverter.h>

class DPUQ88 : public DRRAResource {
public:
  /* Element Library Info */
  SST_ELI_REGISTER_COMPONENT(DPUQ88, "drra", "dpu_Q88",
                             SST_ELI_ELEMENT_VERSION(1, 0, 0), "DPU Q8.8 fixed component",
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

  DPUQ88(SST::ComponentId_t id, SST::Params &params);
  ~DPUQ88() {};

  // SST lifecycle methods
  virtual void init(unsigned int phase) override;
  virtual void setup() override;
  virtual void complete(unsigned int phase) override;
  virtual void finish() override;

  bool clockTick(SST::Cycle_t currentCycle) override;
  void handleEventWithSlotID(SST::Event *event, uint32_t slot_id);

private:
  // buffers
  // std::map<uint32_t, std::vector<uint8_t>> data_buffers;
  int32_t accumulate_register;
  //std::vector<uint8_t> accumulate_register;
  // Supported opcodes
  void decodeInstr(uint32_t instr) override;
  enum OpCode { REP, REPX, FSM, DPU_OP };
  void handleRep(uint32_t instr);
  void handleRepx(uint32_t instr);
  void handleFSM(uint32_t instr);
  void handleDPU(uint32_t instr);
  void handleOperation(std::string name,
                       std::function<int16_t(int16_t, int16_t)> operation);
  // DPU modes
  //ADD mode SAT32_16, to saturate Q16.16 result to Q8.8 after MAC accumulation
  enum DPU_MODE {
    IDLE,
    ADD,
    SUM_ACC,
    ADD_CONST,
    SUBT,
    SUBT_ABS,
    MODE_06,
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

  int16_t vectorToint16(std::vector<uint8_t> data) {
    int16_t result = 0;
    for (size_t i = 0; i < 2; i++) {
      result |= data[i] << (i * 8);
    }
    return result;
  }

  std::vector<uint8_t> int16ToVector(int16_t data) {
      std::vector<uint8_t> result(2);
      uint16_t u = static_cast<uint16_t>(data); 
      result[0] = u & 0xFF;
      result[1] = (u >> 8) & 0xFF;
      return result;
  }

  int32_t vectorToint32(std::vector<uint8_t> data) {
    int32_t result = 0;
    for (size_t i = 0; i < 4; i++) {
      result |= data[i] << (i * 8);
    }
    return result;
  }

  std::vector<uint8_t> int32ToVector(int32_t data) {
      std::vector<uint8_t> result(4);
      uint32_t u = static_cast<uint32_t>(data); 
      for (size_t i = 0; i < 4; i++) {
        result[i] |= ( u >> (i*8) ) & 0xFF;
      }
      return result;
  }

  const int16_t add_q88_sat(int16_t a, int16_t b){
    int32_t s = (int32_t)a + (int32_t)b;
    if (s > INT16_MAX) s = INT16_MAX;
    if (s < INT16_MIN) s = INT16_MIN;
    return (int16_t)s;
  }

  const int16_t mul_q88_sat(int16_t a, int16_t b){
    int32_t p = (int32_t)a * (int32_t)b; //Q16.16
    //rounding, p's going to >>8, 
    //so +/- a 0x80 to Q16.16 to pre-decide the last bit after shifting to Q8.8 
    // if(p>=0){ 
    //   p = p + 128;
    // } else {
    //   p = p - 128;
    // }
    p = p >> 8;
    if (p > INT16_MAX) p = INT16_MAX;
    if (p < INT16_MIN) p = INT16_MIN;
    return (int16_t)p;
  }

  //In order to keep the precision when doing MAC accumulation.
  const int16_t q88_mac(int16_t a, int16_t b){
    int32_t p = (int32_t)a * (int32_t)b; //Q16.16
    p = p >> 8;
    p = p + accumulate_register;
    //p = p + vectorToint32(accumulate_register);
    //accumulate_register = int32ToVector(p);
    accumulate_register = p;
    if (p > INT16_MAX) p = (int16_t)INT16_MAX;
    if (p < INT16_MIN) p = (int16_t)INT16_MIN;
    return (int16_t)p;
  }

  // Map of DPU modes to handlers
  std::map<DPU_MODE, std::function<void()>> dpuHandlers = {
      {IDLE, [this] { out.output("IDLE\n"); }},
      {ADD,
       [this] {
         handleOperation(
             "ADD", [this](int16_t a, int16_t b) { return add_q88_sat(a, b); });
       }},
      {ADD_CONST,
       [this] {
         handleOperation("ADD_CONST", [this](int16_t a, int16_t b) {
           return add_q88_sat(a, b);
         });
       }},
      {SUBT,
       [this] {
         handleOperation(
             "SUBT", [this](int16_t a, int16_t b) { return add_q88_sat(a, -b); });
       }},
      {SUBT_ABS,
       [this] {
         handleOperation("SUBT_ABS", [this](int16_t a, int16_t b) {
           return add_q88_sat(a, -b);
         });
       }},
      {MULT,
       [this] {
         handleOperation(
             "MULT", [this](int16_t a, int16_t b) { return mul_q88_sat(a, b); });
       }},
      {MULT_CONST,
       [this] {
         handleOperation("MULT_CONST", [this](int16_t a, int16_t b) {
           return mul_q88_sat(a, b);
         });
       }},
      {LD_IR,
       [this] {
         handleOperation("LD_IR", [](int16_t a, int16_t b) { return b; });
       }},
      {MAC,
       [this] {
         handleOperation("MAC", [this](int16_t a, int16_t b) {
           return q88_mac(a,b);
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
