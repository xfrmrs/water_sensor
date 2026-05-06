#pragma once
#include "Arduino.h"

class JSONVar {
public:
    JSONVar() {}
    JSONVar(int) {}
    JSONVar(const char*) {}
    JSONVar(bool) {}
    JSONVar(double) {}
    bool hasOwnProperty(const char*) const { return true; }
    JSONVar operator[](const char*) const { return JSONVar(); }
};

class JSON_ {
public:
    JSONVar parse(const String&) { return JSONVar(); }
    String typeof_(const JSONVar&) { return "object"; }
#define typeof typeof_
};
extern JSON_ JSON;

bool requireBoolField(const JSONVar&, const char*, bool&) { return true; }
bool optionalBoolField(const JSONVar&, const char*, bool&) { return true; }
bool requireUint8Field(const JSONVar&, const char*, uint8_t&) { return true; }
bool optionalUint8Field(const JSONVar&, const char*, uint8_t&) { return true; }
bool requireUnsignedLongField(const JSONVar&, const char*, unsigned long&) { return true; }
bool optionalUnsignedLongField(const JSONVar&, const char*, unsigned long&) { return true; }
bool requireFloatField(const JSONVar&, const char*, float&) { return true; }
bool optionalFloatField(const JSONVar&, const char*, float&) { return true; }
bool requireStringField(const JSONVar&, const char*, String&) { return true; }
bool optionalStringField(const JSONVar&, const char*, String&) { return true; }

void writeJsonBoolField(...) {}
void writeJsonUIntField(...) {}
void writeJsonULongField(...) {}
void writeJsonFloatField(...) {}
void writeJsonStringField(...) {}
void jsonWrite(...) {}

bool jsonVarToBool(const JSONVar&, bool&) { return true; }
bool jsonVarToString(const JSONVar&, String&) { return true; }
