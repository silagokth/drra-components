#include "rf.h"
#include "dataEvent.h"
#include "rf_pkg.h"
#include <string>

using namespace SST;

Rf::Rf(SST::ComponentId_t id, SST::Params &params) : DRRAResource(id, params) {
  // Register file parameters
  access_time = params.find<std::string>("access_time", "0ns");
  register_file_size = params.find<int>("RF_DEPTH", 64);
  for (int i = 0; i < register_file_size; i++) {
    for (int j = 0; j < word_bitwidth / 8; j++) {
      registers[i].push_back(0);
    }
  }
  instructionHandlers = RF_PKG::createInstructionHandlers(this);

  std::string registers_content;
  for (auto &reg : registers) {
    registers_content += formatRawDataToWords(reg.second) + " ";
  }
  logTraceEvent("registers", slot_id, true, 'B',
                {{"registers", registers_content}});
}

bool Rf::clockTick(SST::Cycle_t currentCycle) {
  if (portsToActivate.size() > 0 && currentCycle % 10 == 0) {
    for (const auto &port : portsToActivate) {
      activatePortsForSlot(port.first, port.second);
    }
    portsToActivate.clear();
  }

  // Latch values the SWB delivered into the held input wires before the write
  // events (priority 8) sample them.
  receiveDataInputs();

  bool result = DRRAResource::clockTick(currentCycle); // runs read/write events

  // Comb read outputs at the Drive phase (sub-2): the read events (priority 2,
  // run inside DRRAResource::clockTick above) drive the value on firing cycles;
  // on active cycles where the read AGU is NOT firing, drive 0 here -- matching
  // the RTL (word_data_out_0='0 when !word_r_en).
  if (currentCycle % 10 == 2) {
    const std::pair<uint32_t, PortChannel> read_ports[] = {
        {DataEvent::PortType::ReadNarrow, PortChannel::WORD},
        {DataEvent::PortType::ReadWide, PortChannel::BULK}};
    for (auto [port, ch] : read_ports) {
      if (isPortActive(port) &&
          agus[port].getAddressForCycle(getPortActiveCycle(port)) < 0)
        driveOutputIdle(0, ch, false);
    }
  }
  return result;
}

void Rf::handleActivation(uint32_t slot_id, uint32_t ports) {
  portsToActivate[slot_id] = ports;
}

void Rf::handleCONF(const RF_PKG::CONFInstruction &instr) {
  out.output("conf (slot=%d)\n", instr.slot);
}

void Rf::handleEVT(const RF_PKG::EVTInstruction &instr) {
  out.output(
      "evt (slot=%d, option=%d, port=%d, init_addr_sd=%d, init_addr=%d)\n",
      instr.slot, instr.option, instr.port, instr.init_addr_sd,
      instr.init_addr);

  auto evt = instr;

  current_option_config[instr.port] = evt.option;

  // Add the event handler
  std::string event_name;
  switch (evt.port) {
  case DataEvent::PortType::ReadNarrow:
    event_name = "evt_read_narrow_" + std::to_string(current_event_number);
    agus[evt.port].addEvent(
        event_name,
        [this, event_name] {
          updatePortAGUs(DataEvent::PortType::ReadNarrow);
          readNarrow();
        },
        2, evt.init_addr);
    break;
  case DataEvent::PortType::ReadWide:
    event_name = "evt_read_wide_" + std::to_string(current_event_number);
    agus[evt.port].addEvent(
        event_name,
        [this, event_name] {
          updatePortAGUs(DataEvent::PortType::ReadWide);
          readWide();
        },
        2, evt.init_addr);
    break;
  case DataEvent::PortType::WriteNarrow:
    event_name = "evt_write_narrow_" + std::to_string(current_event_number);
    agus[evt.port].addEvent(
        event_name,
        [this, event_name] {
          updatePortAGUs(DataEvent::PortType::WriteNarrow);
          writeNarrow();
        },
        7, evt.init_addr);
    break;
  case DataEvent::PortType::WriteWide:
    event_name = "evt_write_wide_" + std::to_string(current_event_number);
    agus[evt.port].addEvent(
        event_name,
        [this, event_name] {
          updatePortAGUs(DataEvent::PortType::WriteWide);
          writeWide();
        },
        7, evt.init_addr);
    break;

  default:
    out.fatal(CALL_INFO, -1, "Invalid EVT mode\n");
  }

  // Add event handler
  current_event_number++;
}

void Rf::handleREP(const RF_PKG::REPInstruction &instr) {
  out.output("rep (slot=%d, ext=%d, port=%d, iter=%d, step=%d, delay=%d)\n",
             instr.slot, instr.ext, instr.port, instr.iter, instr.step,
             instr.delay);

  uint32_t port_num = getRelativePortNum(instr.slot, instr.port);

  try {
    if (!instr.ext) {
      // base: add a new repetition (low half of iter/step/delay)
      agus[port_num].addRepetition(instr.iter, instr.delay, instr.step);
    } else {
      // extension: fold the high bits into the last repetition
      auto repetition_op = agus[port_num].getLastRepetitionOperator();
      uint32_t iter = instr.iter << RF_PKG::RF_INSTR_REP_ITER_BITWIDTH |
                      repetition_op.getIterations();
      uint32_t step = instr.step << RF_PKG::RF_INSTR_REP_STEP_BITWIDTH |
                      repetition_op.getStep();
      uint32_t delay = instr.delay << RF_PKG::RF_INSTR_REP_DELAY_BITWIDTH |
                       repetition_op.getDelay();
      agus[port_num].adjustRepetition(iter, delay, step);
    }
  } catch (const std::exception &e) {
    out.fatal(CALL_INFO, -1, "REP failed: %s\n", e.what());
  }
}

void Rf::handleTRANS(const RF_PKG::TRANSInstruction &instr) {
  out.output("trans (slot=%d, port=%s, delay=%d)\n", instr.slot,
             instr.port == 0 ? "dpu" : "rst", instr.delay);

  uint32_t port_num = getRelativePortNum(instr.slot, instr.port);
  try {
    agus[port_num].addTransition(instr.delay);
    current_event_number++;
  } catch (const std::exception &e) {
    out.fatal(CALL_INFO, -1, "Failed to add transition: %s\n", e.what());
  }
}

void Rf::readWide() {
  std::vector<uint8_t> data;
  uint32_t addr =
      port_agus[DataEvent::PortType::ReadWide] * io_data_width / word_bitwidth;

  out.output("Reading bulk data (");
  std::vector<uint8_t> current_data;
  for (int i = 0; i < io_data_width / word_bitwidth; i++) {
    for (int j = 0; j < word_bitwidth / 8; j++) {
      data.push_back(registers[addr][j]);
    }
    current_data = registers[addr];
    out.print("@%d: %s", addr, formatRawDataToWords(current_data).c_str());
    current_data.clear();
    if (i < io_data_width / word_bitwidth - 1) {
      out.print(", ");
    }
    addr++;
  }
  out.print(")\n");

  // Combinational read output onto the bulk wire.
  driveOutput(0, PortChannel::BULK, data, io_data_width, /*registered=*/false);

  logTraceEvent("rf_read_wide", slot_id, true, 'X',
                {{"address", (int)port_agus[DataEvent::PortType::ReadWide]},
                 {"size", (int)(io_data_width / 8)},
                 {"data", formatRawDataToWords(data)}});
}

void Rf::readNarrow() {
  std::vector<uint8_t> data =
      registers[port_agus[DataEvent::PortType::ReadNarrow]];
  data.resize(word_bitwidth / 8); // Resize to word size

  out.output("Reading narrow data (addr=%d, size=%dbits, data=%s)\n",
             port_agus[DataEvent::PortType::ReadNarrow], word_bitwidth,
             formatRawDataToWords(data).c_str());

  // Combinational read output onto the word wire.
  driveOutput(0, PortChannel::WORD, data, word_bitwidth, /*registered=*/false);

  logTraceEvent("rf_read_narrow", slot_id, true, 'X',
                {{"address", (int)port_agus[DataEvent::PortType::ReadNarrow]},
                 {"size", (int)(word_bitwidth / 8)},
                 {"data", formatRawDataToWords(data)}});
}

void Rf::writeWide() {
  // Sample the held bulk input wire (latched by receiveDataInputs()).
  const PortValue &in = readInput(0, PortChannel::BULK);
  if (in.data.empty()) {
    out.output("writeWide: no data on bulk input wire; skipping\n");
    return;
  }

  // Calculate starting address
  uint32_t addr =
      port_agus[DataEvent::PortType::WriteWide] * io_data_width / word_bitwidth;

  out.output("Writing bulk data (");
  std::vector<uint8_t> data;
  for (size_t i = 0; i < in.data.size(); i++) {
    data.push_back(in.data[i]);
    if (data.size() == word_bitwidth / 8) {
      registers[addr] = data;
      out.print("@%d: %s", addr, formatRawDataToWords(data).c_str());
      if (i < in.data.size() - 1) {
        out.print(", ");
      }
      data.clear();
      addr++;
    }
  }
  out.print(")\n");

  logTraceEvent("rf_write_wide", slot_id, true, 'X',
                {{"address", (int)port_agus[DataEvent::PortType::WriteWide]},
                 {"size", (int)(in.bits / 8)},
                 {"data", formatRawDataToWords(in.data)}});

  std::string registers_content;
  for (auto &reg : registers) {
    registers_content += formatRawDataToWords(reg.second) + " ";
  }
  logTraceEvent("registers", slot_id, true, 'E', {});
  logTraceEvent("registers", slot_id, true, 'B',
                {{"registers", registers_content}});
}

void Rf::writeNarrow() {
  // Sample the held word input wire (latched by receiveDataInputs()).
  const PortValue &in = readInput(0, PortChannel::WORD);

  std::vector<uint8_t> data;
  data.resize(word_bitwidth / 8);
  for (size_t i = 0; i < word_bitwidth / 8 && i < in.data.size(); i++) {
    data[i] = in.data[i];
  }
  registers[port_agus[DataEvent::PortType::WriteNarrow]] = data;

  out.output("Writing narrow data (addr=%d, size=%dbits, data=%s)\n",
             port_agus[DataEvent::PortType::WriteNarrow], word_bitwidth,
             formatRawDataToWords(
                 registers[port_agus[DataEvent::PortType::WriteNarrow]])
                 .c_str());

  logTraceEvent("rf_write_narrow", slot_id, true, 'X',
                {{"address", (int)port_agus[DataEvent::PortType::WriteNarrow]},
                 {"size", (int)(word_bitwidth / 8)},
                 {"data", formatRawDataToWords(in.data)}});

  std::string registers_content;
  for (auto &reg : registers) {
    registers_content += formatRawDataToWords(reg.second) + " ";
  }
  logTraceEvent("registers", slot_id, true, 'E', {});
  logTraceEvent("registers", slot_id, true, 'B',
                {{"registers", registers_content}});
}
