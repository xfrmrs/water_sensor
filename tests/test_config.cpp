#include "mock_common.h"

// Define globals
Config config;
Config bootConfig;
bool fileSystemReady = false;
ESP8266WebServer *server = nullptr;
SerialClass Serial;
JSONClass JSON;
LittleFSClass LittleFS;

// Define stubs for json_helpers.ino dependencies
bool requireBoolField(const JSONVar &json, const char *name, bool &target) { return false; }
bool optionalBoolField(const JSONVar &json, const char *name, bool &target) { return false; }
bool requireUint8Field(const JSONVar &json, const char *name, uint8_t &target) { return false; }
bool optionalUint8Field(const JSONVar &json, const char *name, uint8_t &target) { return false; }
bool requireUnsignedLongField(const JSONVar &json, const char *name, unsigned long &target) { return false; }
bool optionalUnsignedLongField(const JSONVar &json, const char *name, unsigned long &target) { return false; }
bool requireFloatField(const JSONVar &json, const char *name, float &target) { return false; }
bool optionalFloatField(const JSONVar &json, const char *name, float &target) { return false; }
bool requireStringField(const JSONVar &json, const char *name, String &target) { return false; }
bool optionalStringField(const JSONVar &json, const char *name, String &target) { return false; }

void jsonWrite(JsonOutput &output, const char* s) {}
void writeJsonBoolField(JsonOutput &output, bool &first, const char* name, bool value) {}
void writeJsonUIntField(JsonOutput &output, bool &first, const char* name, uint8_t value) {}
void writeJsonULongField(JsonOutput &output, bool &first, const char* name, unsigned long value) {}
void writeJsonFloatField(JsonOutput &output, bool &first, const char* name, float value, uint8_t decimals) {}
void writeJsonStringField(JsonOutput &output, bool &first, const char* name, const String& value) {}

JsonOutput makeStringJsonOutput(String &payload) { return JsonOutput(); }

bool jsonVarToBool(const JSONVar& var, bool& out) { return false; }
bool jsonVarToString(const JSONVar& var, String& out) { return false; }

// Include the implementation we want to test
// We undefine WATER_SENSOR_COMMON_H to avoid conflicts if config.ino checks it,
// though it just includes "common.h". We already provided mock_common.h
// Wait, we can't easily intercept `#include "common.h"` inside config.ino unless we put a fake common.h in tests/

// Actually, config.ino starts with #include "common.h".
// If we compile test_config.cpp with -I tests, it will find our tests/common.h !
#include "../config.ino"

void resetConfig(Config& c) {
    c.trigPin = 5;
    c.echoPin = 4;
    c.waterPin = 12;
    c.errLedPin = 13;
    c.serialBaud = 115200;
    c.httpPort = 80;
    c.websocketPort = 81;
    c.wifiStaSsid = "test_ssid";
    c.loopDelayMs = 1000;
}

void runTests() {
    std::cout << "Running tests for restartRequired()...\n";

    // 1. Identical configs
    resetConfig(bootConfig);
    resetConfig(config);
    assert(restartRequired() == false);

    // 2. Different trigPin
    resetConfig(config);
    config.trigPin = 14;
    assert(restartRequired() == true);

    // 3. Different echoPin
    resetConfig(config);
    config.echoPin = 14;
    assert(restartRequired() == true);

    // 4. Different waterPin
    resetConfig(config);
    config.waterPin = 14;
    assert(restartRequired() == true);

    // 5. Different errLedPin
    resetConfig(config);
    config.errLedPin = 14;
    assert(restartRequired() == true);

    // 6. Different serialBaud
    resetConfig(config);
    config.serialBaud = 9600;
    assert(restartRequired() == true);

    // 7. Different httpPort
    resetConfig(config);
    config.httpPort = 8080;
    assert(restartRequired() == true);

    // 8. Different websocketPort
    resetConfig(config);
    config.websocketPort = 8081;
    assert(restartRequired() == true);

    // 9. Different wifiStaSsid (non-restart sensitive)
    resetConfig(config);
    config.wifiStaSsid = "new_ssid";
    assert(restartRequired() == false);

    // 10. Different loopDelayMs (non-restart sensitive)
    resetConfig(config);
    config.loopDelayMs = 2000;
    assert(restartRequired() == false);

    std::cout << "All tests passed successfully.\n";
}

int main() {
    runTests();
#include <iostream>
#include <string>
#include <cassert>
#include <cstdint>

// Mocking Arduino structures
using String = std::string;

struct IPAddress {
    uint8_t bytes[4];
    bool isSet = false;

    IPAddress() {
        bytes[0] = 0; bytes[1] = 0; bytes[2] = 0; bytes[3] = 0;
    }

    bool fromString(const String& s) {
        // Mock simple implementation
        int parsedBytes[4] = {0};
        int parsedCount = 0;
        std::string currentNum = "";

        for (char c : s) {
            if (c == '.') {
                if (currentNum.empty() || parsedCount >= 3) return false;
                try { parsedBytes[parsedCount] = std::stoi(currentNum); } catch (...) { return false; }
                if (parsedBytes[parsedCount] < 0 || parsedBytes[parsedCount] > 255) return false;
                currentNum = "";
                parsedCount++;
            } else if (isdigit(c)) {
                currentNum += c;
            } else {
                return false;
            }
        }

        if (currentNum.empty() || parsedCount != 3) return false;
        try { parsedBytes[parsedCount] = std::stoi(currentNum); } catch (...) { return false; }
        if (parsedBytes[parsedCount] < 0 || parsedBytes[parsedCount] > 255) return false;

        bytes[0] = parsedBytes[0];
        bytes[1] = parsedBytes[1];
        bytes[2] = parsedBytes[2];
        bytes[3] = parsedBytes[3];
        isSet = true;
        return true;
    }
};

// Mock Arduino.h to prevent ip_utils.h from failing
// No need to include anything else

#include "../ip_utils.h"

// Provide the implementation to test directly since we mocked the dependencies
#include "../ip_utils.cpp"

// Tests
void test_parseIpAddressString_ValidIP() {
    IPAddress parsed;
    parsed.bytes[0] = 99; parsed.bytes[1] = 99; parsed.bytes[2] = 99; parsed.bytes[3] = 99;

    bool result = parseIpAddressString("192.168.1.1", parsed);
    assert(result == true);
    assert(parsed.bytes[0] == 192);
    assert(parsed.bytes[1] == 168);
    assert(parsed.bytes[2] == 1);
    assert(parsed.bytes[3] == 1);
    assert(parsed.isSet == true);
}

void test_parseIpAddressString_ValidIP_EdgeCases() {
    IPAddress parsed;

    bool result = parseIpAddressString("255.255.255.255", parsed);
    assert(result == true);
    assert(parsed.bytes[0] == 255 && parsed.bytes[1] == 255 && parsed.bytes[2] == 255 && parsed.bytes[3] == 255);

    result = parseIpAddressString("0.0.0.0", parsed);
    assert(result == true);
    assert(parsed.bytes[0] == 0 && parsed.bytes[1] == 0 && parsed.bytes[2] == 0 && parsed.bytes[3] == 0);
}

void test_parseIpAddressString_InvalidIP() {
    IPAddress parsed;
    parsed.bytes[0] = 99; parsed.bytes[1] = 99; parsed.bytes[2] = 99; parsed.bytes[3] = 99;

    // Test various invalid strings
    const char* invalid_strings[] = {
        "invalid-ip",
        "256.1.1.1",
        "192.168.1",
        "192.168.1.1.1",
        "192.168..1",
        "192.168.1.a",
        "",
        " "
    };

    for (const char* invalid_str : invalid_strings) {
        IPAddress p = parsed;
        bool result = parseIpAddressString(invalid_str, p);
        assert(result == false);
        // Values should remain unchanged if parsing fails
        assert(p.bytes[0] == 99);
        assert(p.bytes[1] == 99);
        assert(p.bytes[2] == 99);
        assert(p.bytes[3] == 99);
        assert(p.isSet == false);
    }
}

int main() {
    std::cout << "Running tests for parseIpAddressString...\n";

    test_parseIpAddressString_ValidIP();
    test_parseIpAddressString_ValidIP_EdgeCases();
    test_parseIpAddressString_InvalidIP();

    std::cout << "All tests passed successfully.\n";
    return 0;
}
