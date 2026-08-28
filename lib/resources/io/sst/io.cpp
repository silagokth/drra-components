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

  // Latch any value the SWB delivered into the held bulk input wire before
  // bulkInput() (sub-7) samples it.
  receiveDataInputs();

  if (portsToActivate.size() > 0 && currentCycle % 10 == 0) {
    for (const auto &port : portsToActivate) {
      activatePortsForSlot(port.first, port.second);
    }
    portsToActivate.clear();
  }


  // Gate bulkOutput / bulkInput on the EVT port AGU actually having an event
  // scheduled at the current active cycle, not just the port being "active".
  // isPortActive stays true throughout the port's whole lifetime (until the
  // AGU is exhausted), including gaps between outer rep iterations; firing
  // bulkOutput / bulkInput in those gap cycles would try to recv() data that
  // was never requested/sent and was the source of spurious fatal errors.
  if (currentCycle % 10 == 2)
    if (isPortActive(IO_PKG::EVT_PORT_INPUT_BUFFER) &&
        agus[IO_PKG::EVT_PORT_INPUT_BUFFER].getAddressForCycle(
            getPortActiveCycle(IO_PKG::EVT_PORT_INPUT_BUFFER)) != -1)
      bulkOutput();

  if (currentCycle % 10 == 7)
    if (isPortActive(IO_PKG::EVT_PORT_OUTPUT_BUFFER) &&
        agus[IO_PKG::EVT_PORT_OUTPUT_BUFFER].getAddressForCycle(
            getPortActiveCycle(IO_PKG::EVT_PORT_OUTPUT_BUFFER)) != -1)
      bulkInput();

  return result;
}

void Io::handleActivation(uint32_t slot_id, uint32_t ports) {
  portsToActivate[slot_id] = ports;
}

void Io::handleEVT(const IO_PKG::EVTInstruction &instr) {
  out.output(
      "evt (slot=%d, port=%d, option=%d, init_addr_sd=%d, init_addr=%d)\n",
      instr.slot, instr.port, instr.option, instr.init_addr_sd,
      instr.init_addr);

  std::string event_name;
  switch (instr.port) {
  case IO_PKG::EVT_PORT_INPUT_BUFFER:
    event_name =
        "io_evt_read_from_input_" + std::to_string(current_event_number);
    agus[instr.port].addEvent(
        event_name,
        [this, event_name] {
          updatePortAGUs(IO_PKG::EVT_PORT_INPUT_BUFFER);
          readFromIO();
        },
        1, instr.init_addr);
    break;
  case IO_PKG::EVT_PORT_OUTPUT_BUFFER:
    event_name =
        "io_evt_write_to_output_" + std::to_string(current_event_number);
    agus[instr.port].addEvent(
        event_name,
        [this, event_name] {
          updatePortAGUs(IO_PKG::EVT_PORT_OUTPUT_BUFFER);
          writeToIO();
        },
        8, instr.init_addr);
    break;

  default:
    out.fatal(CALL_INFO, -1, "Invalid EVT mode\n");
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
      agus[IO_PKG::EVT_PORT_INPUT_BUFFER].getAddressForCycle(
          getPortActiveCycle(IO_PKG::EVT_PORT_INPUT_BUFFER));

  IOReadRequest *readReq = new IOReadRequest();
  readReq->address = read_from_io_address_buffer;
  readReq->size = io_data_width / 8;
  readReq->column_id = cell_coordinates[1];

  out.output("Sending read request to IO (addr=%d, size=%dbits)\n",
             read_from_io_address_buffer, io_data_width);
  logTraceEvent("io_read_from_input", slot_id, true, 'X',
                {{"address", (int)read_from_io_address_buffer},
                 {"size", (int)(io_data_width / 8)}});

  io_input_link->send(readReq);
}

void Io::writeToIO() {
  write_to_io_address_buffer =
      agus[IO_PKG::EVT_PORT_OUTPUT_BUFFER].getAddressForCycle(
          getPortActiveCycle(IO_PKG::EVT_PORT_OUTPUT_BUFFER));

  IOWriteRequest *writeReq = new IOWriteRequest();
  writeReq->address = write_to_io_address_buffer;
  writeReq->data = io_output_data_buffer;
  io_output_link->send(writeReq);

  out.output("Sending write request to IO (addr=%d, size=%dbits, data=%s)\n",
             writeReq->address, writeReq->data.size() * 8,
             formatRawDataToWords(writeReq->data).c_str());
  logTraceEvent("io_write_to_output", slot_id, true, 'X',
                {{"address", (int)write_to_io_address_buffer},
                 {"size", (int)(io_output_data_buffer.size())},
                 {"data", formatRawDataToWords(io_output_data_buffer)}});
}

void Io::bulkInput() {
  if (agus[IO_PKG::EVT_PORT_OUTPUT_BUFFER].getAddressForCycle(
          getPortActiveCycle(IO_PKG::EVT_PORT_OUTPUT_BUFFER)) == -1) {
    out.fatal(CALL_INFO, -1,
              "AGU for port %d returned negative address for cycle %d\n",
              IO_PKG::EVT_PORT_OUTPUT_BUFFER,
              getPortActiveCycle(IO_PKG::EVT_PORT_OUTPUT_BUFFER));
  };

  // Sample the held bulk input wire (latched by receiveDataInputs()). The held
  // register naturally holds the value that's on the wire this cycle, replacing
  // the old drain-the-FIFO-for-freshest-payload hack.
  io_output_data_buffer = readInput(0, PortChannel::BULK).data;

  out.output("Received bulk data (data=%s)\n",
             formatRawDataToWords(io_output_data_buffer).c_str());

  logTraceEvent("io_bulk_input", slot_id, true, 'X',
                {{"data", formatRawDataToWords(io_output_data_buffer)}});
}

void Io::bulkOutput() {
  if (agus[IO_PKG::EVT_PORT_INPUT_BUFFER].getAddressForCycle(
          getPortActiveCycle(IO_PKG::EVT_PORT_INPUT_BUFFER)) == -1) {
    out.fatal(CALL_INFO, -1,
              "AGU for port %d returned negative address for cycle %d\n",
              IO_PKG::EVT_PORT_INPUT_BUFFER,
              getPortActiveCycle(IO_PKG::EVT_PORT_INPUT_BUFFER));
  };

  // Check response from the input buffer port
  IOReadResponse *readResp =
      dynamic_cast<IOReadResponse *>(io_input_link->recv());
  if (readResp) {
    out.output("Received read response from IO (addr=%d, size=%dbits, "
               "data=%s)\n",
               readResp->address, readResp->data.size() * 8,
               formatRawDataToWords(readResp->data).c_str());
    io_input_data_buffer = readResp->data;
    if (io_input_data_buffer.size() == 0) {
      out.fatal(CALL_INFO, -1, "No data received from IO\n");
    }
  } else {
    out.fatal(CALL_INFO, -1, "No response received from IO\n");
  }

  // Combinational passthrough of the IO read data onto the bulk wire
  // (io.sv.j2: bulk_data_out_0 = io_data_in).
  driveOutput(0, PortChannel::BULK, io_input_data_buffer, io_data_width,
              /*registered=*/false);

  logTraceEvent("io_bulk_output", slot_id, true, 'X',
                {{"data", formatRawDataToWords(io_input_data_buffer)}});
}
