#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string>
#include <iostream>

class __FlashStringHelper;
#define F(X) (reinterpret_cast<const __FlashStringHelper *>(X))

class String {
public:
    std::string str;
    String() {}
    String(const char* s) : str(s ? s : "") {}
    String(const String& s) : str(s.str) {}
    String(const __FlashStringHelper* s) : str(reinterpret_cast<const char*>(s) ? reinterpret_cast<const char*>(s) : "") {}

    String& operator=(const String& s) { str = s.str; return *this; }
    String& operator=(const char* s) { str = s; return *this; }
    String& operator=(const __FlashStringHelper* s) { str = reinterpret_cast<const char*>(s); return *this; }

    void trim() {}
    size_t length() const { return str.length(); }
    void reserve(size_t) {}
    String& operator+=(const char* s) { str += s; return *this; }
    String& operator+=(const String& s) { str += s.str; return *this; }
    String& operator+=(const __FlashStringHelper* s) { str += reinterpret_cast<const char*>(s); return *this; }

    bool operator==(const String& s) const { return str == s.str; }
    bool operator!=(const String& s) const { return str != s.str; }
    bool operator==(const char* s) const { return str == s; }
    bool operator!=(const char* s) const { return str != s; }
    const char* c_str() const { return str.c_str(); }
};

class Serial_ {
public:
    void print(const char*) {}
    void print(int) {}
    void print(char) {}
    void print(unsigned long) {}
    void println(const char*) {}
    void println(const String&) {}
    void println(int) {}
    void println(unsigned long) {}
    void println(const __FlashStringHelper*) {}
    void print(const __FlashStringHelper*) {}
};
extern Serial_ Serial;

class IPAddress {
public:
    bool fromString(const String&) { return true; }
};
#include <string.h>
#include <stdio.h>
#include "WString.h"

#define ICACHE_FLASH_ATTR
#define PROGMEM
#define PGM_P const char *
#define F(string_literal) string_literal

inline unsigned long millis() { return 0; }
inline void delay(unsigned long) {}

inline char* ultoa(unsigned long value, char* str, int base) {
    if (base == 10) sprintf(str, "%lu", value);
    return str;
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
