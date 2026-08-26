#ifndef _ACTEVENT_H
#define _ACTEVENT_H

#include <sst/core/event.h>

using namespace SST;

class ActEvent : public Event {
public:
  ActEvent() {}
  ~ActEvent() {}

  // Data members
  uint32_t slot_id;
  uint32_t ports;
  // Current loop iteration index, broadcast to every activated resource. 0 when
  // not inside a loop; resources with a configured stride offset their address
  // generation by stride * loop_var.
  uint32_t loop_var = 0;

  ActEvent *clone() override { return new ActEvent(*this); }

  void serialize_order(SST::Core::Serialization::serializer &ser) override {
    Event::serialize_order(ser);
  }

  bool isPortEnabled(uint32_t port) { return (ports & (1 << port)) >> port; }

  ImplementSerializable(ActEvent);
};

#endif // _ACTEVENT_H
