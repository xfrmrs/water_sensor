#pragma once
#include "WString.h"

class ESP8266WebServer {
public:
    void sendContent(const char*) {}
    void sendContent(const String&) {}
};
