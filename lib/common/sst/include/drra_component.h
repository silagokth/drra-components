
#include "drra_output.h"

#include <fstream>

#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/params.h>
#include <variant>

#include "instruction.h"
#include "traceEvent.h"

using namespace SST;

#define PORTS_PER_SLOT 4

class DRRAComponent : public Component {
public:
  DRRAComponent(ComponentId_t id, Params &params) : Component(id) {
    // Configure init
    out.init("", 16, 0, Output::STDOUT);

    // Get parameters
    clock = params.find<std::string>("clock", "100MHz");
    printFrequency = params.find<Cycle_t>("printFrequency", 1);
    word_bitwidth = params.find<size_t>("word_bitwidth", 16);

    // Set statistics
    // number_of_instructions = registerStatistic

    // Output JSON
    trace_name = params.find<std::string>("trace_name", "trace.json");
    std::fstream trace_file_exists(trace_name);
    if (trace_file_exists.good()) {
      trace_file.open(trace_name, std::ios::out | std::ios::app);
      if (!trace_file.is_open()) {
        out.fatal(CALL_INFO, -1, "Failed to open trace file %s\n",
                  trace_name.c_str());
      }
    } else {
      trace_file.open(trace_name, std::ios::out | std::ios::trunc);
      if (!trace_file.is_open()) {
        out.fatal(CALL_INFO, -1, "Failed to open trace file %s\n",
                  trace_name.c_str());
      }
      trace_file << "{ \"traceEvents\": [" << std::endl;
      trace_file << "{\"name\": \"process_name\", \"ph\": \"M\", \"pid\": "
                    "0, \"args\": {\"name\": \"drra\"}},\n";
    }
    trace_file.close();

    tc = registerClock(
        clock,
        new Clock::Handler2<DRRAComponent, &DRRAComponent::clockTickBase>(
            this));

    // Parent cell variables
    std::vector<int> paramsCellCoordinates;
    params.find_array<int>("cell_coordinates", paramsCellCoordinates);
    if (paramsCellCoordinates.size() != 2) {
      out.output("Size of cell coordinates: %lu\n",
                 paramsCellCoordinates.size());
      out.fatal(CALL_INFO, -1, "Invalid cell coordinates\n");
    } else {
      cell_coordinates[0] = paramsCellCoordinates[0];
      cell_coordinates[1] = paramsCellCoordinates[1];
    }
    num_slots = params.find<uint32_t>("num_slots", 16);

    // Instruction format
    format.total_bitwidth = params.find<uint64_t>("instr_bitwidth", 32);
    format.type_bitwidth = params.find<uint64_t>("instr_type_bitwidth", 1);
    format.opcode_bitwidth = params.find<uint64_t>("instr_opcode_width", 3);
    format.slot_bitwidth = params.find<uint64_t>("instr_slot_width", 4);
    instrBitwidth = format.total_bitwidth;
    instrTypeBitwidth = format.type_bitwidth;
    instrOpcodeWidth = format.opcode_bitwidth;
    instrSlotWidth = format.slot_bitwidth;
  }

  virtual ~DRRAComponent() {}

  // SST lifecycle methods
  virtual void init(unsigned int phase) override {
    out.verbose(CALL_INFO, 1, 0, "Initialized\n");
  };
  virtual void setup() override {};
  virtual void complete(unsigned int phase) override {};
  virtual void finish() override {
    out.verbose(CALL_INFO, 1, 0, "Finishing\n");
  }

  // SST clock handler
  bool clockTickBase(Cycle_t currentCycle) {
    _currentSSTCycle = currentCycle;
    if (currentCycle % 10 == 0) {
      out.output("--- CYCLE %" PRIu64 " ---\n", currentCycle / 10);
    }
    bool result = clockTick(currentCycle);
    return result;
  }

  void logTraceEvent(
      std::string name, int slot_id, bool isResource = true,
      const std::unordered_map<std::string, std::variant<int, std::string>>
          &args = {}) {
    trace_file.open(trace_name, std::ios::app);
    TraceEvent trace_event(name, _currentSSTCycle);
    trace_event.setThreadId(isResource ? 1 : 2, cell_coordinates[0],
                            cell_coordinates[1], slot_id);
    trace_event.setProcessId(0);
    for (const auto &arg : args) {
      if (std::holds_alternative<int>(arg.second)) {
        trace_event.addArg(arg.first, std::get<int>(arg.second));
      } else if (std::holds_alternative<std::string>(arg.second)) {
        trace_event.addArg(arg.first, std::get<std::string>(arg.second));
      }
    }
    trace_file << trace_event.toJsonLine();
    trace_file.close();
  }

  // SST event handler
  // virtual void handleEvent(Event *event) = 0;

protected:
  virtual bool clockTick(Cycle_t currentCycle) { return false; }
  // Document params
  static std::vector<SST::ElementInfoParam> getBaseParams() {
    std::vector<SST::ElementInfoParam> params;
    params.push_back({"clock", "Clock frequency", "100MHz"});
    params.push_back(
        {"printFrequency", "Frequency to print tick messages", "1000"});
    params.push_back({"io_data_width", "Width of the IO data", "256"});
    params.push_back({"word_bitwidth", "Width of the word", "16"});
    params.push_back({"instr_bitwidth", "Instruction bitwidth", "32"});
    params.push_back({"instr_type_bitwidth", "Instruction type", "1"});
    params.push_back({"instr_opcode_width", "Instruction opcode width", "3"});
    params.push_back({"instr_slot_width", "Instruction slot width", "4"});
    params.push_back({"cell_coordinates", "Cell coordinates", "[0,0]"});
    params.push_back({"num_slots", "Number of slots in the parent cell", "16"});
    return params;
  }

  static std::vector<SST::ElementInfoStatistic> getBaseStatistics() {
    std::vector<SST::ElementInfoStatistic> stats;
    // stats.push_back({"number_of_instructions", "Number of instructions
    // executed", "count", 1});
    return stats;
  }

  // Simulation global variables
  TimeConverter *tc;
  Clock::HandlerBase *clockHandler;
  DRRAOutput out;
  std::string clock;
  Cycle_t printFrequency;
  Cycle_t _currentSSTCycle = 0;

  std::string trace_name = "";
  std::ofstream trace_file;

  // Local buffers
  uint32_t instrBuffer;

  // Parent cell variables
  uint32_t num_slots;
  uint32_t cell_coordinates[2] = {0, 0};

  // Narrow level
  size_t word_bitwidth;

  // Instruction format (from isa.json file)
  Instruction::Format format;
  uint32_t instrBitwidth;
  uint32_t instrTypeBitwidth;
  uint32_t instrOpcodeWidth;
  uint32_t instrSlotWidth;

  // Instruction handlers
  std::unordered_map<uint32_t, std::function<void(uint32_t)>>
      instructionHandlers;

  // TODO: remove this once all components are updated
  uint32_t getInstrType(uint32_t instr) {
    return getInstrField(instr, instrTypeBitwidth,
                         instrBitwidth - instrTypeBitwidth);
  }

  uint32_t getInstrOpcode(uint32_t instr) {
    return getInstrField(instr, instrOpcodeWidth,
                         instrBitwidth - instrTypeBitwidth - instrOpcodeWidth);
  }

  uint32_t getInstrSlot(uint32_t instr) {
    return getInstrField(instr, instrSlotWidth,
                         instrBitwidth - instrTypeBitwidth - instrOpcodeWidth -
                             instrSlotWidth);
  }

  uint32_t isResourceInstruction(uint32_t instr) {
    return getInstrType(instr) == 1;
  }

  uint32_t isControlInstruction(uint32_t instr) {
    return getInstrType(instr) == 0;
  }

  uint32_t getInstrField(uint32_t instr, uint32_t fieldWidth,
                         uint32_t fieldOffset) {
    return (instr & ((1 << fieldWidth) - 1) << fieldOffset) >> fieldOffset;
  }

  std::string formatRawDataToWords(std::vector<uint8_t> raw_data) {
    std::string formatted_data = "[";
    size_t word_size = word_bitwidth / 8;
    uint64_t current_word = 0;
    for (size_t i = 0; i < raw_data.size(); i++) {
      current_word |= raw_data[i] << (i % word_size * 8);
      if ((i + 1) % word_size == 0) {
        formatted_data += std::to_string(current_word);
        current_word = 0;
        if (i + 1 < raw_data.size()) {
          formatted_data += ", ";
        }
      }
    }
    formatted_data += "]";
    return formatted_data;
  }
};
