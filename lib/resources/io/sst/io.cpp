#include "io.h"
#include "dataEvent.h"
#include "ioEvents.h"
#include "io_pkg.h"
#include "timingOperators.h"

using namespace SST;

Io::Io(SST::ComponentId_t id, SST::Params &params) : DRRAResource(id, params) {
  instructionHandlers = IO_PKG::createInstructionHandlers(this);
}

bool Io::clockTick(SST::Cycle_t currentCycle) {
  bool result = DRRAResource::clockTick(currentCycle);

  if (portsToActivate.size() > 0 && currentCycle % 10 == 0) {
    for (const auto &port : portsToActivate) {
      activatePortsForSlot(port.first, port.second);
    }
    portsToActivate.clear();
  }

  // Check if input and output ports are active at the same time
  if (isPortActive(IO_PKG::DSU_PORT_INPUT_BUFFER) &&
      isPortActive(IO_PKG::DSU_PORT_OUTPUT_BUFFER)) {
    out.fatal(CALL_INFO, -1,
              "Both DSU ports cannot be active at the same time\n");
  }

  // Output IO data to bulk port
  if (currentCycle % 10 == 2)
    if (isPortActive(IO_PKG::DSU_PORT_INPUT_BUFFER))
      bulkOutput(); // write bulk if input buffer is read from

  // Read IO data from bulk port
  if (currentCycle % 10 == 7)
    if (isPortActive(IO_PKG::DSU_PORT_OUTPUT_BUFFER))
      bulkInput(); // read bulk if output buffer is written to

  return result;
}

void Io::handleActivation(uint32_t slot_id, uint32_t ports) {
  portsToActivate[slot_id] = ports;
}

void Io::handleDSU(const IO_PKG::DSUInstruction &instr) {
  out.output(
      "dsu (slot=%d, port=%d, option=%d, init_addr_sd=%d, init_addr=%d)\n",
      instr.slot, instr.port, instr.option, instr.init_addr_sd,
      instr.init_addr);

  // Set initial address
  agus[instr.port].setInitialAddress(instr.init_addr);
  out.output("Set initial address for port %d to %d\n", instr.port,
             instr.init_addr);

  std::string event_name;
  switch (instr.port) {
  case IO_PKG::DSU_PORT_INPUT_BUFFER:
    event_name =
        "io_dsu_read_from_input_" + std::to_string(current_event_number);
    agus[instr.port].addEvent(
        event_name,
        [this, event_name] {
          updatePortAGUs(IO_PKG::DSU_PORT_INPUT_BUFFER);
          readFromIO();
        },
        1);
    break;
  case IO_PKG::DSU_PORT_OUTPUT_BUFFER:
    event_name =
        "io_dsu_write_to_output_" + std::to_string(current_event_number);
    agus[instr.port].addEvent(
        event_name,
        [this, event_name] {
          updatePortAGUs(IO_PKG::DSU_PORT_OUTPUT_BUFFER);
          writeToIO();
        },
        8);
    break;

  default:
    out.fatal(CALL_INFO, -1, "Invalid DSU mode\n");
  }

  // Add event handler
  current_event_number++;
}

void Io::handleREP(const IO_PKG::REPInstruction &instr) {
  out.output("rep (slot=%d, port=%d, iter=%d, step=%d, delay=%d)\n", instr.slot,
             instr.port, instr.iter, instr.step, instr.delay);

  // add repetition to the timing model
  try {
    agus[instr.port].addRepetition(instr.iter, instr.delay, instr.step);
    out.output("Added repetition to port %d (iter=%d, step=%d)\n", instr.port,
               instr.iter, instr.step);
  } catch (const std::exception &e) {
    out.fatal(CALL_INFO, -1, "Failed to add repetition: %s\n", e.what());
  }
}

void Io::handleREPX(const IO_PKG::REPXInstruction &instr) {
  out.output("repx (slot=%d, port=%d, iter=%d, step=%d, delay=%d)\n",
             instr.slot, instr.port, instr.iter, instr.step, instr.delay);

  auto repetition_op = agus[instr.port].getLastRepetitionOperator();
  uint32_t iter = instr.iter << IO_PKG::IO_INSTR_REPX_ITER_BITWIDTH |
                  repetition_op.getIterations();
  uint32_t step = instr.step << IO_PKG::IO_INSTR_REPX_STEP_BITWIDTH |
                  repetition_op.getStep();
  uint32_t delay = instr.delay << IO_PKG::IO_INSTR_REPX_DELAY_BITWIDTH |
                   repetition_op.getDelay();
  out.output("Adjusting repetition for port %d (iter=%d, step=%d, delay=%d)\n",
             instr.port, iter, step, delay);
  try {
    agus[instr.port].adjustRepetition(iter, delay, step);
  } catch (const std::exception &e) {
    out.fatal(CALL_INFO, -1, "REPX failed: %s\n", e.what());
  }
}

void Io::handleTRANS(const IO_PKG::TRANSInstruction &instr) {
  out.output("trans (slot=%d, port=%d, delay=%d)\n", instr.slot, instr.port,
             instr.delay);

  try {
    agus[instr.port].addTransition(instr.delay);
    current_event_number++;
  } catch (const std::exception &e) {
    out.fatal(CALL_INFO, -1, "Failed to add transition: %s\n", e.what());
  }
}

void Io::readFromIO() {
  read_from_io_address_buffer =
      agus[IO_PKG::DSU_PORT_INPUT_BUFFER].getAddressForCycle(
          getPortActiveCycle(IO_PKG::DSU_PORT_INPUT_BUFFER));

  IOReadRequest *readReq = new IOReadRequest();
  readReq->address = read_from_io_address_buffer;
  readReq->size = io_data_width / 8;
  readReq->column_id = cell_coordinates[1];

  out.output("Sending read request to IO (addr=%d, size=%dbits)\n",
             read_from_io_address_buffer, io_data_width);
  logTraceEvent("io_dsu_read_from_input_", slot_id, true, 'X',
                {{"address", (int)read_from_io_address_buffer},
                 {"size", (int)(io_data_width / 8)}});

  io_input_link->send(readReq);
}

void Io::writeToIO() {
  write_to_io_address_buffer =
      agus[IO_PKG::DSU_PORT_OUTPUT_BUFFER].getAddressForCycle(
          getPortActiveCycle(IO_PKG::DSU_PORT_OUTPUT_BUFFER));

  IOWriteRequest *writeReq = new IOWriteRequest();
  writeReq->address = write_to_io_address_buffer;
  writeReq->data = io_data_buffer;
  io_output_link->send(writeReq);

  out.output("Sending write request to IO (addr=%d, size=%dbits, data=%s)\n",
             writeReq->address, writeReq->data.size() * 8,
             formatRawDataToWords(writeReq->data).c_str());
  logTraceEvent("io_dsu_write_to_output_", slot_id, true, 'X',
                {{"address", (int)write_to_io_address_buffer},
                 {"size", (int)(io_data_buffer.size())},
                 {"data", formatRawDataToWords(io_data_buffer)}});
}

void Io::bulkInput() {
  if (agus[IO_PKG::DSU_PORT_OUTPUT_BUFFER].getAddressForCycle(
          getPortActiveCycle(IO_PKG::DSU_PORT_OUTPUT_BUFFER)) == -1) {
    out.fatal(CALL_INFO, -1,
              "AGU for port %d returned negative address for cycle %d\n",
              IO_PKG::DSU_PORT_OUTPUT_BUFFER,
              getPortActiveCycle(IO_PKG::DSU_PORT_OUTPUT_BUFFER));
  };

  // Receive data from bulk input port
  DataEvent *dataEvent = dynamic_cast<DataEvent *>(data_links[0]->recv());
  if (dataEvent == nullptr)
    out.fatal(CALL_INFO, -1, "No data received on bulk input port\n");

  // Store the data in the buffer
  out.output("Received bulk data (size=%dbits, data=%s)\n", dataEvent->size,
             formatRawDataToWords(dataEvent->payload).c_str());
  io_data_buffer = dataEvent->payload;

  logTraceEvent("io_bulk_input", slot_id, true, 'X',
                {{"data", formatRawDataToWords(io_data_buffer)}});
}

void Io::bulkOutput() {
  if (agus[IO_PKG::DSU_PORT_INPUT_BUFFER].getAddressForCycle(
          getPortActiveCycle(IO_PKG::DSU_PORT_INPUT_BUFFER)) == -1) {
    out.fatal(CALL_INFO, -1,
              "AGU for port %d returned negative address for cycle %d\n",
              IO_PKG::DSU_PORT_INPUT_BUFFER,
              getPortActiveCycle(IO_PKG::DSU_PORT_INPUT_BUFFER));
  };

  // Check response from the input buffer port
  IOReadResponse *readResp =
      dynamic_cast<IOReadResponse *>(io_input_link->recv());
  if (readResp) {
    out.output("Received read response from IO (addr=%d, size=%dbits, "
               "data=%s)\n",
               readResp->address, readResp->data.size() * 8,
               formatRawDataToWords(readResp->data).c_str());
    io_data_buffer = readResp->data;
    if (io_data_buffer.size() == 0) {
      out.fatal(CALL_INFO, -1, "No data received from IO\n");
    }
  } else {
    out.fatal(CALL_INFO, -1, "No response received from IO\n");
  }

  // Send data to output bulk port
  DataEvent *dataEvent = new DataEvent(DataEvent::PortType::WriteWide);
  dataEvent->size = io_data_width;
  dataEvent->payload = io_data_buffer;
  data_links[0]->send(dataEvent);

  logTraceEvent("io_bulk_output", slot_id, true, 'X',
                {{"data", formatRawDataToWords(io_data_buffer)}});
}
