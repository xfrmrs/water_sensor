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
