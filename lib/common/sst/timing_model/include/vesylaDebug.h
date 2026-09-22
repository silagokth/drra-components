#pragma once

#include <cstdlib>

// True when VESYLA_DEBUG is set. Read once: getenv scans the whole environment
// and was called on the per-cycle path.
inline bool vesylaDebug() {
  static const bool enabled = std::getenv("VESYLA_DEBUG") != nullptr;
  return enabled;
}
