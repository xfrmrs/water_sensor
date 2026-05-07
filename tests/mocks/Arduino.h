#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <iostream>

#include "ESP8266WebServer.h"
#include "WebSocketsServer.h"
#include "SimpleKalmanFilter.h"
#include "Arduino_JSON.h"
#include "LittleFS.h"

#define F(x) reinterpret_cast<const __FlashStringHelper*>(x)
#define ICACHE_RAM_ATTR
#define DCACHE_RAM_ATTR
class __FlashStringHelper;

class String {
public:
    std::string s;
    String() {}
    String(const char* str) : s(str) {}
    String(const std::string& str) : s(str) {}
    String(const __FlashStringHelper* str) : s(reinterpret_cast<const char*>(str)) {}
    String& operator=(const char* str) { s = str; return *this; }
    String& operator=(const __FlashStringHelper* str) { s = reinterpret_cast<const char*>(str); return *this; }
    String& operator=(const String& other) { s = other.s; return *this; }
    String& operator+=(const char* str) { s += str; return *this; }
    String& operator+=(const __FlashStringHelper* str) { s += reinterpret_cast<const char*>(str); return *this; }
    String& operator+=(const String& other) { s += other.s; return *this; }
    bool operator==(const char* str) const { return s == str; }
    bool operator!=(const char* str) const { return s != str; }
    bool operator==(const String& other) const { return s == other.s; }
    bool operator!=(const String& other) const { return s != other.s; }
    size_t length() const { return s.length(); }
    void trim() {
        if (s.empty()) return;
        size_t first = s.find_first_not_of(" \t\r\n");
        if (std::string::npos == first) { s = ""; return; }
        size_t last = s.find_last_not_of(" \t\r\n");
        s = s.substr(first, (last - first + 1));
    }
    void reserve(size_t n) {}
    const char* c_str() const { return s.c_str(); }
};

inline bool operator!=(const String& lhs, const String& rhs) { return lhs.s != rhs.s; }

class IPAddress {
public:
    bool fromString(const String& str) {
        if (str.length() == 0) return false;
        int dots = 0;
        for (char c : str.s) { if (c == '.') dots++; else if (c < '0' || c > '9') return false; }
        return dots == 3;
    }
    String toString() const { return String("0.0.0.0"); }
};

class Print {
public:
    virtual size_t print(const char* s) { return 0; }
    virtual size_t print(int n) { return 0; }
    virtual size_t print(unsigned int n) { return 0; }
    virtual size_t print(long n) { return 0; }
    virtual size_t print(unsigned long n) { return 0; }
    virtual size_t print(const String& s) { return 0; }
    virtual size_t print(const __FlashStringHelper* s) { return 0; }
    virtual size_t print(uint8_t n) { return 0; }

    virtual size_t println() { return 0; }
    virtual size_t println(const char* s) { return 0; }
    virtual size_t println(int n) { return 0; }
    virtual size_t println(unsigned int n) { return 0; }
    virtual size_t println(long n) { return 0; }
    virtual size_t println(unsigned long n) { return 0; }
    virtual size_t println(const String& s) { return 0; }
    virtual size_t println(const __FlashStringHelper* s) { return 0; }
    virtual size_t println(uint8_t n) { return 0; }
    virtual size_t println(const __FlashStringHelper*& s) { return 0; }
};

class HardwareSerial : public Print {
public:
    void begin(unsigned long baud) {}
};
extern HardwareSerial Serial;

unsigned long millis();
unsigned long micros();
void delay(unsigned long ms);
void delayMicroseconds(unsigned int us);
#include <stdint.h>
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
