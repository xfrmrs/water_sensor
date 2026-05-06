#pragma once
#include <string>
class JSONVar {
public:
    bool hasOwnProperty(const char*) const { return false; }
    JSONVar operator[](const char*) const { return JSONVar(); }
};

class JSONClass {
public:
    JSONVar parse(const std::string&) { return JSONVar(); }
    std::string typeof_(const JSONVar&) { return "undefined"; }
};
extern JSONClass JSON;
#define typeof typeof_
