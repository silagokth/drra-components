#include "rf.h"
#include "dataEvent.h"
#include "timingOperators.h"

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
}

bool Rf::clockTick(SST::Cycle_t currentCycle) {
  return DRRAResource::clockTick(currentCycle);
}

void Rf::handleDSU(const RF_PKG::DSUInstruction &instr) {
  out.output("dsu (slot=%d, init_addr_sd=%d, init_addr=%d, port=%d)\n",
             instr.slot, instr.init_addr_sd, instr.init_addr, instr.port);

  auto dsu = instr;

  port_agus_init[dsu.port] = dsu.init_addr;

  // Add the event handler
  std::string event_name;
  switch (dsu.port) {
  case DataEvent::PortType::ReadNarrow:
    event_name = "dsu_read_narrow_" + std::to_string(current_event_number);
    next_timing_states[dsu.port].addEvent(event_name, 1, [this, event_name] {
      updatePortAGUs(DataEvent::PortType::ReadNarrow);
      readNarrow();
    });
    break;
  case DataEvent::PortType::ReadWide:
    event_name = "dsu_read_wide_" + std::to_string(current_event_number);
    next_timing_states[dsu.port].addEvent(event_name, 1, [this, event_name] {
      updatePortAGUs(DataEvent::PortType::ReadWide);
      readWide();
    });
    break;
  case DataEvent::PortType::WriteNarrow:
    event_name = "dsu_write_narrow_" + std::to_string(current_event_number);
    next_timing_states[dsu.port].addEvent(event_name, 9, [this, event_name] {
      updatePortAGUs(DataEvent::PortType::WriteNarrow);
      writeNarrow();
    });
    break;
  case DataEvent::PortType::WriteWide:
    event_name = "dsu_write_wide_" + std::to_string(current_event_number);
    next_timing_states[dsu.port].addEvent(event_name, 9, [this, event_name] {
      updatePortAGUs(DataEvent::PortType::WriteWide);
      writeWide();
    });
    break;

  default:
    out.fatal(CALL_INFO, -1, "Invalid DSU mode\n");
  }

  // Add event handler
  current_event_number++;
}

void Rf::handleREP(const RF_PKG::REPInstruction &instr) {
  out.output("rep (slot=%d, port=%d, level=%d, iter=%d, step=%d, delay=%d)\n",
             instr.slot, instr.port, instr.level, instr.iter, instr.step,
             instr.delay);

  auto rep = instr;
  uint32_t port_num = 0;
  auto it = std::find(slot_ids.begin(), slot_ids.end(), rep.slot);
  if (it != slot_ids.end()) {
    port_num = std::distance(slot_ids.begin(), it);
  } else {
    out.fatal(CALL_INFO, -1, "Slot ID not found\n");
  }
  port_num = port_num * 4 + rep.port;

  // For now, we only support increasing repetition levels (and no skipping)
  if (rep.level != port_last_rep_level[port_num] + 1) {
    out.fatal(CALL_INFO, -1, "Invalid repetition level (last=%u, curr=%u)\n",
              port_last_rep_level[port_num], rep.level);
  } else {
    port_last_rep_level[port_num] = rep.level;
  }

  // add repetition to the timing model
  try {
    next_timing_states[port_num].addRepetition(rep.iter, rep.delay, rep.level,
                                               rep.step);
  } catch (const std::exception &e) {
    out.fatal(CALL_INFO, -1, "Failed to add repetition: %s\n", e.what());
  }
}

void Rf::handleREPX(const RF_PKG::REPXInstruction &instr) {
  out.output("repx (slot=%d, port=%d, level=%d, iter=%d, step=%d, delay=%d)\n",
             instr.slot, instr.port, instr.level, instr.iter, instr.step,
             instr.delay);

  auto repx = instr;
  uint32_t port_num = 0;
  auto it = std::find(slot_ids.begin(), slot_ids.end(), repx.slot);
  if (it != slot_ids.end()) {
    port_num = std::distance(slot_ids.begin(), it);
  } else {
    out.fatal(CALL_INFO, -1, "Slot ID not found\n");
  }
  port_num = port_num * 4 + repx.port;

  auto repetition_op =
      next_timing_states[port_num].getRepetitionOperatorFromLevel(repx.level);
  uint32_t iter = repx.iter << 6 | repetition_op.getIterations();
  uint32_t step = repx.step << 6 | repetition_op.getStep();
  uint32_t delay = repx.delay << 6 | repetition_op.getDelay();
  try {
    next_timing_states[port_num].adjustRepetition(iter, delay, repx.level,
                                                  step);
  } catch (const std::exception &e) {
    out.fatal(CALL_INFO, -1, "REPX failed: %s\n", e.what());
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

  DataEvent *dataEvent = new DataEvent(DataEvent::PortType::WriteWide);
  dataEvent->size = io_data_width;
  dataEvent->payload = data;

  data_links[0]->send(dataEvent);
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

  logTraceEvent("rf_read_narrow", 0, true,
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

  logTraceEvent("rf_write_wide", 0, true,
                {{"address", (int)port_agus[DataEvent::PortType::WriteWide]},
                 {"size", (int)(data_event->size / 8)},
                 {"data", formatRawDataToWords(data_event->payload)}});
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
}
