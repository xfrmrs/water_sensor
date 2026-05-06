#pragma once
#include "Arduino.h"
class File {
public:
    operator bool() const { return false; }
    String readString() { return String(""); }
    void close() {}
    size_t print(const String&) { return 0; }
};

class LittleFSMock {
public:
    bool begin() { return true; }
    bool exists(const char*) { return false; }
    File open(const char*, const char*) { return File(); }
    bool remove(const char*) { return true; }
    bool rename(const char*, const char*) { return true; }
};

extern LittleFSMock LittleFS;
