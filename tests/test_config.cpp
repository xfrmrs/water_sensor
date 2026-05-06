#include <iostream>
#include <cassert>

// Include mocks first
#include "mocks/Arduino.h"
#include "mocks/Arduino_JSON.h"
#include "mocks/ESP8266WiFi.h"
#include "mocks/ESP8266WebServer.h"
#include "mocks/Hash.h"
#include "mocks/LittleFS.h"
#include "mocks/SimpleKalmanFilter.h"
#include "mocks/WebSocketsServer.h"

// Include common headers that declare these types
#include "../common.h"

// Provide necessary definitions that are missing since we aren't compiling all .ino files
Config config;
Config bootConfig;
ESP8266WebServer *server = nullptr;
WebSocketsServer *webSocket = nullptr;
SimpleKalmanFilter *kalmanFilter = nullptr;
MeasurementSnapshot latestMeasurement;
HistorySample measurementHistory[MAX_HISTORY_BUFFER_CAPACITY];
uint8_t measurementHistoryCount = 0;
uint8_t measurementHistoryHead = 0;
int count = 0;
bool filling = false;
bool fileSystemReady = false;
bool networkReady = false;
bool networkReconnectPending = false;
bool restartPending = false;
unsigned long networkReconnectAfterMs = 0;
unsigned long restartAfterMs = 0;
String uiStatusMessage;
String reconnectHint;
NetworkMode currentNetworkMode;
String currentNetworkIp;
uint8_t activeTrigPin = 0;
uint8_t activeEchoPin = 0;
uint8_t activeWaterPin = 0;
uint8_t activeErrLedPin = 0;
unsigned long activeSerialBaud = 0;
unsigned long lastAcceptedUs = 0;
unsigned long pendingShortUs = 0;
uint8_t pendingShortCount = 0;
uint8_t invalidBurstCount = 0;
LittleFS_ LittleFS;
JSON_ JSON;
Serial_ Serial;

JsonOutput makeStringJsonOutput(String &payload) {
    JsonOutput out;
    return out;
}

// Forward declare printConfigSummary before config.ino so it resolves inside saveConfigToFs
void printConfigSummary(const Config &source, const __FlashStringHelper *label);

// Instead of redefining beginFileSystem, we include the actual source file here.
#include "../config.ino"

void runTests() {
    std::cout << "Testing beginFileSystem()..." << std::endl;

    // Test case 1: LittleFS.begin() succeeds
    LittleFS.begin_result = true;
    fileSystemReady = false; // Reset state
    bool result = beginFileSystem();
    assert(result == true);
    assert(fileSystemReady == true);
    std::cout << "  [PASS] beginFileSystem() returns true when LittleFS.begin() succeeds" << std::endl;

    // Test case 2: LittleFS.begin() fails
    LittleFS.begin_result = false;
    fileSystemReady = true; // Reset state
    result = beginFileSystem();
    assert(result == false);
    assert(fileSystemReady == false);
    std::cout << "  [PASS] beginFileSystem() returns false when LittleFS.begin() fails" << std::endl;

    std::cout << "All beginFileSystem() tests passed!" << std::endl;
}

int main() {
    runTests();
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
