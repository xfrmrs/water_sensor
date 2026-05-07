#pragma once
#include <string>
#include <iostream>

class String {
public:
    std::string str;
    String() {}
    String(const char* s) : str(s) {}
    String(const std::string& s) : str(s) {}
    String& operator+=(const String& other) {
        str += other.str;
        return *this;
    }
    String& operator+=(const char* other) {
        str += other;
        return *this;
    }
    bool operator==(const String& other) const {
        return str == other.str;
    }
    bool operator==(const char* other) const {
        return str == std::string(other);
    }
    void trim() {}
    const char* c_str() const { return str.c_str(); }
};

#define F(x) x
#define PROGMEM
#define ICACHE_FLASH_ATTR

typedef unsigned char uint8_t;
typedef unsigned long uint32_t;
