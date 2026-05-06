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
    return 0;
}
