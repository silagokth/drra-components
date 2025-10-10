#include "bcpnn.h"

using namespace SST;

Bcpnn::Bcpnn(SST::ComponentId_t id, SST::Params &params)
    : DRRAResource(id, params) {
      instructionHandlers = BCPNN_PKG::createInstructionHandlers(this);
}

bool Bcpnn::clockTick(SST::Cycle_t currentCycle) {
  return DRRAResource::clockTick(currentCycle);
}

void Bcpnn::handleDSU(const BCPNN_PKG::DSUInstruction &instr) {
  out.output(
    "dsu (slot=%d, init_addr_sd=%d, init_addr=%d, port=%d)\n",
    instr.slot, instr.init_addr_sd, instr.init_addr, instr.port);

  // TODO: implement instruction behavior
  auto dsu = instr;

  port_agus_init[dsu.port] = dsu.init_addr;

  // Add the event handler
  switch (dsu.port) {
  case DataEvent::PortType::ReadNarrow:
    next_timing_states[dsu.port].addEvent(
        "dsu_read_narrow_" + std::to_string(current_event_number), 1, [this] {
          updatePortAGUs(DataEvent::PortType::ReadNarrow);
          readNarrow();
        });
    break;
  case DataEvent::PortType::ReadWide:
    next_timing_states[dsu.port].addEvent(
        "dsu_read_wide_" + std::to_string(current_event_number), 1, [this] {
          updatePortAGUs(DataEvent::PortType::ReadWide);
          readWide();
        });
    break;
  case DataEvent::PortType::WriteNarrow:
    next_timing_states[dsu.port].addEvent(
        "dsu_write_narrow_" + std::to_string(current_event_number), 9, [this] {
          updatePortAGUs(DataEvent::PortType::WriteNarrow);
          writeNarrow();
        });
    break;
  case DataEvent::PortType::WriteWide:
    next_timing_states[dsu.port].addEvent(
        "dsu_write_wide_" + std::to_string(current_event_number), 9, [this] {
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

void Bcpnn::handleREP(const BCPNN_PKG::REPInstruction &instr) {
  out.output(
    "rep (slot=%d, port=%d, level=%d, iter=%d, step=%d, delay=%d)\n",
    instr.slot, instr.port, instr.level, instr.iter, instr.step, instr.delay);

  // TODO: implement instruction behavior
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

void Bcpnn::handleREPX(const BCPNN_PKG::REPXInstruction &instr) {
  out.output(
    "repx (slot=%d, port=%d, level=%d, iter=%d, step=%d, delay=%d)\n",
    instr.slot, instr.port, instr.level, instr.iter, instr.step, instr.delay);

  // TODO: implement instruction behavior
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


