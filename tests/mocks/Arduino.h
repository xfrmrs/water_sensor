#pragma once
#include <string>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <iostream>

#define F(x) (x)

class SerialMock {
public:
    void println(const char*) {}
    void println(unsigned long) {}
    template<typename T> void println(const T&) {}
    void print(const char*) {}
    void print(char) {}
    void print(unsigned long) {}
    void print(uint8_t) {}
};
extern SerialMock Serial;

class IPAddress {
public:
    IPAddress() {}
    bool fromString(const std::string& s) { return true; }
};

typedef const char* FlashStringHelper;
#define __FlashStringHelper char

struct Config;
void printConfigSummary(const Config& source, const char* label);
#include <cstring>
#include <cstdlib>
#include <cstdint>

class String {
public:
    std::string str;
    String() {}
    String(const char* s) : str(s ? s : "") {}
    String(const std::string& s) : str(s) {}
    String(unsigned int v) { str = std::to_string(v); }
    String(unsigned long v) { str = std::to_string(v); }
    String(float v, int decimals = 2) { str = std::to_string(v); }

    const char* c_str() const { return str.c_str(); }
    size_t length() const { return str.length(); }

    bool operator==(const String& other) const { return str == other.str; }
    bool operator==(const char* other) const { return str == std::string(other); }
    bool operator!=(const char* other) const { return str != std::string(other); }

    String& operator+=(const char* other) { str += other; return *this; }
    String& operator+=(const String& other) { str += other.str; return *this; }
};

inline void ultoa(unsigned long value, char* buffer, int radix) {
    if (radix == 10) {
        std::string s = std::to_string(value);
        std::strcpy(buffer, s.c_str());
    }
}
