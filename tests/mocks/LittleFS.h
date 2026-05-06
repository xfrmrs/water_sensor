#pragma once
#include "Arduino.h"

class File {
public:
    File() : _valid(false) {}
    File(bool v) : _valid(v) {}
    operator bool() const { return _valid; }
    String readString() { return "{}"; }
    void close() {}
    size_t print(const String&) { return 0; }
private:
    bool _valid;
};

class LittleFS_ {
public:
    bool begin_result = true;
    bool begin() { return begin_result; }
    bool exists(const char*) { return true; }
    File open(const char*, const char*) { return File(true); }
    bool remove(const char*) { return true; }
    bool rename(const char*, const char*) { return true; }
};
extern LittleFS_ LittleFS;
class File {
public:
    operator bool() const { return false; }
    String readString() { return ""; }
    void close() {}
    size_t print(const String&) { return 0; }
};

class LittleFSClass {
public:
    bool begin() { return true; }
    bool exists(const char*) { return false; }
    File open(const char*, const char*) { return File(); }
    bool remove(const char*) { return true; }
    bool rename(const char*, const char*) { return true; }
};
extern LittleFSClass LittleFS;
