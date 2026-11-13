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

  // Word path: request the row, then pass one word of it to the word port.
  registerPortAction(IO_PKG::EVT_PORT_INPUT_WORD, 3, "io_evt_read_word",
                     [this](int64_t address) {
                       readFromIO(address / wordsPerRow());
                     });
  registerPortAction(IO_PKG::EVT_PORT_INPUT_WORD, 4, "io_word_output",
                     [this](int64_t address) { wordOutput(address); });

  // Output path: take the bulk-port payload, then send it to the IO subsystem
  // one subcycle later.
  registerPortAction(IO_PKG::EVT_PORT_OUTPUT_BUFFER, 7, "io_bulk_input",
                     [this](int64_t address) { bulkInput(address); });
  registerPortAction(IO_PKG::EVT_PORT_OUTPUT_BUFFER, 8,
                     "io_evt_write_to_output",
                     [this](int64_t address) { writeToIO(address); });
}

void Io::handleCONF(const IO_PKG::CONFInstruction &instr) {
  out.output("conf (slot=%d, wr_mask=0x%x)\n", instr.slot, instr.wr_mask);
  wr_mask = instr.wr_mask;
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
  writeReq->wr_mask = wr_mask;
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
  delete dataEvent;

  logTraceEvent("io_bulk_input", slot_id, true, 'X',
                {{"data", formatRawDataToWords(io_output_data_buffer)}});
}

void Io::wordOutput(int64_t address) {
  IOReadResponse *readResp =
      dynamic_cast<IOReadResponse *>(io_input_link->recv());
  if (readResp == nullptr)
    out.fatal(CALL_INFO, -1, "No response received from IO\n");
  const size_t bytes = word_bitwidth / 8;
  const size_t at = (address % wordsPerRow()) * bytes;
  if (readResp->data.size() < at + bytes)
    out.fatal(CALL_INFO, -1, "No data received from IO\n");
  std::vector<uint8_t> word(readResp->data.begin() + at,
                            readResp->data.begin() + at + bytes);
  delete readResp;

  DataEvent *dataEvent = new DataEvent(DataEvent::PortType::WriteNarrow);
  dataEvent->size = word_bitwidth;
  dataEvent->payload = word;
  data_links[0]->send(dataEvent);

  out.output("Word output (addr=%ld, data=%s)\n", (long)address,
             formatRawDataToWords(word).c_str());
  logTraceEvent("io_word_output", slot_id, true, 'X',
                {{"address", (int)address},
                 {"data", formatRawDataToWords(word)}});
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
    delete readResp;
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
