#include <iostream>
#include <cassert>
#include "mocks/common.h"
#include "mocks/ESP8266WebServer.h"
#include "mocks/WebSocketsServer.h"
#include "mocks/SimpleKalmanFilter.h"

ESP8266WebServer* server = nullptr;
WebSocketsServer* webSocket = nullptr;
SimpleKalmanFilter* kalmanFilter = nullptr;

// Forward declarations for functions in json_helpers.ino
bool jsonVarToUnsignedLong(const JSONVar &value, unsigned long &parsedValue);
bool jsonVarToUint8(const JSONVar &value, uint8_t &parsedValue);
bool jsonVarToString(const JSONVar &value, String &parsedValue);
bool jsonVarToBool(const JSONVar &value, bool &parsedValue);
bool jsonVarToFloat(const JSONVar &value, float &parsedValue);
void jsonWrite(JsonOutput &output, const String &value);
void jsonWrite(JsonOutput &output, const char *value);
void writeEscapedJsonString(JsonOutput &output, const String &value);

#include "../json_helpers.ino"

void test_jsonVarToUint8_valid() {
    JSONVar val("123");
    uint8_t result = 0;
    assert(jsonVarToUint8(val, result) == true);
    assert(result == 123);
    std::cout << "test_jsonVarToUint8_valid passed\n";
}

void test_jsonVarToUint8_min() {
    JSONVar val("0");
    uint8_t result = 255;
    assert(jsonVarToUint8(val, result) == true);
    assert(result == 0);
    std::cout << "test_jsonVarToUint8_min passed\n";
}

void test_jsonVarToUint8_max() {
    JSONVar val("255");
    uint8_t result = 0;
    assert(jsonVarToUint8(val, result) == true);
    assert(result == 255);
    std::cout << "test_jsonVarToUint8_max passed\n";
}

void test_jsonVarToUint8_too_large() {
    JSONVar val("256");
    uint8_t result = 0;
    assert(jsonVarToUint8(val, result) == false);
    std::cout << "test_jsonVarToUint8_too_large passed\n";
}

void test_jsonVarToUint8_negative() {
    JSONVar val("-1");
    uint8_t result = 0;
    assert(jsonVarToUint8(val, result) == false);
    std::cout << "test_jsonVarToUint8_negative passed\n";
}

void test_jsonVarToUint8_invalid_string() {
    JSONVar val("abc");
    uint8_t result = 0;
    assert(jsonVarToUint8(val, result) == false);
    std::cout << "test_jsonVarToUint8_invalid_string passed\n";
}

void test_jsonVarToUint8_float() {
    JSONVar val("12.3");
    uint8_t result = 0;
    assert(jsonVarToUint8(val, result) == false);
    std::cout << "test_jsonVarToUint8_float passed\n";
}

int main() {
    test_jsonVarToUint8_valid();
    test_jsonVarToUint8_min();
    test_jsonVarToUint8_max();
    test_jsonVarToUint8_too_large();
    test_jsonVarToUint8_negative();
    test_jsonVarToUint8_invalid_string();
    test_jsonVarToUint8_float();
    std::cout << "All tests passed!\n";
    return 0;
}
