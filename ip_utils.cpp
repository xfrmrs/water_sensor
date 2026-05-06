#include "ip_utils.h"

#ifndef ARDUINO
// In tests, IPAddress is fully defined in test_config.cpp before ip_utils.cpp is included
#endif

bool parseIpAddressString(const String &value, IPAddress &parsedValue) {
  IPAddress candidate;
  if (!candidate.fromString(value)) {
    return false;
  }

  parsedValue = candidate;
  return true;
}
