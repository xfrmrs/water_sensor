#pragma once

#include <sstream>
#include <string>
#include <vector>

#include "Arduino.h"

class IPAddress {
public:
  IPAddress() : octets{0, 0, 0, 0} {}

  bool fromString(const String &value) {
    std::stringstream stream(value.value);
    std::string segment;
    std::vector<int> parts;
    while (std::getline(stream, segment, '.')) {
      if (segment.empty() || segment.length() > 3) {
        return false;
      }
      for (char ch : segment) {
        if (ch < '0' || ch > '9') {
          return false;
        }
      }
      int parsed = std::stoi(segment);
      if (parsed < 0 || parsed > 255) {
        return false;
      }
      parts.push_back(parsed);
    }
    if (parts.size() != 4) {
      return false;
    }
    for (size_t i = 0; i < 4; ++i) {
      octets[i] = (uint8_t)parts[i];
    }
    return true;
  }

  String toString() const {
    std::ostringstream stream;
    stream << (unsigned int)octets[0] << '.'
           << (unsigned int)octets[1] << '.'
           << (unsigned int)octets[2] << '.'
           << (unsigned int)octets[3];
    return String(stream.str());
  }

private:
  uint8_t octets[4];
};

class WiFiClass {};

extern WiFiClass WiFi;
