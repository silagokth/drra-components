#include "iosram_btm.h"
#include "custom_backing.h"
#include "dataEvent.h"
#include "ioEvents.h"
#include "iosram_btm_pkg.h"

using namespace SST;

Iosram_btm::Iosram_btm(SST::ComponentId_t id, SST::Params &params)
    : DRRAResource(id, params) {
  instructionHandlers = IOSRAM_BTM_PKG::createInstructionHandlers(this);
  access_time = params.find<std::string>("access_time", "0ns");
  iosram_depth = 2 ^ params.find<uint32_t>("SRAM_ADDR_WIDTH", 6);
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

bool Iosram_btm::clockTick(SST::Cycle_t currentCycle) {
  return DRRAResource::clockTick(currentCycle);
}

void Iosram_btm::handleDSU(const IOSRAM_BTM_PKG::DSUInstruction &instr) {
  out.output("dsu (slot=%d, init_addr_sd=%d, init_addr=%d, port=%d)\n",
             instr.slot, instr.init_addr_sd, instr.init_addr, instr.port);

  // Set initial address
  agu_initial_addr = instr.init_addr;

  // Add the event handler
  uint32_t port_num = getRelativePortNum(instr.slot, instr.port);
  switch (port_num) {
  case IOSRAM_BTM_PKG::DSU_PORT::DSU_PORT_SRAM_READ_FROM_IO:
    out.fatal(CALL_INFO, -1,
              "Invalid DSU mode IOSRAM Btm should not read from IO\n");
    sram_read_from_io_initial_addr = instr.init_addr;
    readFromIO();
    break;
  case IOSRAM_BTM_PKG::DSU_PORT::DSU_PORT_SRAM_WRITE_TO_IO:
    sram_write_to_io_initial_addr = instr.init_addr;
    writeToIO();
    break;
  case IOSRAM_BTM_PKG::DSU_PORT::DSU_PORT_IO_WRITE_TO_SRAM:
    io_write_to_sram_initial_addr = instr.init_addr;
    writeToSRAM();
    break;
  case IOSRAM_BTM_PKG::DSU_PORT::DSU_PORT_IO_READ_FROM_SRAM:
    io_read_from_sram_initial_addr = instr.init_addr;
    readFromSRAM();
    break;
  case IOSRAM_BTM_PKG::DSU_PORT::DSU_PORT_WRITE_BULK:
    write_bulk_initial_addr = instr.init_addr;
    writeBulk();
    break;
  case IOSRAM_BTM_PKG::DSU_PORT::DSU_PORT_READ_BULK:
    read_bulk_initial_addr = instr.init_addr;
    readBulk();
    break;

  default:
    out.fatal(CALL_INFO, -1, "Invalid DSU mode\n");
  }

  // Add event handler
  current_event_number++;
}

void Iosram_btm::handleREP(const IOSRAM_BTM_PKG::REPInstruction &instr) {
  out.output("rep (slot=%d, port=%d, level=%d, iter=%d, step=%d, delay=%d)\n",
             instr.slot, instr.port, instr.level, instr.iter, instr.step,
             instr.delay);

  uint32_t port_num = 0;
  auto it = std::find(slot_ids.begin(), slot_ids.end(), instr.slot);
  if (it != slot_ids.end()) {
    port_num = std::distance(slot_ids.begin(), it);
  } else {
    out.fatal(CALL_INFO, -1, "Slot ID not found\n");
  }
  port_num = port_num * 4 + instr.port;

  // For now, we only support increasing repetition levels (and no skipping)
  if (instr.level != port_last_rep_level[port_num] + 1) {
    out.output("port_num = %d\n", port_num);
    out.fatal(
        CALL_INFO, -1,
        "Invalid repetition level (last=%d, curr=%d), instruction: (slot: "
        "%d, port: %d, level: %d, iter: %d, step: %d, delay: %d)\n",
        port_last_rep_level[port_num], instr.level, instr.slot, instr.port,
        instr.level, instr.iter, instr.step, instr.delay);
  } else {
    port_last_rep_level[port_num] = instr.level;
  }

  // add repetition to the timing model
  try {
    next_timing_states[port_num].addRepetition(instr.iter, instr.delay,
                                               instr.level, instr.step);
    out.output("Added repetition to port %d (iter=%d, step=%d)\n", port_num,
               instr.iter, instr.step);
  } catch (const std::exception &e) {
    out.fatal(CALL_INFO, -1, "Failed to add repetition: %s\n", e.what());
  }
}

void Iosram_btm::handleREPX(const IOSRAM_BTM_PKG::REPXInstruction &instr) {
  out.output("repx (slot=%d, port=%d, level=%d, iter=%d, step=%d, delay=%d)\n",
             instr.slot, instr.port, instr.level, instr.iter, instr.step,
             instr.delay);

  uint32_t port_num = 0;
  auto it = std::find(slot_ids.begin(), slot_ids.end(), instr.slot);
  if (it != slot_ids.end()) {
    port_num = std::distance(slot_ids.begin(), it);
  } else {
    out.fatal(CALL_INFO, -1, "Slot ID not found\n");
  }
  port_num = port_num * 4 + instr.port;

  auto repetition_op =
      next_timing_states[port_num].getRepetitionOperatorFromLevel(instr.level);
  uint32_t iter = instr.iter << 6 | repetition_op.getIterations();
  uint32_t step = instr.step << 6 | repetition_op.getStep();
  uint32_t delay = instr.delay << 6 | repetition_op.getDelay();
  try {
    next_timing_states[port_num].adjustRepetition(iter, delay, instr.level,
                                                  step);
  } catch (const std::exception &e) {
    out.fatal(CALL_INFO, -1, "REPX failed: %s\n", e.what());
  }
}

void Iosram_btm::readFromIO() {
  // Reading data from the IO to the buffer
  next_timing_states[IOSRAM_BTM_PKG::DSU_PORT_SRAM_READ_FROM_IO].addEvent(
      "dsu_read_from_io_" + std::to_string(current_event_number), 1, [this] {
        sram_read_from_io_address_buffer =
            sram_read_from_io_initial_addr +
            current_timing_states[IOSRAM_BTM_PKG::DSU_PORT_SRAM_READ_FROM_IO]
                .getRepIncrementForCycle(getPortActiveCycle(
                    IOSRAM_BTM_PKG::DSU_PORT_SRAM_READ_FROM_IO));

        IOReadRequest *readReq = new IOReadRequest();
        readReq->address = sram_read_from_io_address_buffer;
        readReq->size = io_data_width / 8;
        readReq->column_id = cell_coordinates[1];

        out.output("Sending read request to IO (addr=%d, size=%dbits)\n",
                   sram_read_from_io_address_buffer, io_data_width);

        io_input_link->send(readReq);
      });
}

void Iosram_btm::writeToIO() {
  // Writing buffer data to the IO
  next_timing_states[IOSRAM_BTM_PKG::DSU_PORT_SRAM_WRITE_TO_IO].addEvent(
      "dsu_write_to_io_" + std::to_string(current_event_number), 9, [this] {
        sram_write_to_io_address_buffer =
            sram_write_to_io_initial_addr +
            current_timing_states[IOSRAM_BTM_PKG::DSU_PORT_SRAM_WRITE_TO_IO]
                .getRepIncrementForCycle(getPortActiveCycle(
                    IOSRAM_BTM_PKG::DSU_PORT_SRAM_WRITE_TO_IO));

        IOWriteRequest *writeReq = new IOWriteRequest();
        writeReq->address = sram_write_to_io_address_buffer;
        writeReq->data = to_io_data_buffer;
        io_output_link->send(writeReq);

        out.output(
            "Sending write request to IO (addr=%d, size=%dbits, data=%s)\n",
            writeReq->address, writeReq->data.size() * 8,
            formatRawDataToWords(writeReq->data).c_str());
      });
}

void Iosram_btm::writeToSRAM() {
  // Writing buffer data to the backend
  next_timing_states[IOSRAM_BTM_PKG::DSU_PORT_IO_WRITE_TO_SRAM].addEvent(
      "dsu_write_to_sram_" + std::to_string(current_event_number), 8, [this] {
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
            io_write_to_sram_initial_addr +
            current_timing_states[IOSRAM_BTM_PKG::DSU_PORT_IO_WRITE_TO_SRAM]
                .getRepIncrementForCycle(getPortActiveCycle(
                    IOSRAM_BTM_PKG::DSU_PORT_IO_WRITE_TO_SRAM));

        // Write data to the backend (SRAM)
        backend->set(io_write_to_sram_address_buffer,
                     from_io_data_buffer.size(), from_io_data_buffer);
        out.output("Writing to SRAM (addr=%d, size=%dbits, data=%s)\n",
                   io_write_to_sram_address_buffer,
                   from_io_data_buffer.size() * 8,
                   formatRawDataToWords(from_io_data_buffer).c_str());

        // Clear the buffer
        from_io_data_buffer.clear();
      });
}

void Iosram_btm::readFromSRAM() {
  // Reading data from the backend to the buffer
  next_timing_states[IOSRAM_BTM_PKG::DSU_PORT_IO_READ_FROM_SRAM].addEvent(
      "dsu_read_from_sram_" + std::to_string(current_event_number), 2, [this] {
        io_read_from_sram_address_buffer =
            io_read_from_sram_initial_addr +
            current_timing_states[IOSRAM_BTM_PKG::DSU_PORT_IO_READ_FROM_SRAM]
                .getRepIncrementForCycle(getPortActiveCycle(
                    IOSRAM_BTM_PKG::DSU_PORT_IO_READ_FROM_SRAM));

        to_io_data_buffer.clear();
        backend->get(io_read_from_sram_address_buffer, io_data_width / 8,
                     to_io_data_buffer);

        out.output("Reading from SRAM (addr=%d, size=%dbits, data=%s)\n",
                   io_read_from_sram_address_buffer, io_data_width,
                   formatRawDataToWords(to_io_data_buffer).c_str());
      });
}

void Iosram_btm::readBulk() {
  out.output("Add event read bulk (port %d prio %d)\n",
             IOSRAM_BTM_PKG::DSU_PORT_READ_BULK, 1);
  next_timing_states[IOSRAM_BTM_PKG::DSU_PORT_READ_BULK].addEvent(
      "dsu_send_" + std::to_string(current_event_number), 1, [this] {
        read_bulk_address_buffer =
            read_bulk_initial_addr +
            current_timing_states[IOSRAM_BTM_PKG::DSU_PORT_READ_BULK]
                .getRepIncrementForCycle(IOSRAM_BTM_PKG::DSU_PORT_READ_BULK);

        DataEvent *dataEvent = new DataEvent(DataEvent::PortType::WriteWide);
        vector<uint8_t> data;
        backend->get(read_bulk_address_buffer, io_data_width / 8, data);
        out.output("Reading bulk data (addr=%d, size=%dbits, data=%s)\n",
                   read_bulk_address_buffer, io_data_width,
                   formatRawDataToWords(data).c_str());
        dataEvent->size = io_data_width;
        dataEvent->payload = data;
        data_links[1]->send(dataEvent);
      });
}

void Iosram_btm::writeBulk() {
  next_timing_states[IOSRAM_BTM_PKG::DSU_PORT_WRITE_BULK].addEvent(
      "dsu_receive_" + std::to_string(current_event_number), 9, [this] {
        write_bulk_address_buffer =
            write_bulk_initial_addr +
            current_timing_states[IOSRAM_BTM_PKG::DSU_PORT_WRITE_BULK]
                .getRepIncrementForCycle(
                    getPortActiveCycle(IOSRAM_BTM_PKG::DSU_PORT_WRITE_BULK));

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
      });
}
