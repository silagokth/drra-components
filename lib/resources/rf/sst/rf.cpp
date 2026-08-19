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
  bool result = DRRAResource::clockTick(currentCycle);
  if (portsToActivate.size() > 0 && currentCycle % 10 == 0) {
    for (const auto &port : portsToActivate) {
      activatePortsForSlot(port.first, port.second);
    }
    portsToActivate.clear();
  }
  return result;
}

void Rf::handleActivation(uint32_t slot_id, uint32_t ports) {
  portsToActivate[slot_id] = ports;
}

void Rf::handleCONF(const RF_PKG::CONFInstruction &instr) {
  out.output("conf (slot=%d, address=%d, value=%d)\n", instr.slot,
             instr.address, instr.value);

  if (instr.address >= register_file_size) {
    out.fatal(CALL_INFO, -1, "Invalid CONF address (greater than RF size)\n");
  }

  // CONF initialises one register directly from the instruction stream.
  registers[instr.address] = uint64ToVector(instr.value);
}

void Rf::handleEVT(const RF_PKG::EVTInstruction &instr) {
  out.output(
      "evt (slot=%d, option=%d, port=%d, init_addr_sd=%d, init_addr=%d)\n",
      instr.slot, instr.option, instr.port, instr.init_addr_sd,
      instr.init_addr);

  auto evt = instr;

  port_agus_init[evt.port] = evt.init_addr;
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
        1);
    break;
  case DataEvent::PortType::ReadWide:
    event_name = "evt_read_wide_" + std::to_string(current_event_number);
    agus[evt.port].addEvent(
        event_name,
        [this, event_name] {
          updatePortAGUs(DataEvent::PortType::ReadWide);
          readWide();
        },
        1);
    break;
  case DataEvent::PortType::WriteNarrow:
    event_name = "evt_write_narrow_" + std::to_string(current_event_number);
    agus[evt.port].addEvent(
        event_name,
        [this, event_name] {
          updatePortAGUs(DataEvent::PortType::WriteNarrow);
          writeNarrow();
        },
        8);
    break;
  case DataEvent::PortType::WriteWide:
    event_name = "evt_write_wide_" + std::to_string(current_event_number);
    agus[evt.port].addEvent(
        event_name,
        [this, event_name] {
          updatePortAGUs(DataEvent::PortType::WriteWide);
          writeWide();
        },
        8);
    break;

  default:
    out.fatal(CALL_INFO, -1, "Invalid EVT mode\n");
  }

  // Add event handler
  current_event_number++;
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

  DataEvent *dataEvent = new DataEvent(DataEvent::PortType::WriteWide);
  dataEvent->size = io_data_width;
  dataEvent->payload = data;

  data_links[0]->send(dataEvent);

  logTraceEvent("rf_read_wide", slot_id, true, 'X',
                {{"address", (int)port_agus[DataEvent::PortType::ReadWide]},
                 {"size", (int)(io_data_width / 8)},
                 {"data", formatRawDataToWords(data)}});
}

void Rf::readNarrow() {
  DataEvent *dataEvent = new DataEvent(DataEvent::PortType::WriteNarrow);
  std::vector<uint8_t> data =
      registers[port_agus[DataEvent::PortType::ReadNarrow]];
  data.resize(word_bitwidth / 8); // Resize to word size

  dataEvent->size = word_bitwidth;
  dataEvent->payload = data;
  out.output("Reading narrow data (addr=%d, size=%dbits, data=%s)\n",
             port_agus[DataEvent::PortType::ReadNarrow], word_bitwidth,
             formatRawDataToWords(data).c_str());

  data_links[0]->send(dataEvent);

  logTraceEvent("rf_read_narrow", slot_id, true, 'X',
                {{"address", (int)port_agus[DataEvent::PortType::ReadNarrow]},
                 {"size", (int)(word_bitwidth / 8)},
                 {"data", formatRawDataToWords(data)}});
}

void Rf::writeWide() {
  Event *temp_event = nullptr;
  DataEvent *data_event = nullptr;
  do {
    temp_event = data_links[0]->recv();
    if (temp_event != nullptr) {
      DataEvent *new_data_event = dynamic_cast<DataEvent *>(temp_event);
      if (new_data_event != nullptr) {
        if (data_event != nullptr)
          delete data_event;
        data_event = new_data_event;
      }
    }
  } while (temp_event != nullptr);

  if (data_event == nullptr)
    out.fatal(CALL_INFO, -1, "Failed to receive data event (writeWide)\n");
  if (data_event->portType != DataEvent::PortType::WriteWide)
    out.fatal(CALL_INFO, -1, "Invalid port type: %d\n", data_event->portType);

  // Calculate starting address
  uint32_t addr =
      port_agus[DataEvent::PortType::WriteWide] * io_data_width / word_bitwidth;

  out.output("Writing bulk data (");
  std::vector<uint8_t> data;
  for (int i = 0; i < data_event->payload.size(); i++) {
    data.push_back(data_event->payload[i]);
    if (data.size() == word_bitwidth / 8) {
      registers[addr] = data;
      out.print("@%d: %s", addr, formatRawDataToWords(data).c_str());
      if (i < data_event->payload.size() - 1) {
        out.print(", ");
      }
      data.clear();
      addr++;
    }
  }
  out.print(")\n");

  logTraceEvent("rf_write_wide", slot_id, true, 'X',
                {{"address", (int)port_agus[DataEvent::PortType::WriteWide]},
                 {"size", (int)(data_event->size / 8)},
                 {"data", formatRawDataToWords(data_event->payload)}});

  std::string registers_content;
  for (auto &reg : registers) {
    registers_content += formatRawDataToWords(reg.second) + " ";
  }
  logTraceEvent("registers", slot_id, true, 'E', {});
  logTraceEvent("registers", slot_id, true, 'B',
                {{"registers", registers_content}});
}

void Rf::writeNarrow() {
  Event *temp_event = nullptr;
  DataEvent *data_event = nullptr;
  do {
    temp_event = data_links[0]->recv();
    if (temp_event != nullptr) {
      DataEvent *new_data_event = dynamic_cast<DataEvent *>(temp_event);
      if (new_data_event != nullptr) {
        if (data_event != nullptr)
          delete data_event;
        data_event = new_data_event;
      }
    }
  } while (temp_event != nullptr);

  if (data_event == nullptr)
    out.fatal(CALL_INFO, -1, "Failed to receive data event (writeNarrow)\n");
  if (data_event->portType != DataEvent::PortType::WriteNarrow)
    out.fatal(CALL_INFO, -1, "Invalid port type\n");

  std::vector<uint8_t> data;
  data.resize(word_bitwidth / 8);
  for (int i = 0; i < word_bitwidth / 8; i++) {
    data[i] = data_event->payload[i];
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
                 {"data", formatRawDataToWords(data_event->payload)}});

  std::string registers_content;
  for (auto &reg : registers) {
    registers_content += formatRawDataToWords(reg.second) + " ";
  }
  logTraceEvent("registers", slot_id, true, 'E', {});
  logTraceEvent("registers", slot_id, true, 'B',
                {{"registers", registers_content}});
}
