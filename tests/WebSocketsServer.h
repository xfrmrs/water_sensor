#pragma once

#include <cstddef>
#include <cstdint>

#include "Arduino.h"

enum WStype_t {
  WStype_CONNECTED,
  WStype_DISCONNECTED,
  WStype_TEXT
};

typedef void (*WebSocketEventHandler)(uint8_t, WStype_t, uint8_t *, size_t);

class WebSocketsServer {
public:
  explicit WebSocketsServer(uint16_t = 81) {}

  void begin() {}
  void onEvent(WebSocketEventHandler) {}
  void loop() {}
  bool broadcastTXT(String &) { return true; }
  bool sendTXT(uint8_t, String &) { return true; }
};
