#include "iosram_both.h"
#include "custom_backing.h"
#include "dataEvent.h"
#include "ioEvents.h"
#include "iosram_both_pkg.h"
#include "timingOperators.h"

using namespace SST;

Iosram_both::Iosram_both(SST::ComponentId_t id, SST::Params &params)
    : DRRAResource(id, params) {
  instructionHandlers = IOSRAM_BOTH_PKG::createInstructionHandlers(this);
  access_time = params.find<std::string>("access_time", "0ns");
  iosram_depth = 1ULL << params.find<uint32_t>("SRAM_ADDR_WIDTH", 6);
  read_only = params.find<bool>("read_only", false);

  // Ports addressing the IO side count in io_data_width words, the SRAM side
  // in SRAM rows.
  const uint64_t io_side_depth = iosram_depth * (io_data_width / word_bitwidth);

  // What this resource does with each AGU's address, and when in the
  // cycle it does it. The base fires each one on every cycle its port's
  // AGU produces an address, and range-checks that address first.

  registerPortAction(
      DSU_PORT_SRAM_READ_FROM_IO, 3, "dsu_sram_read_from_io",
      [this](int64_t address) { readFromIO(address); }, io_side_depth);
  registerPortAction(
      DSU_PORT_SRAM_WRITE_TO_IO, 8, "dsu_sram_write_to_io",
      [this](int64_t address) { writeToIO(address); }, io_side_depth);
  registerPortAction(
      DSU_PORT_IO_WRITE_TO_SRAM, 7, "dsu_io_write_to_sram",
      [this](int64_t address) { writeToSRAM(address); }, iosram_depth);
  registerPortAction(
      DSU_PORT_IO_READ_FROM_SRAM, 4, "dsu_io_read_from_sram",
      [this](int64_t address) { readFromSRAM(address); }, iosram_depth);
  registerPortAction(
      DSU_PORT_WRITE_BULK, 8, "dsu_write_bulk",
      [this](int64_t address) { writeBulk(address); }, iosram_depth);
  registerPortAction(
      DSU_PORT_READ_BULK, 3, "dsu_read_bulk",
      [this](int64_t address) { readBulk(address); }, iosram_depth);

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

void Iosram_both::handleCONF(const IOSRAM_BOTH_PKG::CONFInstruction &instr) {
  out.output("conf (slot=%d)\n", instr.slot);
}

void Iosram_both::readFromIO(int64_t address) {
  IOReadRequest *readReq = new IOReadRequest();
  readReq->address = address;
  readReq->size = io_data_width / 8;
  readReq->column_id = cell_coordinates[1];

  out.output("Sending read request to IO (addr=%ld, size=%dbits)\n",
             (long)address, io_data_width);
  logTraceEvent("iosram_read_from_io", slot_id, true, 'X',
                {{"address", (int)address},
                 {"size", (int)(io_data_width / 8)}});

  io_input_link->send(readReq);
}

void Iosram_both::writeToIO(int64_t address) {
  IOWriteRequest *writeReq = new IOWriteRequest();
  writeReq->address = address;
  writeReq->data = to_io_data_buffer;
  io_output_link->send(writeReq);

  out.output("Sending write request to IO (addr=%d, size=%dbits, data=%s)\n",
             writeReq->address, writeReq->data.size() * 8,
             formatRawDataToWords(writeReq->data).c_str());
  logTraceEvent("iosram_write_to_io", slot_id, true, 'X',
                {{"address", (int)address},
                 {"size", (int)(to_io_data_buffer.size())},
                 {"data", formatRawDataToWords(to_io_data_buffer)}});
}

std::string Iosram_both::dumpBackendContent() {
  if (!debug_enabled)
    return ""; // debug disabled: skip full-SRAM dump built for trace args
  std::string result;
  std::vector<uint8_t> data;
  for (uint32_t addr = 0; addr < iosram_depth; addr++) {
    data.clear();
    backend->get(addr, io_data_width / 8, data);
    result += "[" + formatRawDataToWords(data) + "] ";
  }
  return result;
}

void Iosram_both::writeToSRAM(int64_t address) {
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

  // Write data to the backend (SRAM)
  backend->set(address, from_io_data_buffer.size(), from_io_data_buffer);
  out.output("Writing to SRAM (addr=%ld, size=%dbits, data=%s)\n",
             (long)address, from_io_data_buffer.size() * 8,
             formatRawDataToWords(from_io_data_buffer).c_str());
  logTraceEvent("io_write_to_sram", slot_id, true, 'X',
                {{"address", (int)address},
                 {"size", (int)(from_io_data_buffer.size())},
                 {"data", formatRawDataToWords(from_io_data_buffer)}});

  // Clear the buffer
  from_io_data_buffer.clear();

  // Log memory state
  logTraceEvent("memory", slot_id, true, 'E', {});
  logTraceEvent("memory", slot_id, true, 'B',
                {{"memory", dumpBackendContent()}});
}

void Iosram_both::readFromSRAM(int64_t address) {
  to_io_data_buffer.clear();
  backend->get(address, io_data_width / 8, to_io_data_buffer);

  out.output("Reading from SRAM (addr=%ld, size=%dbits, data=%s)\n",
             (long)address, io_data_width,
             formatRawDataToWords(to_io_data_buffer).c_str());
  logTraceEvent("io_read_from_sram", slot_id, true, 'X',
                {{"address", (int)address},
                 {"size", (int)(io_data_width / 8)},
                 {"data", formatRawDataToWords(to_io_data_buffer)}});
}

void Iosram_both::readBulk(int64_t address) {
  out.output("Initiating bulk read (addr=%ld, size=%dbits)\n", (long)address,
             io_data_width);
  DataEvent *dataEvent = new DataEvent(DataEvent::PortType::WriteWide);
  std::vector<uint8_t> data;
  backend->get(address, io_data_width / 8, data);
  out.output("Reading bulk data (addr=%ld, size=%dbits, data=%s)\n",
             (long)address, io_data_width,
             formatRawDataToWords(data).c_str());
  logTraceEvent("iosram_read_bulk", slot_id, true, 'X',
                {{"address", (int)address},
                 {"size", (int)(io_data_width / 8)},
                 {"data", formatRawDataToWords(data)}});
  dataEvent->size = io_data_width;
  dataEvent->payload = data;
  data_links[1]->send(dataEvent);
}

void Iosram_both::writeBulk(int64_t address) {
  // Check if some data was received
  DataEvent *dataEvent = dynamic_cast<DataEvent *>(data_links[1]->recv());
  if (dataEvent == nullptr)
    out.fatal(CALL_INFO, -1, "No data received\n");

  // Write data to the backend
  backend->set(address, dataEvent->size / 8, dataEvent->payload);

  out.output("Writing bulk data (addr=%ld, size=%dbits, data=%s)\n",
             (long)address, dataEvent->size,
             formatRawDataToWords(dataEvent->payload).c_str());
  logTraceEvent("iosram_write_bulk", slot_id, true, 'X',
                {{"address", (int)address},
                 {"size", (int)(dataEvent->size / 8)},
                 {"data", formatRawDataToWords(dataEvent->payload)}});

  // Log memory state
  logTraceEvent("memory", slot_id, true, 'E', {});
  logTraceEvent("memory", slot_id, true, 'B',
                {{"memory", dumpBackendContent()}});
}
