#include "mocks/Arduino.h"
#include "mocks/Arduino_JSON.h"
#include <iostream>

JSONClass JSON;

// Provide definitions for globals needed by json_helpers.ino via common.h
#include "../common.h"

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
uint8_t activeTrigPin;
uint8_t activeEchoPin;
uint8_t activeWaterPin;
uint8_t activeErrLedPin;
unsigned long activeSerialBaud;
unsigned long lastAcceptedUs;
unsigned long pendingShortUs;
uint8_t pendingShortCount;
uint8_t invalidBurstCount;

// Forward declarations to fix order in json_helpers.ino
inline void jsonWrite(JsonOutput &output, const char *value);
inline void jsonWrite(JsonOutput &output, const String &value);
void writeEscapedJsonString(JsonOutput &output, const String &value);

#include "../json_helpers.ino"

void test_jsonVarToString() {
    std::cout << "Testing jsonVarToString...\n";

    // 1. Valid string
    JSONVar strVar("test string");
    String outStr;
    bool res1 = jsonVarToString(strVar, outStr);
    if (!res1 || outStr != "test string") {
        std::cerr << "FAILED: Valid string test" << std::endl;
        exit(1);
    }

    // 2. Empty string
    JSONVar emptyStrVar("");
    String outEmptyStr;
    bool res2 = jsonVarToString(emptyStrVar, outEmptyStr);
    if (!res2 || outEmptyStr != "") {
        std::cerr << "FAILED: Empty string test" << std::endl;
        exit(1);
    }

    // 3. Not a string (e.g. number)
    JSONVar notStrVar;
    notStrVar.type_str = "number";
    notStrVar.string_val = "123";
    String outNotStr;
    bool res3 = jsonVarToString(notStrVar, outNotStr);
    if (res3) {
        std::cerr << "FAILED: Non-string (number) test" << std::endl;
        exit(1);
    }

    // 4. Not a string (boolean)
    JSONVar boolVar;
    boolVar.type_str = "boolean";
    boolVar.string_val = "true";
    String outBoolVar;
    bool res4 = jsonVarToString(boolVar, outBoolVar);
    if (res4) {
        std::cerr << "FAILED: Non-string (boolean) test" << std::endl;
        exit(1);
    }

    std::cout << "All jsonVarToString tests passed!" << std::endl;
}

int main() {
    test_jsonVarToString();
    return 0;
}
