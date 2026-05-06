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
