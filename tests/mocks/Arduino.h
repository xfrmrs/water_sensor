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
