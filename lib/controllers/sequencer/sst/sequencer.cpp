#include "sequencer.h"
#include "activationEvent.h"
#include "sequencer_pkg.h"
#include <bitset>
#include <cstdint>

using namespace SST;

namespace {
// Sign-extend the low `bits` of a raw unsigned field into a signed offset. The
// generated instruction structs expose every segment as unsigned, so signed
// segments (e.g. brn branch targets) must be extended before arithmetic.
int32_t sign_extend(uint32_t value, uint32_t bits) {
  uint32_t mask = 1u << (bits - 1);
  return static_cast<int32_t>((value ^ mask) - mask);
}

// Scalar register whose value is broadcast as the loop iteration index with
// every activation. Matches the compiler's depth-0 loop counter (r15).
// TODO: nested loops use distinct counters (r15..r13); a single fixed register
// only covers single-level loops. Revisit when the loop-variable register is
// selected per activation.
constexpr uint32_t LOOP_VAR_REG = 15;
} // namespace

Sequencer::Sequencer(SST::ComponentId_t id, SST::Params &params)
    : DRRAController(id, params) {

  assemblyProgramPath = params.find<std::string>("assembly_program_path");

  // Register params
  registerSize = params.find<int>("register_size", 16);
  numRegisters = params.find<int>("num_registers", 16);

  // Instruction handlers
  instructionHandlers = SEQUENCER_PKG::createInstructionHandlers(this);

  // Register as primary component
  registerAsPrimaryComponent();
  primaryComponentDoNotEndSim();
}

void Sequencer::init(unsigned int phase) {
  // Load the assembly program
  load_assembly_program(assemblyProgramPath);

  // Initialize scalar and bool registers
  for (uint32_t i = 0; i < numRegisters; i++) {
    scalarRegisters.push_back(0);
  }
  out.output("Initialized %u scalar registers\n", numRegisters);

  // End of initialization
  out.verbose(CALL_INFO, 1, 0, "Initialized\n");
}

bool Sequencer::clockTick(SST::Cycle_t currentSSTCycle) {
  if (currentSSTCycle % 10 == 0) {
    if (cyclesToWait > 0) {
      cyclesToWait--;
      out.output("Waiting %u cycles\n", cyclesToWait);
      return false;
    }

    if (pc >= assemblyProgram.size()) {
      out.fatal(CALL_INFO, -1, "Program counter out of bounds\n");
    }
    uint32_t instruction = assemblyProgram[pc];
    Instruction instrObj(instruction, format);
    logTraceEvent("instruction", 0, false, 'X',
                  {{"pc", static_cast<int>(pc)},
                   {"instruction", instrObj.toString()},
                   {"instruction_bin", instrObj.toBinaryString()},
                   {"instruction_hex", instrObj.toHexString()}});
    // A branch handler computes the next PC itself (relative to this
    // instruction's own PC); suppress the default increment in that case so
    // the target is not shifted by one.
    branchTaken = false;
    decodeInstr(instruction);
    if (!branchTaken) {
      pc++;
    }

    if (readyToFinish) {
      primaryComponentOKToEndSim();
      return true;
    }
  }
  return false;
}

void Sequencer::load_assembly_program(std::string assemblyProgramPath) {
  // Load the assembly program
  if (assemblyProgramPath.empty()) {
    out.fatal(CALL_INFO, -1, "No assembly program provided\n");
  }
  std::ifstream assemblyProgramFile(assemblyProgramPath);
  if (!assemblyProgramFile.is_open()) {
    out.fatal(CALL_INFO, -1, "Failed to open assembly program file\n");
  }

  std::string line;
  bool isSelfCell = false;
  while (std::getline(assemblyProgramFile, line)) {
    // out.output("Read line: %s\n", line.c_str());
    if (line.find("cell") != std::string::npos) {
      out.output("Found cell section in line: %s\n", line.c_str());
      if (line.find("cell " + std::to_string(cell_coordinates[0]) + " " +
                    std::to_string(cell_coordinates[1])) != std::string::npos) {
        isSelfCell = true;
      } else {
        isSelfCell = false;
      }
      continue;
    } else if (isSelfCell) {
      out.output("Adding instruction: %s\n", line.c_str());
      std::bitset<32> bits(line);
      assemblyProgram.push_back(static_cast<uint32_t>(bits.to_ulong()));
    }
  }
  assemblyProgram.shrink_to_fit(); // Ensure proper alignment and size
  out.output("Loaded %lu instructions\n", assemblyProgram.size());
}

void Sequencer::handleHALT(const SEQUENCER_PKG::HALTInstruction &instr) {
  out.output("halt (slot=%d, )\n", instr.slot);
  logTraceEvent("halt", 0, false, 'X', {{"pc", static_cast<int>(pc)}});
  readyToFinish = true;
}

void Sequencer::handleWAIT(const SEQUENCER_PKG::WAITInstruction &instr) {
  out.output("wait (slot=%d, mode=%d, cycle=%d)\n", instr.slot, instr.mode,
             instr.cycle);

  logTraceEvent("wait", 0, false, 'X',
                {{"pc", static_cast<int>(pc)},
                 {"wait_mode", static_cast<int>(instr.mode)},
                 {"wait_cycle", static_cast<int>(instr.cycle)}});

  if (instr.mode == 0) {
    cyclesToWait = instr.cycle;
  } else {
    out.fatal(CALL_INFO, -1, "Event-based wait not implemented\n");
  }
}

void Sequencer::handleACT(const SEQUENCER_PKG::ACTInstruction &instr) {
  out.output("act (slot=%d, ports=%d, mode=%d, param=%d)\n", instr.slot,
             instr.ports, instr.mode, instr.param);

  // Sample the loop-variable register once; every act event this instruction
  // broadcasts carries it (0 when not inside a loop, so no address offset).
  currentLoopVar =
      LOOP_VAR_REG < scalarRegisters.size() ? scalarRegisters[LOOP_VAR_REG] : 0;

  logTraceEvent("activation", 0, false, 'X',
                {{"pc", static_cast<int>(pc)},
                 {"action_mode", static_cast<int>(instr.mode)},
                 {"action_ports", static_cast<int>(instr.ports)},
                 {"action_param", static_cast<int>(instr.param)},
                 {"loop_var", static_cast<int>(currentLoopVar)}});

  switch (instr.mode) {
  case SEQUENCER_PKG::ACT_MODE_CONTIGUOUS:
    handle_continuous_port_mode(instr.ports, instr.param);
    break;

  case SEQUENCER_PKG::ACT_MODE_PORT_INDEX:
    handle_all_port_X_mode(instr.ports, instr.param);
    break;

  case SEQUENCER_PKG::ACT_MODE_MAP:
    handle_activation_vector_mode(instr.ports, instr.param);
    break;

  default: {
    out.fatal(CALL_INFO, -1, "Invalid activation mode: %d\n", instr.mode);
    break;
  }
  }
}

void Sequencer::handleCALC(const SEQUENCER_PKG::CALCInstruction &instr) {
  out.output("calc (slot=%d, mode=%d, operand1=%d, operand2_sd=%d, "
             "operand2=%d, result=%d)\n",
             instr.slot, instr.mode, instr.operand1, instr.operand2_sd,
             instr.operand2, instr.result);

  logTraceEvent("calculation", 0, false, 'X',
                {{"pc", static_cast<int>(pc)},
                 {"calc_mode", static_cast<int>(instr.mode)},
                 {"calc_operand1", static_cast<int>(instr.operand1)},
                 {"calc_operand2_sd", static_cast<int>(instr.operand2_sd)},
                 {"calc_operand2", static_cast<int>(instr.operand2)},
                 {"calc_result", static_cast<int>(instr.result)}});

  uint32_t operand1_tmp = 0;
  operand1_tmp = scalarRegisters[instr.operand1];
  uint32_t operand2_tmp = 0;
  if (instr.operand2_sd)
    operand2_tmp = scalarRegisters[instr.operand2];
  else
    operand2_tmp = instr.operand2;

  std::string operationStr;
  switch (instr.mode) {
  case 0:
    operationStr = "idle";
    break;
  case 1:
    operationStr = "add";
    scalarRegisters[instr.result] = operand1_tmp + operand2_tmp;
    break;
  case 2:
    operationStr = "sub";
    scalarRegisters[instr.result] = operand1_tmp - operand2_tmp;
    break;
  case 3:
    operationStr = "lls";
    scalarRegisters[instr.result] = operand1_tmp << operand2_tmp;
    break;
  case 4:
    operationStr = "lrs";
    scalarRegisters[instr.result] = operand1_tmp >> operand2_tmp;
    break;
  case 5:
    operationStr = "mul";
    scalarRegisters[instr.result] = operand1_tmp * operand2_tmp;
    break;
  case 6:
    operationStr = "div";
    scalarRegisters[instr.result] = operand1_tmp / operand2_tmp;
    break;
  case 7:
    operationStr = "mod";
    scalarRegisters[instr.result] = operand1_tmp % operand2_tmp;
    break;
  case 8:
    operationStr = "bitand";
    scalarRegisters[instr.result] = operand1_tmp & operand2_tmp;
    break;
  case 9:
    operationStr = "bitor";
    scalarRegisters[instr.result] = operand1_tmp | operand2_tmp;
    break;
  case 10:
    operationStr = "bitinv";
    scalarRegisters[instr.result] = ~operand1_tmp;
    break;
  case 11:
    operationStr = "bitxor";
    scalarRegisters[instr.result] = operand1_tmp ^ operand2_tmp;
    break;
  case 17:
    operationStr = "eq";
    scalarRegisters[instr.result] = (operand1_tmp == operand2_tmp);
    break;
  case 18:
    operationStr = "ne";
    scalarRegisters[instr.result] = (operand1_tmp != operand2_tmp);
    break;
  case 19:
    operationStr = "gt";
    scalarRegisters[instr.result] = (operand1_tmp > operand2_tmp);
    break;
  case 20:
    operationStr = "ge";
    scalarRegisters[instr.result] = (operand1_tmp >= operand2_tmp);
    break;
  case 21:
    operationStr = "lt";
    scalarRegisters[instr.result] = (operand1_tmp < operand2_tmp);
    break;
  case 22:
    operationStr = "le";
    scalarRegisters[instr.result] = (operand1_tmp <= operand2_tmp);
    break;
  case 23:
    operationStr = "addh";
    scalarRegisters[instr.result] =
        (operand1_tmp & 0xFF) + ((operand2_tmp & 0xFF) << 8);
    break;
  case 32:
    operationStr = "and";
    scalarRegisters[instr.result] = operand1_tmp && operand2_tmp;
    break;
  case 33:
    operationStr = "or";
    scalarRegisters[instr.result] = operand1_tmp || operand2_tmp;
    break;
  case 34:
    operationStr = "not";
    scalarRegisters[instr.result] = !operand1_tmp;
    break;

  default:
    out.fatal(CALL_INFO, -1, "Invalid operation mode: %d\n", instr.mode);
    break;
  }
}

void Sequencer::handleBRN(const SEQUENCER_PKG::BRNInstruction &instr) {
  // Branch targets are signed offsets (ISA is_signed) relative to this
  // instruction's own PC, but the generated struct exposes them as raw
  // unsigned bit fields; sign-extend before use so backward branches work.
  int32_t target_true = sign_extend(
      instr.target_true,
      SEQUENCER_PKG::SEQUENCER_INSTR_BRN_TARGET_TRUE_BITWIDTH);
  int32_t target_false = sign_extend(
      instr.target_false,
      SEQUENCER_PKG::SEQUENCER_INSTR_BRN_TARGET_FALSE_BITWIDTH);

  out.output("brn (slot=%d, reg=%d, target_true=%d, target_false=%d)\n",
             instr.slot, instr.reg, target_true, target_false);

  logTraceEvent("branch", 0, false, 'X',
                {{"pc", static_cast<int>(pc)},
                 {"branch_reg", static_cast<int>(instr.reg)},
                 {"branch_target_true", static_cast<int>(target_true)},
                 {"branch_target_false", static_cast<int>(target_false)}});

  // New PC is relative to the branch's own PC. branchTaken suppresses the
  // fetch loop's default increment so the offset is not shifted by one.
  int32_t offset = scalarRegisters[instr.reg] ? target_true : target_false;
  pc = static_cast<uint32_t>(static_cast<int32_t>(pc) + offset);
  branchTaken = true;
}

void Sequencer::handle_continuous_port_mode(uint32_t ports, uint32_t param) {
  uint32_t current_slot = param;
  uint32_t target_ports_for_slot = 0;
  uint32_t temp_ports = ports;
  uint32_t ports_segment_length = 16;

  for (uint32_t i = 0; i < ports_segment_length; i += PORTS_PER_SLOT) {
    // Extract 1-bit activation for each port and accumulate
    uint32_t maskAllPorts = ((1 << (PORTS_PER_SLOT + 1)) - 1);
    target_ports_for_slot = temp_ports & maskAllPorts;
    temp_ports >>= PORTS_PER_SLOT;

    // Skip this slot if no ports are activated or not linked
    if ((target_ports_for_slot == 0) || (slot_links[current_slot] == nullptr)) {
      current_slot++;
      continue;
    }

    sendActEvent(current_slot, target_ports_for_slot);

    // Next slot
    target_ports_for_slot = 0;
    current_slot++;
  }
}

void Sequencer::handle_all_port_X_mode(uint32_t ports, uint32_t param) {
  uint32_t target_ports_for_slot = 0;
  uint32_t ports_segment_length = 16;
  if (param >= PORTS_PER_SLOT) {
    out.fatal(CALL_INFO, -1,
              "Invalid port number, activation mode 1 requires a value "
              "between 0 and %d.\n",
              PORTS_PER_SLOT - 1);
  }
  target_ports_for_slot = param;
  for (uint32_t i = 0; i < ports_segment_length; i++) {
    if (ports & (1 << i)) {
      sendActEvent(i, target_ports_for_slot);
    }
  }
}

void Sequencer::handle_activation_vector_mode(uint32_t ports, uint32_t param) {
  uint32_t num_bits_needed = num_slots * PORTS_PER_SLOT;
  uint32_t num_registers_needed = num_bits_needed / registerSize;

  if (param == 0) {
    out.fatal(CALL_INFO, -1, "Register 0 is reserved.");
  }

  if (param > num_slots - num_registers_needed) {
    out.fatal(CALL_INFO, -1,
              "Invalid value for param (%d), %d consecutive registers are "
              "needed and the sequencer only has %d registers (r0 to r%d).",
              param, num_registers_needed, num_slots, num_slots - 1);
  }

  uint64_t activationVector = 0;
  for (uint32_t i = 0; i < num_registers_needed; i++) {
    if (param + i >= scalarRegisters.size()) {
      out.fatal(CALL_INFO, -1,
                "Invalid register index %d, only %d registers available.",
                param + i, scalarRegisters.size());
    }
    uint64_t regValue = scalarRegisters[param + i];
    activationVector += (regValue << (i * registerSize));
  }
  out.output("Using activation vector from register %d: %s\n", param,
             std::bitset<64>(activationVector).to_string().c_str());

  uint64_t activationMask = activationVector;
  uint64_t slotActMask = 0;
  uint64_t currentMask =
      (1 << PORTS_PER_SLOT) - 1; // Mask for PORTS_PER_SLOT bits
  for (int currentSlot = 0; currentSlot < num_slots; currentSlot++) {
    slotActMask = activationMask & currentMask;
    if (slot_links[currentSlot] != nullptr) {
      if (slotActMask != 0)
        sendActEvent(currentSlot, slotActMask);
    } else {
      if (activationMask != 0) {
        out.fatal(CALL_INFO, -1,
                  "Trying to activate slot %d, but it is not linked.\n",
                  currentSlot);
      }
    }
    activationMask >>= PORTS_PER_SLOT; // Shift to the next slot
  }
}

void Sequencer::sendActEvent(uint32_t slot_id, uint32_t ports_mask) {
  out.output("Sending ACT event to slot %u with ports mask %s\n", slot_id,
             std::bitset<PORTS_PER_SLOT>(ports_mask).to_string().c_str());
  ActEvent *event = new ActEvent();
  event->slot_id = slot_id;
  event->ports = ports_mask;
  event->loop_var = currentLoopVar;
  if (slot_links[slot_id]->isConfigured())
    slot_links[slot_id]->send(event);
  else
    delete event;
}
