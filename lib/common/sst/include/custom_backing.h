#ifndef _CUS_BACKING_H
#define _CUS_BACKING_H

#include "Array.hpp"
#include "sst/elements/memHierarchy/membackend/backing.h"
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <regex>
#include <stdexcept>
#include <string>
#include <sys/mman.h>
#include <unistd.h>

#define MAX_WIDTH 1024
#define MAX_DEPTH 65536

namespace SST {
namespace MemHierarchy {
namespace Backend {

class BackingIO : public Backing {
private:
  std::string memory_file_;
  size_t width_, depth_;
  bool read_only_ = true;
  Array<MAX_DEPTH, MAX_WIDTH> *io_;

public:
  BackingIO(std::string memory_file, size_t io_width, size_t io_depth,
            bool read_only)
      : Backing(), memory_file_(memory_file), width_(io_width),
        depth_(io_depth), read_only_(read_only) {
    // Check if exceeding maximum width or depth
    if ((io_width > MAX_WIDTH) || (io_depth > MAX_DEPTH)) {
      throw std::runtime_error("Exceeding maximum width or depth");
    }
    io_ = new Array<MAX_DEPTH, MAX_WIDTH>();

    // if no name is provided, do not init the array
    if (memory_file.empty()) {
      return;
    }

    // Load memory file
    ifstream ifs(memory_file);
    if (!ifs.is_open()) {
      cout << "Error: Can't open file: " << memory_file << endl;
      abort();
    }

    std::string current_line;
    while (std::getline(ifs, current_line)) {
      std::smatch sm;
      std::regex e1("\\s*(\\d+)\\s+([01]+)\\s*");
      if (std::regex_match(current_line.cbegin(), current_line.cend(), sm,
                           e1)) {
        int addr = stoi(sm[1]);
        string raw_data = sm[2];
        raw_data.resize(MAX_WIDTH, '0');
        io_->write(addr, 1, raw_data);
      }
    }
  }

  ~BackingIO() {
    // Dump memory to file
    if ((!read_only_) && (!memory_file_.empty())) {
      dump(nullptr);
      delete io_;
    }
  }

  void set(Addr addr, uint8_t value) {
    string raw_data = std::bitset<8>(value).to_string();
    raw_data.resize(MAX_WIDTH, '0');
    io_->write(addr, 1, raw_data);
  }

  void set(Addr addr, size_t size, std::vector<uint8_t> &data) {
    // Pack `size` bytes into the top of a fresh MAX_WIDTH-bit word directly,
    // reproducing the legacy bit layout without building a MAX_WIDTH-char
    // bit-string per access. Byte d occupies bits [base + 8d, base + 8d + 7]
    // with base = MAX_WIDTH - 8*size (data[size-1] at the MSB end), matching
    // the old bytes -> bit-string -> bitset path exactly.
    // Guard the base computation: if size exceeds the word width in bytes,
    // `MAX_WIDTH - 8*size` underflows (size_t) and slice.set() throws
    // out_of_range. A word write can never be wider than the backing word.
    if (8 * size > MAX_WIDTH) {
      throw std::out_of_range(
          "BackingIO::set: size (" + std::to_string(size) +
          " bytes) exceeds word width (" + std::to_string(MAX_WIDTH / 8) +
          " bytes)");
    }
    std::bitset<MAX_WIDTH> slice;
    size_t base = MAX_WIDTH - 8 * size;
    for (size_t d = 0; d < size; d++) {
      uint8_t byte = data[d];
      for (int b = 0; b < 8; b++) {
        if ((byte >> b) & 1u) {
          slice.set(base + 8 * d + b);
        }
      }
    }
    io_->set_slice(addr, slice);
  }

  uint8_t get(Addr addr) {
    string raw_data = io_->read(addr, 1);
    return std::bitset<8>(raw_data).to_ulong();
  }

  void get(Addr addr, size_t size, std::vector<uint8_t> &data) {
    // Unpack width_/8 bytes from the top of the addressed word (inverse of
    // set), reading the bitset chunk directly instead of round-tripping
    // through a MAX_WIDTH-char string. `size` is ignored, matching the legacy
    // behaviour where only the addressed chunk's bits were used.
    (void)size;
    std::bitset<MAX_WIDTH> slice = io_->get_slice(addr);
    size_t base = MAX_WIDTH - width_;
    for (size_t i = 0; i < width_ / 8; i++) {
      uint8_t byte = 0;
      for (int b = 0; b < 8; b++) {
        if (slice.test(base + 8 * i + b)) {
          byte |= (1u << b);
        }
      }
      data.push_back(byte);
    }
  }

  void printToFile(std::string outfile) {
    FILE *fp = fopen(outfile.c_str(), "w");
    dump(fp);
    fclose(fp);
  }

  void dump(FILE *fp) {
    if (fp == nullptr) {
      fp = fopen(memory_file_.c_str(), "w");
    }
    if (fp == nullptr) {
      cout << "Error: Can't open file: " << memory_file_ << endl;
      abort();
    }
    // Dump memory to file
    vector<size_t> active_rows = io_->get_active_rows();
    std::string current_row;
    for (size_t idx : active_rows) {
      current_row = io_->read(idx, 1);
      current_row.resize(width_, '0');
      fprintf(fp, "%zu %s\n", idx, current_row.c_str());
    }
  }

  void printToScreen(Addr addr_offset, Addr addr_start,
                     Addr addr_interleave_size, Addr addr_interleave_step) {
    vector<size_t> active_rows = io_->get_active_rows();
    std::string current_row;
    for (size_t idx : active_rows) {
      current_row = io_->read(idx, 1);
      current_row.resize(width_, '0');
      cout << idx << " " << current_row << endl;
    }
  }

  void setReadOnly(bool ro) { read_only_ = ro; }
};
} // namespace Backend
} // namespace MemHierarchy
} // namespace SST

#endif // _CUS_BACKING_H
