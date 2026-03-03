#include "instruction.h"
#include <bitset>
#include <cassert>
#include <cstdint>

Instruction::Instruction(uint32_t raw_instr) {
  // Default format
  format.total_bitwidth = 32;
  format.type_bitwidth = 1;
  format.opcode_bitwidth = 3;
  format.slot_bitwidth = 4;

  _raw = raw_instr;

  type = getType(raw_instr);
  opcode = getOpcode(raw_instr);
  slot = getSlot(raw_instr);
  segments.push_back(Segment{"type", type});
  segments.push_back(Segment{"opcode", opcode});
  segments.push_back(Segment{"slot", slot});
}

Instruction::Instruction(uint32_t raw_instr, Format fmt)
    : Instruction(raw_instr) {
  _raw = raw_instr;
  format = fmt;
}

Instruction::Instruction(uint32_t raw_instr, Format fmt,
                         std::vector<SegmentRange> &segments_def)
    : Instruction(raw_instr, fmt) {
  _raw = raw_instr;
  for (auto &seg : segments_def) {
    uint32_t seg_value = getInstrField(raw_instr, seg.bitwidth, seg.offset);
    segments.push_back(Segment{seg.name, seg_value});
  }
}

Segment Instruction::get(std::string name) const {
  for (auto &seg : segments) {
    if (seg.name == name) {
      return seg;
    }
  }
  return Segment{};
}

uint32_t Instruction::getInstrField(uint32_t instr, uint32_t fieldWidth,
                                    uint32_t fieldOffset) {
  assert(fieldWidth + fieldOffset <= format.total_bitwidth);
  assert(fieldWidth > 0);
  return (instr & ((1 << fieldWidth) - 1) << fieldOffset) >> fieldOffset;
}

uint32_t Instruction::getType(uint32_t raw_instr) {
  return getInstrField(raw_instr, format.type_bitwidth,
                       format.total_bitwidth - format.type_bitwidth);
}

uint32_t Instruction::getOpcode(uint32_t raw_instr) {
  return getInstrField(raw_instr, format.opcode_bitwidth,
                       format.total_bitwidth - format.type_bitwidth -
                           format.opcode_bitwidth);
}

uint32_t Instruction::getSlot(uint32_t raw_instr) {
  return getInstrField(raw_instr, format.slot_bitwidth,
                       format.total_bitwidth - format.type_bitwidth -
                           format.opcode_bitwidth - format.slot_bitwidth);
}

std::string Instruction::toString() const {
  std::string result = "Instruction(";
  for (size_t i = 0; i < segments.size(); i++) {
    result += segments[i].name + "=" + std::to_string(segments[i].value);
    if (i < segments.size() - 1) {
      result += ", ";
    }
  }
  result += ")";
  return result;
}

std::string Instruction::toBinaryString() const {
  return std::bitset<32>(_raw).to_string();
}

std::string Instruction::toHexString() const {

  char buffer[9];
  std::snprintf(buffer, sizeof(buffer), "%08X", _raw);
  return std::string(buffer);
}
