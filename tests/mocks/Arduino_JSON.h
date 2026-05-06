#pragma once
#include <string>
#include "WString.h"

class JSONVar {
    std::string _value;
public:
    JSONVar() {}
    JSONVar(const char* val) : _value(val) {}
    JSONVar(std::string val) : _value(val) {}
    JSONVar(int val) : _value(std::to_string(val)) {}

    std::string value() const { return _value; }

    bool hasOwnProperty(const char* name) const { return false; }
    JSONVar operator[](const char* name) const { return JSONVar(); }

    explicit operator const char*() const { return _value.c_str(); }
    explicit operator bool() const { return true; }
    explicit operator double() const { return 0.0; }
    explicit operator int() const { return 0; }
};

class JSONClass {
public:
    String stringify(const JSONVar& var) {
        return String(var.value());
    }
    String typeof_(const JSONVar& var) { // 'typeof' is a keyword in g++, JSON.typeof is an error if typeof is a keyword
        return String("string");
    }
};

#define typeof typeof_

extern JSONClass JSON;
