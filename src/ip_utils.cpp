#include "ip_utils.h"

bool parseIpAddressString(const String &value, IPAddress &parsedValue) {
  IPAddress candidate;
  if (!candidate.fromString(value)) {
    return false;
  }

  parsedValue = candidate;
  return true;
}
