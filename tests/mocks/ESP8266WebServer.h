#pragma once
#include "Arduino.h"
class ESP8266WebServer {
public:
    String arg(const char*) { return "{}"; }
};
