#include "io_mux.h"
#include "dataEvent.h"
#include "ioEvents.h"
#include "io_mux_pkg.h"
#include "timingOperators.h"

using namespace SST;

Io_mux::Io_mux(SST::ComponentId_t id, SST::Params &params)
    : DRRAResource(id, params) {
  instructionHandlers = IO_MUX_PKG::createInstructionHandlers(this);

  num_input_patterns = params.find<uint32_t>("NUM_INPUT_PATTERNS", 2);
  num_output_patterns = params.find<uint32_t>("NUM_OUTPUT_PATTERNS", 1);
  num_pattern_agus = num_input_patterns + num_output_patterns;
  num_selector_agus = (hasInputSelector() ? 1 : 0) + (hasOutputSelector() ? 1 : 0);
  required_physical_agus = num_pattern_agus + num_selector_agus;

  if (num_input_patterns == 0 || num_output_patterns == 0) {
    out.fatal(CALL_INFO, -1,
              "io_mux requires at least one input and one output pattern AGU\n");
  }
  // io_mux's physical AGU count is data-dependent: one AGU per input/output
  // pattern plus the selector AGUs. It addresses AGUs at contiguous indices
  // [0, required_physical_agus). If that exceeds the base default port space,
  // grow num_agus so checkAGULifetime covers them. (Replaces the old guard that
  // fataled when required exceeded the FSM_PER_SLOT-derived count.)
  if (required_physical_agus > num_agus) {
    setNumAgus(required_physical_agus);
  }
}

bool Io_mux::clockTick(SST::Cycle_t currentCycle) {
  bool result = DRRAResource::clockTick(currentCycle);

  // Latch any delivered value into the held bulk input wire before bulkInput().
  receiveDataInputs();

  if (!portsToActivate.empty() && currentCycle % 10 == 0) {
    for (const auto &port : portsToActivate) {
      if (port.second & (1u << INPUT_PORT)) {
        activateMuxPort(INPUT_PORT);
      }
      if (port.second & (1u << OUTPUT_PORT)) {
        activateMuxPort(OUTPUT_PORT);
      }
    }
    portsToActivate.clear();
  }

  if (currentCycle % 10 == 2) {
    if (isLogicalPortActive(INPUT_PORT) &&
        addressForLogicalPort(INPUT_PORT) != -1) {
      bulkOutput();
    }
  }

  if (currentCycle % 10 == 7) {
    if (isLogicalPortActive(OUTPUT_PORT) &&
        addressForLogicalPort(OUTPUT_PORT) != -1) {
      bulkInput();
    }
  }

  return result;
}

void Io_mux::handleActivation(uint32_t slot_id, uint32_t ports) {
  portsToActivate[slot_id] = ports;
}

void Io_mux::handleEVT(const IO_MUX_PKG::EVTInstruction &instr) {
  uint32_t physical_agu = 0;
  bool is_selector = false;

  out.output(
      "evt (slot=%d, port=%d, agu_idx=%d, init_addr_sd=%d, init_addr=%d)\n",
      instr.slot, instr.port, instr.agu_idx, instr.init_addr_sd,
      instr.init_addr);

  if (!decodeEVTTarget(instr.port, instr.agu_idx, physical_agu, is_selector)) {
    out.fatal(CALL_INFO, -1,
              "Invalid io_mux EVT target: port=%u agu_idx=%u\n", instr.port,
              instr.agu_idx);
  }

  current_target_valid = true;
  current_target_agu = physical_agu;

  addEVTEvent(instr.port, physical_agu, is_selector, instr.init_addr);
  current_event_number++;
}

void Io_mux::handleREP(const IO_MUX_PKG::REPInstruction &instr) {
  out.output("rep (slot=%d, port=%d, iter=%d, step=%d, delay=%d)\n",
             instr.slot, instr.port, instr.iter, instr.step, instr.delay);

  if (!current_target_valid) {
    out.fatal(CALL_INFO, -1, "REP issued before a valid EVT target\n");
  }

  try {
    agus[current_target_agu].addRepetition(instr.iter, instr.delay, instr.step);
    out.output("Added repetition to physical AGU %u (iter=%d, step=%d)\n",
               current_target_agu, instr.iter, instr.step);
  } catch (const std::exception &e) {
    out.fatal(CALL_INFO, -1, "Failed to add repetition: %s\n", e.what());
  }
}

void Io_mux::handleREPX(const IO_MUX_PKG::REPXInstruction &instr) {
  out.output("repx (slot=%d, port=%d, iter=%d, step=%d, delay=%d)\n",
             instr.slot, instr.port, instr.iter, instr.step, instr.delay);

  if (!current_target_valid) {
    out.fatal(CALL_INFO, -1, "REPX issued before a valid EVT target\n");
  }

  auto repetition_op = agus[current_target_agu].getLastRepetitionOperator();
  uint32_t iter = instr.iter << IO_MUX_PKG::IO_MUX_INSTR_REPX_ITER_BITWIDTH |
                  repetition_op.getIterations();
  uint32_t step = instr.step << IO_MUX_PKG::IO_MUX_INSTR_REPX_STEP_BITWIDTH |
                  repetition_op.getStep();
  uint32_t delay = instr.delay << IO_MUX_PKG::IO_MUX_INSTR_REPX_DELAY_BITWIDTH |
                   repetition_op.getDelay();

  try {
    agus[current_target_agu].adjustRepetition(iter, delay, step);
  } catch (const std::exception &e) {
    out.fatal(CALL_INFO, -1, "REPX failed: %s\n", e.what());
  }
}

void Io_mux::handleTRANS(const IO_MUX_PKG::TRANSInstruction &instr) {
  out.output("trans (slot=%d, port=%d, delay=%d)\n", instr.slot, instr.port,
             instr.delay);

  if (!current_target_valid) {
    out.fatal(CALL_INFO, -1, "TRANS issued before a valid EVT target\n");
  }

  try {
    agus[current_target_agu].addTransition(instr.delay);
    current_event_number++;
  } catch (const std::exception &e) {
    out.fatal(CALL_INFO, -1, "Failed to add transition: %s\n", e.what());
  }
}

bool Io_mux::hasSelector(uint32_t port) const {
  return port == INPUT_PORT ? hasInputSelector() : hasOutputSelector();
}

uint32_t Io_mux::patternBase(uint32_t port) const {
  return port == INPUT_PORT ? inputPatternBase() : outputPatternBase();
}

uint32_t Io_mux::patternCount(uint32_t port) const {
  return port == INPUT_PORT ? num_input_patterns : num_output_patterns;
}

uint32_t Io_mux::selectorIndex(uint32_t port) const {
  return port == INPUT_PORT ? inputSelectorIndex() : outputSelectorIndex();
}

uint32_t Io_mux::activeRepresentativeAgu(uint32_t port) const {
  return hasSelector(port) ? selectorIndex(port) : patternBase(port);
}

bool Io_mux::decodeEVTTarget(uint32_t port, uint32_t agu_idx,
                             uint32_t &physical_agu,
                             bool &is_selector) const {
  if (port != INPUT_PORT && port != OUTPUT_PORT) {
    return false;
  }

  if (agu_idx < patternCount(port)) {
    physical_agu = patternBase(port) + agu_idx;
    is_selector = false;
    return true;
  }

  if (hasSelector(port) && agu_idx == patternCount(port)) {
    physical_agu = selectorIndex(port);
    is_selector = true;
    return true;
  }

  return false;
}

void Io_mux::addEVTEvent(uint32_t port, uint32_t physical_agu, bool is_selector,
                         uint64_t init_addr) {
  std::string event_name;
  const bool drives_io = is_selector || !hasSelector(port);

  if (port == INPUT_PORT && drives_io) {
    event_name = "io_mux_evt_read_from_input_" + std::to_string(current_event_number);
    agus[physical_agu].addEvent(
        event_name,
        [this] {
          readFromIO();
        },
        1, init_addr);
  } else if (port == OUTPUT_PORT && drives_io) {
    event_name = "io_mux_evt_write_to_output_" + std::to_string(current_event_number);
    agus[physical_agu].addEvent(
        event_name,
        [this] {
          writeToIO();
        },
        8, init_addr);
  } else {
    event_name = "io_mux_pattern_addr_" + std::to_string(current_event_number);
    agus[physical_agu].addEvent(event_name, [] {}, 5, init_addr);
  }
}

void Io_mux::activateMuxPort(uint32_t port) {
  for (uint32_t i = 0; i < patternCount(port); i++) {
    activatePort(patternBase(port) + i);
  }
  if (hasSelector(port)) {
    activatePort(selectorIndex(port));
  }
}

uint32_t Io_mux::selectorIndexWidth(uint32_t count) const {
  uint32_t width = 1;
  uint32_t encodings = 2;

  while (encodings < count && width < 31) {
    width++;
    encodings <<= 1;
  }

  return width;
}

bool Io_mux::selectedPatternIndex(uint32_t port, uint32_t cycle,
                                  uint32_t &pattern_idx) {
  if (!hasSelector(port)) {
    pattern_idx = 0;
    return true;
  }

  int64_t selector_value = agus[selectorIndex(port)].getAddressForCycle(cycle);
  if (selector_value < 0) {
    // The selector AGU produced no address for this cycle: it is idle or has
    // retired ahead of the pattern AGUs. The io_mux datapath gates io_en on
    // selector_valid, so an absent selector value means the port drives no
    // output this cycle — it is idle, not an error.
    return false;
  }

  uint32_t width = selectorIndexWidth(patternCount(port));
  uint32_t mask = (1u << width) - 1u;
  uint32_t candidate = static_cast<uint32_t>(selector_value) & mask;

  if (candidate >= patternCount(port)) {
    return false;
  }

  pattern_idx = candidate;
  return true;
}

int64_t Io_mux::addressForLogicalPort(uint32_t port) {
  uint32_t cycle = getPortActiveCycle(activeRepresentativeAgu(port));
  uint32_t pattern_idx = 0;
  if (!selectedPatternIndex(port, cycle, pattern_idx)) {
    return -1;
  }
  uint32_t physical_pattern = patternBase(port) + pattern_idx;
  int64_t address = agus[physical_pattern].getAddressForCycle(cycle);

  if (address < 0) {
    return -1;
  }
  return address;
}

bool Io_mux::isLogicalPortActive(uint32_t port) {
  return isPortActive(activeRepresentativeAgu(port));
}

void Io_mux::readFromIO() {
  read_from_io_address_buffer = addressForLogicalPort(INPUT_PORT);
  if (read_from_io_address_buffer < 0) {
    // No pattern is selected this cycle (selector idle or an unused selector
    // encoding): stay idle and issue no IO read.
    return;
  }

  IOReadRequest *readReq = new IOReadRequest();
  readReq->address = read_from_io_address_buffer;
  readReq->size = io_data_width / 8;
  readReq->column_id = cell_coordinates[1];

  out.output("Sending read request to IO (addr=%d, size=%dbits)\n",
             read_from_io_address_buffer, io_data_width);
  logTraceEvent("io_mux_read_from_input", slot_id, true, 'X',
                {{"address", (int)read_from_io_address_buffer},
                 {"size", (int)(io_data_width / 8)}});

  io_input_link->send(readReq);
}

void Io_mux::writeToIO() {
  write_to_io_address_buffer = addressForLogicalPort(OUTPUT_PORT);
  if (write_to_io_address_buffer < 0) {
    // No pattern is selected this cycle (selector idle or an unused selector
    // encoding): stay idle and issue no IO write.
    return;
  }

  IOWriteRequest *writeReq = new IOWriteRequest();
  writeReq->address = write_to_io_address_buffer;
  writeReq->data = io_output_data_buffer;
  io_output_link->send(writeReq);

  out.output("Sending write request to IO (addr=%d, size=%dbits, data=%s)\n",
             writeReq->address, writeReq->data.size() * 8,
             formatRawDataToWords(writeReq->data).c_str());
  logTraceEvent("io_mux_write_to_output", slot_id, true, 'X',
                {{"address", (int)write_to_io_address_buffer},
                 {"size", (int)(io_output_data_buffer.size())},
                 {"data", formatRawDataToWords(io_output_data_buffer)}});
}

void Io_mux::bulkInput() {
  if (addressForLogicalPort(OUTPUT_PORT) == -1) {
    out.fatal(CALL_INFO, -1,
              "Output io_mux selected no address for cycle %u\n",
              getPortActiveCycle(activeRepresentativeAgu(OUTPUT_PORT)));
  }

  // Sample the held bulk input wire (latched by receiveDataInputs()).
  io_output_data_buffer = readInput(0, PortChannel::BULK).data;

  out.output("Received bulk data (data=%s)\n",
             formatRawDataToWords(io_output_data_buffer).c_str());

  logTraceEvent("io_mux_bulk_input", slot_id, true, 'X',
                {{"data", formatRawDataToWords(io_output_data_buffer)}});
}

void Io_mux::bulkOutput() {
  if (addressForLogicalPort(INPUT_PORT) == -1) {
    out.fatal(CALL_INFO, -1,
              "Input io_mux selected no address for cycle %u\n",
              getPortActiveCycle(activeRepresentativeAgu(INPUT_PORT)));
  }

  IOReadResponse *readResp =
      dynamic_cast<IOReadResponse *>(io_input_link->recv());
  if (readResp) {
    out.output("Received read response from IO (addr=%d, size=%dbits, "
               "data=%s)\n",
               readResp->address, readResp->data.size() * 8,
               formatRawDataToWords(readResp->data).c_str());
    io_input_data_buffer = readResp->data;
    if (io_input_data_buffer.empty()) {
      out.fatal(CALL_INFO, -1, "No data received from IO\n");
    }
  } else {
    out.fatal(CALL_INFO, -1, "No response received from IO\n");
  }

  // Combinational passthrough of the IO read data onto the bulk wire
  // (io_mux.sv.j2: bulk_data_out_0 = io_en_in ? io_data_in : '0).
  driveOutput(0, PortChannel::BULK, io_input_data_buffer, io_data_width,
              /*registered=*/false);

  logTraceEvent("io_mux_bulk_output", slot_id, true, 'X',
                {{"data", formatRawDataToWords(io_input_data_buffer)}});
}
