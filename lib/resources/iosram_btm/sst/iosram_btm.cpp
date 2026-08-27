#include "iosram_btm.h"
#include "custom_backing.h"
#include "dataEvent.h"
#include "ioEvents.h"
#include "iosram_btm_pkg.h"
#include "timingOperators.h"

using namespace SST;

Iosram_btm::Iosram_btm(SST::ComponentId_t id, SST::Params &params)
    : DRRAResource(id, params) {
  instructionHandlers = IOSRAM_BTM_PKG::createInstructionHandlers(this);
  access_time = params.find<std::string>("access_time", "0ns");
  iosram_depth = 1ULL << params.find<uint32_t>("SRAM_ADDR_WIDTH", 6);
  io_address_width = params.find<uint32_t>("IO_ADDR_WIDTH", 16);
  read_only = params.find<bool>("read_only", false);

  // Ports addressing the IO side are bounded by the io address space, not by
  // the local SRAM geometry -- they address the external io input/output
  // buffer. (Carried forward from 757bfe8, which fixed this in the hand-written
  // bounds check that registerPortAction's max_addr replaced.) SRAM-side ports
  // are bounded by iosram_depth below.
  const uint64_t io_side_depth = 1ULL << io_address_width;

  // What this resource does with each AGU's address, and when in the
  // cycle it does it. The base fires each one on every cycle its port's
  // AGU produces an address, and range-checks that address first.
  //
  // This variant carries IO in one direction only, so the opposite
  // port has no action and an EVT naming it is an error.
  forbidPort(DSU_PORT_SRAM_READ_FROM_IO, "IOSRAM Btm does not read from IO");

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

void Iosram_btm::handleCONF(const IOSRAM_BTM_PKG::CONFInstruction &instr) {
  out.output("conf (slot=%d)\n", instr.slot);
}

void Iosram_btm::readFromIO(int64_t address) {
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

void Iosram_btm::writeToIO(int64_t address) {
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

std::string Iosram_btm::dumpBackendContent() {
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

void Iosram_btm::writeToSRAM(int64_t address) {
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

void Iosram_btm::readFromSRAM(int64_t address) {
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

void Iosram_btm::readBulk(int64_t address) {
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

void Iosram_btm::writeBulk(int64_t address) {
  // Check if some data was received
  DataEvent *dataEvent = dynamic_cast<DataEvent *>(data_links[1]->recv());
  if (dataEvent == nullptr)
    out.fatal(CALL_INFO, -1, "No data received\n");

  // RTL SRAM write-port arbitration (iosram_btm.sv "input ports" always_comb):
  //   if (agu_valid[2]) [io_write_to_sram]  <-- priority
  //   else if (agu_valid[4]) [write_bulk / drain]
  // The io_write_to_sram (input-staging) AGU has priority over the write_bulk
  // (drain) AGU at the shared single SRAM write port. When input-staging is
  // producing an address in the same cycle, RTL masks the drain write entirely.
  // SST otherwise commits both writes as independent backend->set() calls,
  // silently keeping a drain RTL drops -- the F1 divergence (see iosram_both).
  // Latent in the 3-cell fabric today (staging and drain never overlap), but
  // reproduce the arbitration for faithfulness. No-op for those schedules.
  // agu_array.addrValid() is the direct replacement for the retired
  // getAddressForCycle(getPortActiveCycle(port)) >= 0: both mean "this
  // AGU produces an address this cycle". Safe to read here -- outputs
  // settle at subcycle 2, io_write_to_sram runs at 7 and write_bulk at 8.
  bool io_write_to_sram_active =
      isPortActive(DSU_RELATIVE_PORT::DSU_PORT_IO_WRITE_TO_SRAM) &&
      isPortAddressValid(DSU_RELATIVE_PORT::DSU_PORT_IO_WRITE_TO_SRAM);
  if (io_write_to_sram_active) {
    logTraceEvent("iosram_write_bulk_dropped", slot_id, true, 'X',
                  {{"address", (int)address}});
    delete dataEvent;
    return;
  }

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
