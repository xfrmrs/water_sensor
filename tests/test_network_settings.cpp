#include <iostream>
#include <cassert>

// Use custom String mock that has trim()
#include "String.h"
#include "Arduino.h"
#include "ESP8266WiFi.h"
#include "ESP8266WebServer.h"
#include "LittleFS.h"
#include "Arduino_JSON.h"
#include "SimpleKalmanFilter.h"
#include "WebSocketsServer.h"
#include "Hash.h"

// Forward declaration of the function we are testing
struct Config;
bool networkSettingsDiffer(const Config &left, const Config &right);

// Mock implementation to avoid linking errors
#include "../common.h"

Config config;
Config bootConfig;
ESP8266WebServer *server;
WebSocketsServer *webSocket;
SimpleKalmanFilter *kalmanFilter;
MeasurementSnapshot latestMeasurement;
HistorySample measurementHistory[MAX_HISTORY_BUFFER_CAPACITY];
uint8_t measurementHistoryCount;
uint8_t measurementHistoryHead;
int count;
bool filling;
bool fileSystemReady;
bool networkReady;
bool networkReconnectPending;
bool restartPending;
unsigned long networkReconnectAfterMs;
unsigned long restartAfterMs;
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

SerialMock Serial;
LittleFSClass LittleFS;
JSONClass JSON;

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
JsonOutput makeStringJsonOutput(String &target) { return JsonOutput(); }
void writeJsonBoolField(JsonOutput &output, bool &first, const char *name, bool value) {}
void writeJsonUIntField(JsonOutput &output, bool &first, const char *name, uint32_t value) {}
void writeJsonULongField(JsonOutput &output, bool &first, const char *name, unsigned long value) {}
void writeJsonFloatField(JsonOutput &output, bool &first, const char *name, float value, uint8_t decimals) {}
void writeJsonStringField(JsonOutput &output, bool &first, const char *name, const String &value) {}
void jsonWrite(JsonOutput &output, const char *value) {}
bool jsonVarToBool(const JSONVar &json, bool &target) { return false; }
bool jsonVarToString(const JSONVar &json, String &target) { return false; }

#include "../config.ino"

void runTests() {
    std::cout << "Running tests for networkSettingsDiffer..." << std::endl;
    Config c1;
    c1.wifiStaSsid = "HomeNet";
    c1.wifiStaPassword = "password123";
    c1.wifiApSsid = "WaterSensor";
    c1.wifiApPassword = "admin";
    c1.wifiStaIp = "192.168.1.100";
    c1.wifiStaGateway = "192.168.1.1";
    c1.wifiStaSubnet = "255.255.255.0";
    c1.wifiApIp = "10.0.0.1";
    c1.wifiApGateway = "10.0.0.1";
    c1.wifiApSubnet = "255.255.255.0";
    c1.wifiStaConnectTimeoutMs = 15000;

    Config c2 = c1;

    // Test identical configs
    assert(networkSettingsDiffer(c1, c2) == false);

    // Test each field differ
    c2.wifiStaSsid = "OtherNet";
    assert(networkSettingsDiffer(c1, c2) == true);
    c2.wifiStaSsid = c1.wifiStaSsid;

    c2.wifiStaPassword = "otherpassword";
    assert(networkSettingsDiffer(c1, c2) == true);
    c2.wifiStaPassword = c1.wifiStaPassword;

    c2.wifiApSsid = "OtherAp";
    assert(networkSettingsDiffer(c1, c2) == true);
    c2.wifiApSsid = c1.wifiApSsid;

    c2.wifiApPassword = "otheradmin";
    assert(networkSettingsDiffer(c1, c2) == true);
    c2.wifiApPassword = c1.wifiApPassword;

    c2.wifiStaIp = "192.168.1.101";
    assert(networkSettingsDiffer(c1, c2) == true);
    c2.wifiStaIp = c1.wifiStaIp;

    c2.wifiStaGateway = "192.168.1.254";
    assert(networkSettingsDiffer(c1, c2) == true);
    c2.wifiStaGateway = c1.wifiStaGateway;

    c2.wifiStaSubnet = "255.255.0.0";
    assert(networkSettingsDiffer(c1, c2) == true);
    c2.wifiStaSubnet = c1.wifiStaSubnet;

    c2.wifiApIp = "10.0.0.2";
    assert(networkSettingsDiffer(c1, c2) == true);
    c2.wifiApIp = c1.wifiApIp;

    c2.wifiApGateway = "10.0.0.254";
    assert(networkSettingsDiffer(c1, c2) == true);
    c2.wifiApGateway = c1.wifiApGateway;

    c2.wifiApSubnet = "255.0.0.0";
    assert(networkSettingsDiffer(c1, c2) == true);
    c2.wifiApSubnet = c1.wifiApSubnet;

    c2.wifiStaConnectTimeoutMs = 20000;
    assert(networkSettingsDiffer(c1, c2) == true);
    c2.wifiStaConnectTimeoutMs = c1.wifiStaConnectTimeoutMs;

    std::cout << "All tests passed!" << std::endl;
}

int main() {
    runTests();
    return 0;
}
