#pragma once

#include <iostream>
#include <string>
#include <stdint.h>
#include <stddef.h>
#include <cassert>

// Mock for Arduino PROGMEM strings
class __FlashStringHelper;
#define F(x) (reinterpret_cast<const __FlashStringHelper*>(x))

// Mock String class
class String {
public:
    std::string s;
    String() {}
    String(const char* str) : s(str) {}
    String(const __FlashStringHelper* str) : s((const char*)str) {}

    void trim() {}
    void reserve(size_t) {}
    size_t length() const { return s.length(); }

    bool operator==(const String& other) const { return s == other.s; }
    bool operator!=(const String& other) const { return s != other.s; }

    String& operator+=(const String& other) { s += other.s; return *this; }
    String& operator+=(const char* other) { s += other; return *this; }
    String& operator+=(const __FlashStringHelper* other) { s += (const char*)other; return *this; }

    String& operator=(const char* other) { s = other; return *this; }
    String& operator=(const String& other) { s = other.s; return *this; }
    String& operator=(const __FlashStringHelper* other) { s = (const char*)other; return *this; }
};

// Mock IPAddress
struct IPAddress {
    bool fromString(const String& s) { return true; }
};

// Mock JSON library
class JSONVar {
public:
    bool hasOwnProperty(const char* key) const { return false; }
    JSONVar operator[](const char* key) const { return JSONVar(); }
};

class JSONClass {
public:
    JSONVar parse(const String& s) { return JSONVar(); }
    String typeof_(const JSONVar& v) { return "undefined"; }
};
extern JSONClass JSON;
#define typeof typeof_

// Mock Serial
class SerialClass {
public:
    void print(const char* s) {}
    void print(int s) {}
    void print(const String& s) {}
    void println(const char* s) {}
    void println(const String& s) {}
    void println(const __FlashStringHelper* s) {}
    void print(const __FlashStringHelper* s) {}
    void println(int s) {}
};
extern SerialClass Serial;

// Mock FS (LittleFS)
class File {
public:
    operator bool() const { return false; }
    String readString() { return ""; }
    void close() {}
    size_t print(const String& s) { return 0; }
};

class LittleFSClass {
public:
    bool begin() { return true; }
    bool exists(const char* path) { return false; }
    File open(const char* path, const char* mode) { return File(); }
    bool remove(const char* path) { return true; }
    bool rename(const char* from, const char* to) { return true; }
};
extern LittleFSClass LittleFS;

// Mock WebServer classes
class ESP8266WebServer {
public:
    String arg(const char* name) { return ""; }
};

class WebSocketsServer {};
class SimpleKalmanFilter {};

// Provide stubs for json_helpers.ino functions used in config.ino
struct JsonOutput;
bool requireBoolField(const JSONVar &json, const char *name, bool &target);
bool optionalBoolField(const JSONVar &json, const char *name, bool &target);
bool requireUint8Field(const JSONVar &json, const char *name, uint8_t &target);
bool optionalUint8Field(const JSONVar &json, const char *name, uint8_t &target);
bool requireUnsignedLongField(const JSONVar &json, const char *name, unsigned long &target);
bool optionalUnsignedLongField(const JSONVar &json, const char *name, unsigned long &target);
bool requireFloatField(const JSONVar &json, const char *name, float &target);
bool optionalFloatField(const JSONVar &json, const char *name, float &target);
bool requireStringField(const JSONVar &json, const char *name, String &target);
bool optionalStringField(const JSONVar &json, const char *name, String &target);

void jsonWrite(JsonOutput &output, const char* s);
void writeJsonBoolField(JsonOutput &output, bool &first, const char* name, bool value);
void writeJsonUIntField(JsonOutput &output, bool &first, const char* name, uint8_t value);
void writeJsonULongField(JsonOutput &output, bool &first, const char* name, unsigned long value);
void writeJsonFloatField(JsonOutput &output, bool &first, const char* name, float value, uint8_t decimals);
void writeJsonStringField(JsonOutput &output, bool &first, const char* name, const String& value);

JsonOutput makeStringJsonOutput(String &payload);

bool jsonVarToBool(const JSONVar& var, bool& out);
bool jsonVarToString(const JSONVar& var, String& out);

// Forward declarations
struct Config;
void printConfigSummary(const Config &source, const __FlashStringHelper *label);
