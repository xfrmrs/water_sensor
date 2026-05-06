#pragma once
#include "WString.h"

class ESP8266WebServer {
public:
    void sendContent(const char*) {}
    void sendContent(const String&) {}
class ESP8266WebServer {
public:
    String arg(const char*) { return ""; }
    void sendContent(const char* content) {}
    void sendContent(const String& content) {}
};
