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

    explicit operator const char*() const { return _value.c_str(); }
    explicit operator bool() const { return true; }
    explicit operator double() const { return 0.0; }
    explicit operator int() const { return 0; }
    operator const char*() const {
        return string_val.c_str();
    }
};

class JSONClass {
public:
    String stringify(const JSONVar& var) {
        return String(var.value());
    }
    String typeof_(const JSONVar& var) { // 'typeof' is a keyword in g++, JSON.typeof is an error if typeof is a keyword
        return String("string");
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
