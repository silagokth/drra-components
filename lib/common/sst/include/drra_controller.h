
#include "drra_component.h"
#include "timingModel.h"

#include <cmath>

#include <sst/core/link.h>
#include <sst/core/params.h>

using namespace SST;

class DRRAController : public DRRAComponent {
public:
  DRRAController(ComponentId_t id, Params &params) : DRRAComponent(id, params) {
    // Configure output
    out.setPrefix(getType() + " [" + std::to_string(cell_coordinates[0]) + "_" +
                  std::to_string(cell_coordinates[1]) + "] - ");

    // Configure slot links
    uint8_t num_connected_links = 0;
    for (uint32_t i = 0; i < num_slots; i++) {
      if (isPortConnected("slot_port" + std::to_string(i))) {
        slot_links.push_back(configureLink("slot_port" + std::to_string(i)));
        num_connected_links++;
      } else {
        slot_links.push_back(nullptr);
      }
    }
    sst_assert(slot_links.size() == num_slots, CALL_INFO, -1,
               "Slot links size mismatch");
    if (num_connected_links > 0) {
      out.output("Connected %lu slot links (", num_connected_links);
      for (uint32_t i = 0; i < num_slots; i++) {
        if (slot_links[i] != nullptr) {
          out.print("%u,", i);
        }
      }
      out.print(")\n");
    }
  }

  virtual ~DRRAController() {
    // Open trace file to modify the last line
    if (cell_coordinates[0] == 0 && cell_coordinates[1] == 0) {
      trace_file.open(trace_name, std::ios::app);
      // Find the last comma and remove it
      std::ifstream trace_file_read(trace_name);
      std::string line;
      std::string last_line;
      while (std::getline(trace_file_read, line)) {
        last_line = line;
      }
      trace_file_read.close();
      size_t last_comma = last_line.find_last_of(',');
      if (last_comma != std::string::npos) {
        last_line = last_line.substr(0, last_comma);
      }
      trace_file << last_line << std::endl;
      // Add the closing bracket
      trace_file << "], \"displayTimeUnit\": \"ns\"}" << std::endl;
      trace_file.flush();
      trace_file.close();
      // Move the trace file to the output directory
      std::string command = "mv " + trace_name + " trace_complete.json";
      int returnCode = system(command.c_str());
      if (returnCode != 0) {
        out.output(
            "WARN: Could not copy the trace file to trace_complete.json");
      }
    }
  }

  static std::vector<SST::ElementInfoParam> getBaseParams() {
    std::vector<SST::ElementInfoParam> params = DRRAComponent::getBaseParams();
    return params;
  }

  static std::vector<SST::ElementInfoPort> getBasePorts() {
    std::vector<SST::ElementInfoPort> ports;
    ports.push_back({"slot_port%(portnum)d",
                     "Link(s) to resources in slots. Connect slot_port0, "
                     "slot_port1, etc."});
    return ports;
  }

protected:
  // Program counter
  uint32_t pc = 0;

  // Slot links
  std::vector<Link *> slot_links;
};
