#include "io.h"
#include "dataEvent.h"
#include "ioEvents.h"
#include "io_pkg.h"
#include "timingOperators.h"

using namespace SST;

Io::Io(SST::ComponentId_t id, SST::Params &params) : DRRAResource(id, params) {
  instructionHandlers = IO_PKG::createInstructionHandlers(this);

  // Input path: request from the IO subsystem, then forward the response on
  // the bulk port one subcycle later.
  registerPortAction(IO_PKG::EVT_PORT_INPUT_BUFFER, 3,
                     "io_evt_read_from_input",
                     [this](int64_t address) { readFromIO(address); });
  registerPortAction(IO_PKG::EVT_PORT_INPUT_BUFFER, 4, "io_bulk_output",
                     [this](int64_t address) { bulkOutput(address); });

  // Output path: take the bulk-port payload, then send it to the IO subsystem
  // one subcycle later.
  registerPortAction(IO_PKG::EVT_PORT_OUTPUT_BUFFER, 7, "io_bulk_input",
                     [this](int64_t address) { bulkInput(address); });
  registerPortAction(IO_PKG::EVT_PORT_OUTPUT_BUFFER, 8,
                     "io_evt_write_to_output",
                     [this](int64_t address) { writeToIO(address); });
}

void Io::handleCONF(const IO_PKG::CONFInstruction &instr) {
  out.output("conf (slot=%d)\n", instr.slot);
}

void Io::readFromIO(int64_t address) {
  IOReadRequest *readReq = new IOReadRequest();
  readReq->address = address;
  readReq->size = io_data_width / 8;
  readReq->column_id = cell_coordinates[1];

  out.output("Sending read request to IO (addr=%ld, size=%dbits)\n",
             (long)address, io_data_width);
  logTraceEvent("io_evt_read_from_input_", slot_id, true, 'X',
                {{"address", (int)address},
                 {"size", (int)(io_data_width / 8)}});

  io_input_link->send(readReq);
}

void Io::writeToIO(int64_t address) {
  IOWriteRequest *writeReq = new IOWriteRequest();
  writeReq->address = address;
  writeReq->data = io_output_data_buffer;
  io_output_link->send(writeReq);

  out.output("Sending write request to IO (addr=%d, size=%dbits, data=%s)\n",
             writeReq->address, writeReq->data.size() * 8,
             formatRawDataToWords(writeReq->data).c_str());
  logTraceEvent("io_evt_write_to_output_", slot_id, true, 'X',
                {{"address", (int)address},
                 {"size", (int)(io_output_data_buffer.size())},
                 {"data", formatRawDataToWords(io_output_data_buffer)}});
}

void Io::bulkInput(int64_t address) {
  // Receive data from bulk input port
  DataEvent *dataEvent = dynamic_cast<DataEvent *>(data_links[0]->recv());
  if (dataEvent == nullptr)
    out.fatal(CALL_INFO, -1, "No data received on bulk input port\n");

  // Store the data in the output-path buffer (consumed by writeToIO at the
  // following subcycle). Using a dedicated buffer prevents races with the
  // input-path bulkOutput().
  out.output("Received bulk data (size=%dbits, data=%s)\n", dataEvent->size,
             formatRawDataToWords(dataEvent->payload).c_str());
  io_output_data_buffer = dataEvent->payload;

  logTraceEvent("io_bulk_input", slot_id, true, 'X',
                {{"data", formatRawDataToWords(io_output_data_buffer)}});
}

void Io::bulkOutput(int64_t address) {
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

  // Send data to output bulk port (using the dedicated input-path buffer to
  // avoid clobbering the output path's io_output_data_buffer).
  DataEvent *dataEvent = new DataEvent(DataEvent::PortType::WriteWide);
  dataEvent->size = io_data_width;
  dataEvent->payload = io_input_data_buffer;
  data_links[0]->send(dataEvent);

  logTraceEvent("io_bulk_output", slot_id, true, 'X',
                {{"data", formatRawDataToWords(io_input_data_buffer)}});
}
