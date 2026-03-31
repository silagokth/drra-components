#include "iosram_top.h"
#include "custom_backing.h"
#include "dataEvent.h"
#include "ioEvents.h"
#include "iosram_top_pkg.h"
#include "timingOperators.h"

using namespace SST;

Iosram_top::Iosram_top(SST::ComponentId_t id, SST::Params &params)
    : DRRAResource(id, params) {
  instructionHandlers = IOSRAM_TOP_PKG::createInstructionHandlers(this);
  access_time = params.find<std::string>("access_time", "0ns");
  iosram_depth = 1ULL << params.find<uint32_t>("SRAM_ADDR_WIDTH", 6);
  read_only = params.find<bool>("read_only", false);

  // Backing store
  bool found = false;
  std::string backingType = params.find<std::string>(
      "backing", "malloc", found); /* Default to using a malloc backing store */
  if (!found) {
    bool oldBackVal = params.find<bool>("do-not-back", false, found);
    if (oldBackVal)
      backingType = "none";
  } else {
    out.output("backing: %s\n", backingType.c_str());
  }

  // Backend
  std::string mallocSize =
      params.find<std::string>("backing_size_unit", "1MiB");
  SST::UnitAlgebra size(mallocSize);
  if (!size.hasUnits("B")) {
    out.fatal(CALL_INFO, -1, "Invalid memory size specified: %s\n",
              mallocSize.c_str());
  }
  size_t sizeBytes = size.getRoundedValue();

  if (backingType == "mfile") {
    std::string memoryFile = params.find<std::string>("memory_file", "");
    if (0 == memoryFile.compare("")) {
      memoryFile.clear();
    }
    try {
      backend = new SST::MemHierarchy::Backend::BackingIO(
          memoryFile, io_data_width, iosram_depth, read_only);
    } catch (int e) {
      if (e == 1) {
        out.fatal(CALL_INFO, -1, "Failed to open memory file: %s\n",
                  memoryFile.c_str());
      } else {
        out.fatal(CALL_INFO, -1, "Failed to map memory file: %s\n",
                  memoryFile.c_str());
      }
    }
  } else if (backingType == "malloc") {
    backend = new SST::MemHierarchy::Backend::BackingMalloc(sizeBytes);
  }
  out.output("Created backing store (type: %s)\n", backingType.c_str());
}

bool Iosram_top::clockTick(SST::Cycle_t currentCycle) {
  bool result = DRRAResource::clockTick(currentCycle);
  if (portsToActivate.size() > 0 && currentCycle % 10 == 0) {
    for (const auto &port : portsToActivate) {
      activatePortsForSlot(port.first, port.second);
    }
    portsToActivate.clear();
  }
  return result;
}

void Iosram_top::handleActivation(uint32_t slot_id, uint32_t ports) {
  portsToActivate[slot_id] = ports;
}

void Iosram_top::handleDSU(const IOSRAM_TOP_PKG::DSUInstruction &instr) {
  out.output("dsu (slot=%d, port=%d, option=%d, init_addr_sd=%d, init_addr=%d, "
             "port=%d)\n",
             instr.slot, instr.port, instr.option, instr.init_addr_sd,
             instr.init_addr);

  // Set initial address
  uint32_t port_num = getRelativePortNum(instr.slot, instr.port);
  agus[port_num].setInitialAddress(instr.init_addr);
  out.output("Set initial address for port %d to %d\n", port_num,
             instr.init_addr);

  std::string event_name;
  switch (port_num) {
  case DSU_RELATIVE_PORT::DSU_PORT_SRAM_READ_FROM_IO:
    event_name =
        "dsu_sram_read_from_io_" + std::to_string(current_event_number);
    agus[port_num].addEvent(
        event_name,
        [this, event_name] {
          updatePortAGUs(DSU_RELATIVE_PORT::DSU_PORT_SRAM_READ_FROM_IO);
          readFromIO();
        },
        1);
    break;
  case DSU_RELATIVE_PORT::DSU_PORT_SRAM_WRITE_TO_IO:
    out.fatal(CALL_INFO, -1,
              "Invalid DSU mode IOSRAM Top should not write to IO\n");
    event_name = "dsu_sram_write_to_io_" + std::to_string(current_event_number);
    agus[port_num].addEvent(
        event_name,
        [this, event_name] {
          updatePortAGUs(DSU_RELATIVE_PORT::DSU_PORT_SRAM_WRITE_TO_IO);
          writeToIO();
        },
        9);
    break;
  case DSU_RELATIVE_PORT::DSU_PORT_IO_WRITE_TO_SRAM:
    event_name = "dsu_io_write_to_sram_" + std::to_string(current_event_number);
    agus[port_num].addEvent(
        event_name,
        [this, event_name] {
          updatePortAGUs(DSU_RELATIVE_PORT::DSU_PORT_IO_WRITE_TO_SRAM);
          writeToSRAM();
        },
        8);
    break;
  case DSU_RELATIVE_PORT::DSU_PORT_IO_READ_FROM_SRAM:
    event_name =
        "dsu_io_read_from_sram_" + std::to_string(current_event_number);
    agus[port_num].addEvent(
        event_name,
        [this, event_name] {
          updatePortAGUs(DSU_RELATIVE_PORT::DSU_PORT_IO_READ_FROM_SRAM);
          readFromSRAM();
        },
        2);
    break;
  case DSU_RELATIVE_PORT::DSU_PORT_WRITE_BULK:
    event_name = "dsu_write_bulk_" + std::to_string(current_event_number);
    agus[port_num].addEvent(
        event_name,
        [this, event_name] {
          updatePortAGUs(DSU_RELATIVE_PORT::DSU_PORT_WRITE_BULK);
          writeBulk();
        },
        9);
    break;
  case DSU_RELATIVE_PORT::DSU_PORT_READ_BULK:
    event_name = "dsu_read_bulk_" + std::to_string(current_event_number);
    agus[port_num].addEvent(
        event_name,
        [this, event_name] {
          updatePortAGUs(DSU_RELATIVE_PORT::DSU_PORT_READ_BULK);
          readBulk();
        },
        1);
    break;

  default:
    out.fatal(CALL_INFO, -1, "Invalid DSU mode\n");
  }

  // Add event handler
  current_event_number++;
}

void Iosram_top::handleREP(const IOSRAM_TOP_PKG::REPInstruction &instr) {
  out.output("rep (slot=%d, port=%d, iter=%d, step=%d, delay=%d)\n", instr.slot,
             instr.port, instr.iter, instr.step, instr.delay);

  uint32_t port_num = getRelativePortNum(instr.slot, instr.port);

  // add repetition to the timing model
  try {
    agus[port_num].addRepetition(instr.iter, instr.delay, instr.step);
    out.output("Added repetition to port %d (iter=%d, step=%d)\n", port_num,
               instr.iter, instr.step);
  } catch (const std::exception &e) {
    out.fatal(CALL_INFO, -1, "Failed to add repetition: %s\n", e.what());
  }
}

void Iosram_top::handleREPX(const IOSRAM_TOP_PKG::REPXInstruction &instr) {
  out.output("repx (slot=%d, port=%d, iter=%d, step=%d, delay=%d)\n",
             instr.slot, instr.port, instr.iter, instr.step, instr.delay);

  uint32_t port_num = getRelativePortNum(instr.slot, instr.port);
  auto repetition_op = agus[port_num].getLastRepetitionOperator();
  uint32_t iter = instr.iter
                      << IOSRAM_TOP_PKG::IOSRAM_TOP_INSTR_REPX_ITER_BITWIDTH |
                  repetition_op.getIterations();
  uint32_t step = instr.step
                      << IOSRAM_TOP_PKG::IOSRAM_TOP_INSTR_REPX_STEP_BITWIDTH |
                  repetition_op.getStep();
  uint32_t delay = instr.delay
                       << IOSRAM_TOP_PKG::IOSRAM_TOP_INSTR_REPX_DELAY_BITWIDTH |
                   repetition_op.getDelay();
  out.output("Adjusting repetition for port %d (iter=%d, step=%d, delay=%d)\n",
             port_num, iter, step, delay);
  try {
    agus[port_num].adjustRepetition(iter, delay, step);
  } catch (const std::exception &e) {
    out.fatal(CALL_INFO, -1, "REPX failed: %s\n", e.what());
  }
}

void Iosram_top::handleTRANS(const IOSRAM_TOP_PKG::TRANSInstruction &instr) {
  out.output("trans (slot=%d, port=%d, delay=%d)\n", instr.slot, instr.port,
             instr.delay);

  uint32_t port_num = getRelativePortNum(instr.slot, instr.port);

  try {
    agus[port_num].addTransition(instr.delay);
    current_event_number++;
  } catch (const std::exception &e) {
    out.fatal(CALL_INFO, -1, "Failed to add transition: %s\n", e.what());
  }
}

void Iosram_top::readFromIO() {
  sram_read_from_io_address_buffer =
      agus[DSU_RELATIVE_PORT::DSU_PORT_SRAM_READ_FROM_IO].getAddressForCycle(
          getPortActiveCycle(DSU_RELATIVE_PORT::DSU_PORT_SRAM_READ_FROM_IO));

  IOReadRequest *readReq = new IOReadRequest();
  readReq->address = sram_read_from_io_address_buffer;
  readReq->size = io_data_width / 8;
  readReq->column_id = cell_coordinates[1];

  out.output("Sending read request to IO (addr=%d, size=%dbits)\n",
             sram_read_from_io_address_buffer, io_data_width);
  logTraceEvent("iosram_read_from_io", slot_id, true, 'X',
                {{"address", (int)sram_read_from_io_address_buffer},
                 {"size", (int)(io_data_width / 8)}});

  io_input_link->send(readReq);
}

void Iosram_top::writeToIO() {
  sram_write_to_io_address_buffer =
      agus[DSU_RELATIVE_PORT::DSU_PORT_SRAM_WRITE_TO_IO].getAddressForCycle(
          getPortActiveCycle(DSU_RELATIVE_PORT::DSU_PORT_SRAM_WRITE_TO_IO));

  IOWriteRequest *writeReq = new IOWriteRequest();
  writeReq->address = sram_write_to_io_address_buffer;
  writeReq->data = to_io_data_buffer;
  io_output_link->send(writeReq);

  out.output("Sending write request to IO (addr=%d, size=%dbits, data=%s)\n",
             writeReq->address, writeReq->data.size() * 8,
             formatRawDataToWords(writeReq->data).c_str());
  logTraceEvent("iosram_write_to_io", slot_id, true, 'X',
                {{"address", (int)sram_write_to_io_address_buffer},
                 {"size", (int)(to_io_data_buffer.size())},
                 {"data", formatRawDataToWords(to_io_data_buffer)}});
}

std::string Iosram_top::dumpBackendContent() {
  std::string result;
  std::vector<uint8_t> data;
  for (uint32_t addr = 0; addr < iosram_depth; addr++) {
    data.clear();
    backend->get(addr, io_data_width / 8, data);
    result += "[" + formatRawDataToWords(data) + "] ";
  }
  return result;
}

void Iosram_top::writeToSRAM() {
  // Check if the IO responded
  IOReadResponse *ioReadResponse =
      dynamic_cast<IOReadResponse *>(io_input_link->recv());
  if (ioReadResponse) {
    out.output("Received read response from IO (addr=%d, size=%dbits, "
               "data=%s)\n",
               ioReadResponse->address, ioReadResponse->data.size() * 8,
               formatRawDataToWords(ioReadResponse->data).c_str());
    from_io_data_buffer = ioReadResponse->data;
    if (from_io_data_buffer.size() == 0) {
      out.fatal(CALL_INFO, -1, "No data from IO\n");
    }
  } else {
    out.fatal(CALL_INFO, -1, "No response from IO\n");
  }

  // Calculate the SRAM address
  io_write_to_sram_address_buffer =
      agus[DSU_RELATIVE_PORT::DSU_PORT_IO_WRITE_TO_SRAM].getAddressForCycle(
          getPortActiveCycle(DSU_RELATIVE_PORT::DSU_PORT_IO_WRITE_TO_SRAM));

  // Write data to the backend (SRAM)
  backend->set(io_write_to_sram_address_buffer, from_io_data_buffer.size(),
               from_io_data_buffer);
  out.output("Writing to SRAM (addr=%d, size=%dbits, data=%s)\n",
             io_write_to_sram_address_buffer, from_io_data_buffer.size() * 8,
             formatRawDataToWords(from_io_data_buffer).c_str());
  logTraceEvent("io_write_to_sram", slot_id, true, 'X',
                {{"address", (int)io_write_to_sram_address_buffer},
                 {"size", (int)(from_io_data_buffer.size())},
                 {"data", formatRawDataToWords(from_io_data_buffer)}});

  // Clear the buffer
  from_io_data_buffer.clear();

  // Log memory state
  logTraceEvent("memory", slot_id, true, 'E', {});
  logTraceEvent("memory", slot_id, true, 'B',
                {{"memory", dumpBackendContent()}});
}

void Iosram_top::readFromSRAM() {
  io_read_from_sram_address_buffer =
      agus[DSU_RELATIVE_PORT::DSU_PORT_IO_READ_FROM_SRAM].getAddressForCycle(
          getPortActiveCycle(DSU_RELATIVE_PORT::DSU_PORT_IO_READ_FROM_SRAM));

  to_io_data_buffer.clear();
  backend->get(io_read_from_sram_address_buffer, io_data_width / 8,
               to_io_data_buffer);

  out.output("Reading from SRAM (addr=%d, size=%dbits, data=%s)\n",
             io_read_from_sram_address_buffer, io_data_width,
             formatRawDataToWords(to_io_data_buffer).c_str());
  logTraceEvent("io_read_from_sram", slot_id, true, 'X',
                {{"address", (int)io_read_from_sram_address_buffer},
                 {"size", (int)(io_data_width / 8)},
                 {"data", formatRawDataToWords(to_io_data_buffer)}});
}

void Iosram_top::readBulk() {
  read_bulk_address_buffer =
      agus[DSU_RELATIVE_PORT::DSU_PORT_READ_BULK].getAddressForCycle(
          getPortActiveCycle(DSU_RELATIVE_PORT::DSU_PORT_READ_BULK));
  out.output("Initiating bulk read (addr=%d, size=%dbits)\n",
             read_bulk_address_buffer, io_data_width);
  DataEvent *dataEvent = new DataEvent(DataEvent::PortType::WriteWide);
  vector<uint8_t> data;
  backend->get(read_bulk_address_buffer, io_data_width / 8, data);
  out.output("Reading bulk data (addr=%d, size=%dbits, data=%s)\n",
             read_bulk_address_buffer, io_data_width,
             formatRawDataToWords(data).c_str());
  logTraceEvent("iosram_read_bulk", slot_id, true, 'X',
                {{"address", (int)read_bulk_address_buffer},
                 {"size", (int)(io_data_width / 8)},
                 {"data", formatRawDataToWords(data)}});
  dataEvent->size = io_data_width;
  dataEvent->payload = data;
  data_links[1]->send(dataEvent);
}

void Iosram_top::writeBulk() {
  write_bulk_address_buffer =
      agus[DSU_RELATIVE_PORT::DSU_PORT_WRITE_BULK].getAddressForCycle(
          getPortActiveCycle(DSU_RELATIVE_PORT::DSU_PORT_WRITE_BULK));

  // Check if some data was received
  DataEvent *dataEvent = dynamic_cast<DataEvent *>(data_links[1]->recv());
  if (dataEvent == nullptr)
    out.fatal(CALL_INFO, -1, "No data received\n");

  // Write data to the backend
  backend->set(write_bulk_address_buffer, dataEvent->size / 8,
               dataEvent->payload);

  out.output("Writing bulk data (addr=%d, size=%dbits, data=%s)\n",
             write_bulk_address_buffer, dataEvent->size,
             formatRawDataToWords(dataEvent->payload).c_str());
  logTraceEvent("iosram_write_bulk", slot_id, true, 'X',
                {{"address", (int)write_bulk_address_buffer},
                 {"size", (int)(dataEvent->size / 8)},
                 {"data", formatRawDataToWords(dataEvent->payload)}});

  // Log memory state
  logTraceEvent("memory", slot_id, true, 'E', {});
  logTraceEvent("memory", slot_id, true, 'B',
                {{"memory", dumpBackendContent()}});
}
