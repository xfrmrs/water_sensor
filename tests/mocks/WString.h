#pragma once
#include <string>
#include <stdint.h>

class String {
    std::string _str;
public:
    String() {}
    String(const char* str) : _str(str ? str : "") {}
    String(std::string str) : _str(str) {}
    String(const String& str) : _str(str._str) {}
    String(unsigned int val) : _str(std::to_string(val)) {}
    String(unsigned long val) : _str(std::to_string(val)) {}
    String(float val, uint8_t dec) : _str(std::to_string(val)) {}

    const char* c_str() const { return _str.c_str(); }
    bool operator==(const char* other) const { return _str == other; }
    bool operator!=(const char* other) const { return _str != other; }
    String& operator+=(const char* other) { _str += other; return *this; }
    String& operator+=(const String& other) { _str += other._str; return *this; }
    size_t length() const { return _str.length(); }
};
