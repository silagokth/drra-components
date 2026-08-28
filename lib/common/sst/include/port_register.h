#ifndef _PORT_REGISTER_H
#define _PORT_REGISTER_H

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

// Independent physical wires between a resource and the switch box.
//   WORD -> narrow word_channels, routed intracell by the SWB crossbar
//   BULK -> wide bulk_intracell / intercell channels, routed by ROUTE configs
// These mirror the two separate always_comb routing blocks in swb.sv.j2.
enum class PortChannel : uint8_t { WORD = 0, BULK = 1 };

// A held value on a data wire. "Idle" is represented as an all-zero value:
// the RTL inter-resource data buses have no valid signal, an undriven bus just
// defaults to '0 (see rf.sv.j2 word_data_out_0='0 when !word_r_en, and the
// SWB crossbar's '0 default). So change detection is purely on the value.
struct PortValue {
  std::vector<uint8_t> data;
  size_t bits = 0;

  PortValue() = default;
  PortValue(std::vector<uint8_t> d, size_t b) : data(std::move(d)), bits(b) {}

  bool operator==(const PortValue &o) const {
    return bits == o.bits && data == o.data;
  }
  bool operator!=(const PortValue &o) const { return !(*this == o); }

  bool isZero() const {
    for (uint8_t b : data)
      if (b != 0)
        return false;
    return true;
  }
};

// One physical output wire modelled as a hardware register.
//   q -> value currently driving the wire (stable within a clock cycle)
//   d -> next-state, computed combinationally during the cycle
// registered == false : combinational output, q follows d the same cycle.
// registered == true  : q <= d only at the clock edge (one cycle of latency);
//                       chain several for a multi-stage pipeline.
struct PortRegister {
  PortValue q;
  PortValue d;
  bool registered = false;
};

// Map key for a (data-link index, channel) wire.
using PortKey = std::pair<uint32_t, PortChannel>;

#endif // _PORT_REGISTER_H
