#pragma once
#include <cstdint>
#include <cstddef>
#include <string>

#define PROGMEM

class __FlashStringHelper;
#define F(X) ((const __FlashStringHelper*)(X))

class String {
    std::string s;
public:
    String() {}
    String(const char* c) : s(c) {}
    String(const __FlashStringHelper* f) : s((const char*)f) {}
    size_t length() const { return s.length(); }
    void trim() {}
    void reserve(size_t) {}
    bool operator==(const String& o) const { return s == o.s; }
    int indexOf(const char* c) const { auto pos = s.find(c); return pos == std::string::npos ? -1 : pos; }
    bool operator!=(const String& o) const { return s != o.s; }
    String& operator+=(const char* c) { s += c; return *this; }
    String& operator+=(const String& o) { s += o.s; return *this; }
    String& operator+=(const __FlashStringHelper* f) { s += (const char*)f; return *this; }
    String& operator=(const char* c) { s = c; return *this; }
    String& operator=(const __FlashStringHelper* f) { s = (const char*)f; return *this; }
};

class IPAddress {
public:
    bool fromString(const String& str) { return true; }
};

class SerialMock {
public:
    void println(const char*) {}
    void println(const String&) {}
    void println(unsigned long) {}
    void println(uint8_t) {}
    void println(const __FlashStringHelper*) {}
    void print(const char*) {}
    void print(const String&) {}
    void print(unsigned long) {}
    void print(uint8_t) {}
    void print(char) {}
    void print(const __FlashStringHelper*) {}
};

extern SerialMock Serial;
