#ifndef _ACTEVENT_H
#define _ACTEVENT_H

#include <sst/core/event.h>

using namespace SST;

class ActEvent : public Event {
public:
  ActEvent() {}
  ~ActEvent() {}

  // Fabric contract: must match the RTL templates and the compiler's
  // loop-counter pool (r15..r13).
  static constexpr uint32_t NUM_LOOP_LEVELS = 3;

  // Data members
  uint32_t slot_id;
  uint32_t ports;
  // Loop counters by nesting depth (0 = outermost), broadcast to every
  // activated resource, which offsets addresses by stride * loop_vars[level].
  uint32_t loop_vars[NUM_LOOP_LEVELS] = {0, 0, 0};

  ActEvent *clone() override { return new ActEvent(*this); }

  void serialize_order(SST::Core::Serialization::serializer &ser) override {
    Event::serialize_order(ser);
  }

  bool isPortEnabled(uint32_t port) { return (ports & (1 << port)) >> port; }

  ImplementSerializable(ActEvent);
};

#endif // _ACTEVENT_H
