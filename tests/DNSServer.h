#pragma once

#include <cstdint>

#include "ESP8266WiFi.h"

enum DNSReplyCode {
  NoError = 0
};

class DNSServer {
public:
  void stop() {}
  void processNextRequest() {}
  void setErrorReplyCode(DNSReplyCode) {}
  void start(uint16_t, const char *, const IPAddress &) {}
};
