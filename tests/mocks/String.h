#pragma once
#include <string>

class String : public std::string {
public:
    String() : std::string() {}
    String(const char* s) : std::string(s) {}
    String(const std::string& s) : std::string(s) {}

    String& operator=(const char* s) {
        std::string::operator=(s);
        return *this;
    }
    String& operator=(const std::string& s) {
        std::string::operator=(s);
        return *this;
    }

    void trim() {}
};
