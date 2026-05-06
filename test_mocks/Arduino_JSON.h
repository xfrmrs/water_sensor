#pragma once
#include "Arduino.h"

class JSONVar {
public:
    bool hasOwnProperty(const char*) const { return false; }
    JSONVar operator[](const char*) const { return JSONVar(); }
};

class JSONMock {
public:
    JSONVar parse(const String&) { return JSONVar(); }
    const char* typeof_(const JSONVar&) { return "undefined"; }
};

extern JSONMock JSON;
