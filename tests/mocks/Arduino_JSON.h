#pragma once
#include <string>
class JSONVar {
public:
    bool hasOwnProperty(const char*) const { return false; }
    JSONVar operator[](const char*) const { return JSONVar(); }
#include "Arduino.h"

class JSONVar {
public:
    std::string type_str;
    std::string string_val;

    JSONVar() : type_str("undefined") {}
    JSONVar(const char* s) : type_str("string"), string_val(s) {}

    bool hasOwnProperty(const char* name) const { return false; }
    JSONVar operator[](const char* name) const { return JSONVar(); }

    operator const char*() const {
        return string_val.c_str();
    }
};

class JSONClass {
public:
    JSONVar parse(const std::string&) { return JSONVar(); }
    std::string typeof_(const JSONVar&) { return "undefined"; }
};
extern JSONClass JSON;
#define typeof typeof_
    String typeof_(const JSONVar& v) {
        return String(v.type_str.c_str());
    }
    String stringify(const JSONVar& v) {
        return String(v.string_val.c_str());
    }
};

#define typeof typeof_

extern JSONClass JSON;
