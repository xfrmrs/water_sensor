#pragma once

#include "Arduino.h"

class File;

class ESP8266WebServer {
public:
  explicit ESP8266WebServer(uint16_t = 80) {}

  template <typename Handler>
  void on(const char *, int, Handler) {}

  template <typename Handler>
  void onNotFound(Handler) {}

  String arg(const char *) const { return requestBody; }
  void setArg(const String &value) { requestBody = value; }

  void send(int, const char *, const String &) {}
  void send(int, const char *, const char *) {}
  void sendContent(const char *) {}
  void sendContent(const String &) {}
  void sendHeader(const char *, const char *) {}
  void setContentLength(int) {}
  void requestAuthentication() {}
  bool authenticate(const char *, const char *) const { return true; }
  String uri() const { return requestUri; }
  void setUri(const String &value) { requestUri = value; }
  void begin() {}
  void stop() {}
  void handleClient() {}
  size_t streamFile(File &, const char *) { return 0; }

private:
  String requestBody;
  String requestUri;
};
