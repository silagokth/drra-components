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

  // What this resource does with each AGU's address. The base fires these on
  // every cycle the port's AGU produces an address; reads land early in the
  // cycle and writes late, so a value produced by another resource this cycle
  // is written back on the next one.
  registerPortAction(
      RF_PKG::EVT_PORT_WORD_READ, 3, "rf_read_narrow",
      [this](int64_t address) { readNarrow(address); }, register_file_size);
  registerPortAction(
      RF_PKG::EVT_PORT_BULK_READ, 3, "rf_read_wide",
      [this](int64_t address) { readWide(address); }, register_file_size);
  registerPortAction(
      RF_PKG::EVT_PORT_WORD_WRITE, 8, "rf_write_narrow",
      [this](int64_t address) { writeNarrow(address); }, register_file_size);
  registerPortAction(
      RF_PKG::EVT_PORT_BULK_WRITE, 8, "rf_write_wide",
      [this](int64_t address) { writeWide(address); }, register_file_size);

  std::string registers_content;
  for (auto &reg : registers) {
    registers_content += formatRawDataToWords(reg.second) + " ";
  }
  logTraceEvent("registers", slot_id, true, 'B',
                {{"registers", registers_content}});
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

void Rf::logRegisters() {
  std::string registers_content;
  for (auto &reg : registers) {
    registers_content += formatRawDataToWords(reg.second) + " ";
  }
  logTraceEvent("registers", slot_id, true, 'E', {});
  logTraceEvent("registers", slot_id, true, 'B',
                {{"registers", registers_content}});
}

void Rf::readWide(int64_t address) {
  std::vector<uint8_t> data;
  uint32_t addr = address * io_data_width / word_bitwidth;

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
                {{"address", (int)address},
                 {"size", (int)(io_data_width / 8)},
                 {"data", formatRawDataToWords(data)}});
}

void Rf::readNarrow(int64_t address) {
  DataEvent *dataEvent = new DataEvent(DataEvent::PortType::WriteNarrow);
  std::vector<uint8_t> data = registers[address];
  data.resize(word_bitwidth / 8); // Resize to word size

  dataEvent->size = word_bitwidth;
  dataEvent->payload = data;
  out.output("Reading narrow data (addr=%ld, size=%dbits, data=%s)\n",
             (long)address, word_bitwidth,
             formatRawDataToWords(data).c_str());

  data_links[0]->send(dataEvent);

  logTraceEvent("rf_read_narrow", slot_id, true, 'X',
                {{"address", (int)address},
                 {"size", (int)(word_bitwidth / 8)},
                 {"data", formatRawDataToWords(data)}});
}

void Rf::writeWide(int64_t address) {
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
  uint32_t addr = address * io_data_width / word_bitwidth;

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
                {{"address", (int)address},
                 {"size", (int)(data_event->size / 8)},
                 {"data", formatRawDataToWords(data_event->payload)}});

  logRegisters();
}

void Rf::writeNarrow(int64_t address) {
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
  registers[address] = data;

  out.output("Writing narrow data (addr=%ld, size=%dbits, data=%s)\n",
             (long)address, word_bitwidth,
             formatRawDataToWords(registers[address]).c_str());

  logTraceEvent("rf_write_narrow", slot_id, true, 'X',
                {{"address", (int)address},
                 {"size", (int)(word_bitwidth / 8)},
                 {"data", formatRawDataToWords(data_event->payload)}});

  logRegisters();
}
