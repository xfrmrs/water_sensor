#pragma once
#include <string>
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
