#include <cstdint>
#include <fstream>
#include <sst/core/component.h>
#include <sst/core/link.h>

#include "activationEvent.h"
#include "drra.h"
#include "instructionEvent.h"
#include "sequencer.h"
#include "sst/core/output.h"
#include <bitset>

Sequencer::Sequencer(ComponentId_t id, Params &params)
    : DRRAController(id, params) {
  assemblyProgramPath = params.find<std::string>("assembly_program_path");

  // Register params
  registerSize = params.find<int>("register_size", 16);
  numRegisters = params.find<int>("num_registers", 16);

  // Register as primary component
  registerAsPrimaryComponent();
  primaryComponentDoNotEndSim();
}

Sequencer::~Sequencer() {}

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

void Sequencer::setup() {}

void Sequencer::complete(unsigned int phase) {}

void Sequencer::finish() { out.verbose(CALL_INFO, 1, 0, "Finishing\n"); }

bool Sequencer::clockTick(Cycle_t currentCycle) {
  if (currentCycle % 10 == 0) {
    if (cyclesToWait > 0) {
      cyclesToWait--;
      out.output("Waiting %u cycles (next instruction: %s)\n", cyclesToWait,
                 bitset<32>(assemblyProgram[pc]).to_string().c_str());
      return false;
    }

    if (pc >= assemblyProgram.size()) {
      out.fatal(CALL_INFO, -1, "Program counter out of bounds\n");
    }
    uint32_t instruction = assemblyProgram[pc];
    fetch_decode(instruction);

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

void Sequencer::fetch_decode(uint32_t instruction) {
  // Decode instruction
  // TODO make dependent on ISA.json
  if (instrBitwidth != 32) {
    out.fatal(CALL_INFO, -1,
              "Invalid instruction bitwidth. Only 32-bit "
              "is supported for now.\n");
  }
  uint32_t instruction_type = getInstrType(instruction);
  uint32_t opcode = getInstrOpcode(instruction);
  uint32_t slot = getInstrSlot(instruction);

  if (instruction_type == 1) // Send event to resource
  {
    InstrEvent *event = new InstrEvent();
    event->instruction = instruction;
    out.output("Sending INSTRUCTION event to slot %u\n", slot);
    if (!slot_links[slot]->isConfigured()) {
      out.fatal(CALL_INFO, -1, "Slot link %u not configured\n", slot);
    }
    slot_links[slot]->send(event);
  } else {
    switch (opcode) {
    case 0: // HALT
      halt();
      break;

    case 1: // WAIT
      wait(instruction);
      break;

    case 2: // ACT
      activate(instruction);
      break;

    case 3: // CALC
      calculate(instruction);
      break;

    case 4: // BRN
      branch(instruction);
      break;

    default:
      break;
    }
  }

  // Increment program counter
  pc++;
}

void Sequencer::halt() {
  out.output("HALT\n");
  readyToFinish = true;
}

void Sequencer::wait(uint32_t instr) {
  out.output("WAIT\n");
  uint32_t mode_segment_length = 1;
  uint32_t cycleSegmentLength = 27;

  // Check validity
  if (mode_segment_length + cycleSegmentLength >
      instrBitwidth - instrTypeBitwidth - instrOpcodeWidth) {
    out.fatal(CALL_INFO, -1, "Invalid instruction format\n");
  }

  // Extract segments
  uint32_t mode = (instr & ((1 << mode_segment_length) - 1)
                               << (instrBitwidth - instrTypeBitwidth -
                                   instrOpcodeWidth - mode_segment_length)) >>
                  (instrBitwidth - instrTypeBitwidth - instrOpcodeWidth -
                   mode_segment_length);
  uint32_t cycles = (instr & ((1 << cycleSegmentLength) - 1));

  if (mode == 0) {
    wait_cycles(cycles);
  } else {
    wait_event();
  }

  out.output("WAIT: mode=%s, cycles=%u\n",
             std::bitset<1>(mode).to_string().c_str(), cycles);
}

void Sequencer::wait_cycles(uint32_t cycles) { cyclesToWait = cycles; }

void Sequencer::wait_event() { out.output("WAIT EVENT\n"); }

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

void Sequencer::activate(uint32_t instr) {
  uint32_t ports_segment_length = 16;
  uint32_t mode_segment_length = 4;
  uint32_t param_segment_length = 8;
  uint32_t ports = getInstrField(instr, ports_segment_length, 12);
  uint32_t mode = getInstrField(instr, mode_segment_length, 8);
  uint32_t param = getInstrField(instr, param_segment_length, 0);

  out.output("act (mode=%d, param=%d, ports=%s)\n", mode, param,
             std::bitset<16>(ports).to_string().c_str());

  switch (mode) {
  case 0: // Continuous ports starting from slot X (param)
    handle_continuous_port_mode(ports, param);
    break;

  case 1: // All port X (param) in each slot
    handle_all_port_X_mode(ports, param);
    break;

  case 2: // Use activation vector stored in a scalar register
    handle_activation_vector_mode(ports, param);
    break;

  default: {
    out.fatal(CALL_INFO, -1, "ACT mode %d not implemented\n", mode);
    break;
  }
  }
}

void Sequencer::sendActEvent(uint32_t slot_id, uint32_t ports_mask) {
  out.output("Sending ACT event to slot %u with ports mask %s\n", slot_id,
             std::bitset<PORTS_PER_SLOT>(ports_mask).to_string().c_str());
  ActEvent *event = new ActEvent();
  event->slot_id = slot_id;
  event->ports = ports_mask;
  if (slot_links[slot_id]->isConfigured())
    slot_links[slot_id]->send(event);
  else
    delete event;
}

void Sequencer::calculate(uint32_t instr) {
  uint32_t mode_segment_length = 6;
  uint32_t operand1SegmentLength = 4;
  uint32_t operand2SDSegmentLength = 1;
  uint32_t operand2SegmentLength = 8;
  uint32_t resultSegmentLength = 4;

  // Check validity
  if (mode_segment_length + operand1SegmentLength + operand2SDSegmentLength +
          operand2SegmentLength + resultSegmentLength >
      instrBitwidth - instrTypeBitwidth - instrOpcodeWidth) {
    out.fatal(CALL_INFO, -1, "Invalid instruction format\n");
  }

  // Extract segments
  uint32_t mode = getInstrField(instr, mode_segment_length,
                                instrBitwidth - instrTypeBitwidth -
                                    instrOpcodeWidth - mode_segment_length);
  uint32_t operand1 =
      getInstrField(instr, operand1SegmentLength,
                    instrBitwidth - instrTypeBitwidth - instrOpcodeWidth -
                        mode_segment_length - operand1SegmentLength);
  uint32_t operand2SD =
      getInstrField(instr, operand2SDSegmentLength,
                    instrBitwidth - instrTypeBitwidth - instrOpcodeWidth -
                        mode_segment_length - operand1SegmentLength -
                        operand2SDSegmentLength);
  uint32_t operand2 =
      getInstrField(instr, operand2SegmentLength,
                    instrBitwidth - instrTypeBitwidth - instrOpcodeWidth -
                        mode_segment_length - operand1SegmentLength -
                        operand2SDSegmentLength - operand2SegmentLength);
  uint32_t result =
      getInstrField(instr, resultSegmentLength,
                    instrBitwidth - instrTypeBitwidth - instrOpcodeWidth -
                        mode_segment_length - operand1SegmentLength -
                        operand2SDSegmentLength - operand2SegmentLength -
                        resultSegmentLength);

  std::string operationStr;

  out.output("calc (mode=%d, operand1=%d, "
             "operand2SD=%s, operand2=%s, result=%d)\n",
             mode, operand1, operand2SD ? "dynamic" : "static",
             operand2SD ? std::to_string(operand2).c_str()
                        : std::bitset<8>(operand2).to_string().c_str(),
             result);

  uint32_t operand1_tmp = 0;
  operand1_tmp = scalarRegisters[operand1];
  uint32_t operand2_tmp = 0;
  if (operand2SD)
    operand2_tmp = scalarRegisters[operand2];
  else
    operand2_tmp = operand2;

  switch (mode) {
  case 0:
    operationStr = "idle";
    break;

  case 1:
    operationStr = "add";
    scalarRegisters[result] = operand1_tmp + operand2_tmp;
    break;
  case 2:
    operationStr = "sub";
    scalarRegisters[result] = operand1_tmp - operand2_tmp;
    break;
  case 3:
    operationStr = "lls";
    scalarRegisters[result] = operand1_tmp << operand2_tmp;
    break;
  case 4:
    operationStr = "lrs";
    scalarRegisters[result] = operand1_tmp >> operand2_tmp;
    break;
  case 5:
    operationStr = "mul";
    scalarRegisters[result] = operand1_tmp * operand2_tmp;
    break;
  case 6:
    operationStr = "div";
    scalarRegisters[result] = operand1_tmp / operand2_tmp;
    break;
  case 7:
    operationStr = "mod";
    scalarRegisters[result] = operand1_tmp % operand2_tmp;
    break;
  case 8:
    operationStr = "bitand";
    scalarRegisters[result] = operand1_tmp & operand2_tmp;
    break;
  case 9:
    operationStr = "bitor";
    scalarRegisters[result] = operand1_tmp | operand2_tmp;
    break;
  case 10:
    operationStr = "bitinv";
    scalarRegisters[result] = ~operand1_tmp;
    break;
  case 11:
    operationStr = "bitxor";
    scalarRegisters[result] = operand1_tmp ^ operand2_tmp;
    break;
  case 17:
    operationStr = "eq";
    scalarRegisters[result] = (operand1_tmp == operand2_tmp);
    break;
  case 18:
    operationStr = "ne";
    scalarRegisters[result] = (operand1_tmp != operand2_tmp);
    break;
  case 19:
    operationStr = "gt";
    scalarRegisters[result] = (operand1_tmp > operand2_tmp);
    break;
  case 20:
    operationStr = "ge";
    scalarRegisters[result] = (operand1_tmp >= operand2_tmp);
    break;
  case 21:
    operationStr = "lt";
    scalarRegisters[result] = (operand1_tmp < operand2_tmp);
    break;
  case 22:
    operationStr = "le";
    scalarRegisters[result] = (operand1_tmp <= operand2_tmp);
    break;
  case 23:
    operationStr = "addh";
    scalarRegisters[result] =
        (operand1_tmp & 0xFF) + ((operand2_tmp & 0xFF) << 8);
    break;
  case 32:
    operationStr = "and";
    scalarRegisters[result] = operand1_tmp && operand2_tmp;
    break;
  case 33:
    operationStr = "or";
    scalarRegisters[result] = operand1_tmp || operand2_tmp;
    break;
  case 34:
    operationStr = "not";
    scalarRegisters[result] = !operand1_tmp;
    break;

  default:
    out.fatal(CALL_INFO, -1, "Invalid operation mode\n");
    break;
  }
}

void Sequencer::branch(uint32_t instr) {
  uint32_t regSegmentLength = 4;
  int32_t targetTrueSegmentLength = 9;
  int32_t targetFalseSegmentLength = 9;

  // Check validity
  if (regSegmentLength + targetTrueSegmentLength + targetFalseSegmentLength >
      instrBitwidth - instrTypeBitwidth - instrOpcodeWidth) {
    out.fatal(CALL_INFO, -1, "Invalid instruction format\n");
  }

  // Extract segments
  uint32_t reg = getInstrField(instr, regSegmentLength,
                               instrBitwidth - instrTypeBitwidth -
                                   instrOpcodeWidth - regSegmentLength);
  int32_t targetTrue =
      getInstrField(instr, targetTrueSegmentLength,
                    instrBitwidth - instrTypeBitwidth - instrOpcodeWidth -
                        regSegmentLength - targetTrueSegmentLength);
  int32_t targetFalse = getInstrField(
      instr, targetFalseSegmentLength,
      instrBitwidth - instrTypeBitwidth - instrOpcodeWidth - regSegmentLength -
          targetTrueSegmentLength - targetFalseSegmentLength);

  // Sign extend targetTrue and targetFalse
  if (targetTrue & (1 << (targetTrueSegmentLength - 1))) {
    targetTrue |= ~((1 << targetTrueSegmentLength) - 1);
  }
  if (targetFalse & (1 << (targetFalseSegmentLength - 1))) {
    targetFalse |= ~((1 << targetFalseSegmentLength) - 1);
  }

  // Compute new PC
  if (scalarRegisters[reg]) {
    pc += targetTrue;
  } else {
    pc += targetFalse;
  }

  out.output("BRANCH: reg=%s, targetTrue=%d, targetFalse=%d\n",
             std::bitset<4>(reg).to_string().c_str(), targetTrue, targetFalse);
}
