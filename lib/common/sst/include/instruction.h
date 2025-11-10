#ifndef _INSTRUCTION_H
#define _INSTRUCTION_H

#include <cstdint>
#include <string>
#include <vector>

class Segment {
public:
  Segment() = default;

  std::string name;
  uint32_t value;
};

struct SegmentRange {
  std::string name;
  uint32_t bitwidth;
  uint32_t offset;

  SegmentRange(const std::string &n, uint32_t width, uint32_t off)
      : name(n), bitwidth(width), offset(off) {}
};

class Instruction {
public:
  class Format {
  public:
    uint32_t total_bitwidth;
    uint32_t type_bitwidth;
    uint32_t opcode_bitwidth;
    uint32_t slot_bitwidth;
  };

  Format format;

  uint32_t type;
  uint32_t opcode;
  uint32_t slot;

  std::vector<Segment> segments;

  Segment get(std::string name) const;

  Instruction(uint32_t raw_instr);
  Instruction(uint32_t raw_instr, Format fmt);
  Instruction(uint32_t raw_instr, Format fmt,
              std::vector<SegmentRange> &segments_def);

  std::string toString() const;
  std::string toBinaryString() const;
  std::string toHexString() const;

private:
  uint32_t _raw;
  uint32_t getType(uint32_t instr);
  uint32_t getOpcode(uint32_t instr);
  uint32_t getSlot(uint32_t instr);
  uint32_t getInstrField(uint32_t instr, uint32_t fieldWidth,
                         uint32_t fieldOffset);
};

#endif // _INSTRUCTION_H
