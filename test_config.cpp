#include "test_mocks/Arduino.h"
#include "test_mocks/Arduino_JSON.h"
#include "test_mocks/ESP8266WiFi.h"
#include "test_mocks/ESP8266WebServer.h"
#include "test_mocks/Hash.h"
#include "test_mocks/LittleFS.h"
#include "test_mocks/SimpleKalmanFilter.h"
#include "test_mocks/WebSocketsServer.h"

#include "common.h"

Config config;
Config bootConfig;
ESP8266WebServer *server = nullptr;
WebSocketsServer *webSocket = nullptr;
SimpleKalmanFilter *kalmanFilter = nullptr;
MeasurementSnapshot latestMeasurement;
HistorySample measurementHistory[120];
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
SerialMock Serial;
LittleFSMock LittleFS;
JSONMock JSON;

bool requireBoolField(const JSONVar &json, const char *name, bool &target) { return true; }
bool optionalBoolField(const JSONVar &json, const char *name, bool &target) { return true; }
bool requireUint8Field(const JSONVar &json, const char *name, uint8_t &target) { return true; }
bool optionalUint8Field(const JSONVar &json, const char *name, uint8_t &target) { return true; }
bool requireUnsignedLongField(const JSONVar &json, const char *name, unsigned long &target) { return true; }
bool optionalUnsignedLongField(const JSONVar &json, const char *name, unsigned long &target) { return true; }
bool requireFloatField(const JSONVar &json, const char *name, float &target) { return true; }
bool optionalFloatField(const JSONVar &json, const char *name, float &target) { return true; }
bool requireStringField(const JSONVar &json, const char *name, String &target) { return true; }
bool optionalStringField(const JSONVar &json, const char *name, String &target) { return true; }

void writeJsonBoolField(JsonOutput &output, bool &first, const char *key, bool value) {}
void writeJsonUIntField(JsonOutput &output, bool &first, const char *key, unsigned int value) {}
void writeJsonULongField(JsonOutput &output, bool &first, const char *key, unsigned long value) {}
void writeJsonFloatField(JsonOutput &output, bool &first, const char *key, float value, uint8_t decimals) {}
void writeJsonStringField(JsonOutput &output, bool &first, const char *key, const String &value) {}
void writeJsonStringField(JsonOutput &output, bool &first, const char *key, const char *value) {}
void jsonWrite(JsonOutput &output, const char *value) {}

JsonOutput makeStringJsonOutput(String &target) { return JsonOutput(); }

bool jsonVarToBool(const JSONVar &value, bool &parsedValue) { return true; }
bool jsonVarToString(const JSONVar &value, String &parsedValue) { return true; }

void printConfigSummary(const Config &source, const __FlashStringHelper *label);

#define typeof typeof_
#include "config.ino"
#undef typeof

#include <cassert>
#include <iostream>

void test_arePinsUnique_all_unique() {
    Config c;
    c.trigPin = 4;
    c.echoPin = 5;
    c.waterPin = 12;
    c.errLedPin = 13;
    assert(arePinsUnique(c) == true);
}

void test_arePinsUnique_duplicate_trig_echo() {
    Config c;
    c.trigPin = 4;
    c.echoPin = 4;
    c.waterPin = 12;
    c.errLedPin = 13;
    assert(arePinsUnique(c) == false);
}

void test_arePinsUnique_duplicate_water_err() {
    Config c;
    c.trigPin = 4;
    c.echoPin = 5;
    c.waterPin = 12;
    c.errLedPin = 12;
    assert(arePinsUnique(c) == false);
}

void test_arePinsUnique_all_same() {
    Config c;
    c.trigPin = 4;
    c.echoPin = 4;
    c.waterPin = 4;
    c.errLedPin = 4;
    assert(arePinsUnique(c) == false);
}

void test_networkSettingsDiffer_identical() {
    Config left;
    left.wifiStaSsid = "MySSID";
    left.wifiStaPassword = "MyPassword";
    left.wifiApSsid = "MyAP";
    left.wifiApPassword = "MyAPPassword";
    left.wifiStaIp = "192.168.1.100";
    left.wifiStaGateway = "192.168.1.1";
    left.wifiStaSubnet = "255.255.255.0";
    left.wifiApIp = "192.168.4.1";
    left.wifiApGateway = "192.168.4.1";
    left.wifiApSubnet = "255.255.255.0";
    left.wifiStaConnectTimeoutMs = 15000;

    Config right = left;
    assert(networkSettingsDiffer(left, right) == false);
}

void test_networkSettingsDiffer_different_fields() {
    Config left;
    left.wifiStaSsid = "A";
    left.wifiStaPassword = "A";
    left.wifiApSsid = "A";
    left.wifiApPassword = "A";
    left.wifiStaIp = "A";
    left.wifiStaGateway = "A";
    left.wifiStaSubnet = "A";
    left.wifiApIp = "A";
    left.wifiApGateway = "A";
    left.wifiApSubnet = "A";
    left.wifiStaConnectTimeoutMs = 10000;

    Config right = left;
    assert(networkSettingsDiffer(left, right) == false);

    right.wifiStaSsid = "B";
    assert(networkSettingsDiffer(left, right) == true);
    right.wifiStaSsid = left.wifiStaSsid;

    right.wifiStaPassword = "B";
    assert(networkSettingsDiffer(left, right) == true);
    right.wifiStaPassword = left.wifiStaPassword;

    right.wifiApSsid = "B";
    assert(networkSettingsDiffer(left, right) == true);
    right.wifiApSsid = left.wifiApSsid;

    right.wifiApPassword = "B";
    assert(networkSettingsDiffer(left, right) == true);
    right.wifiApPassword = left.wifiApPassword;

    right.wifiStaIp = "B";
    assert(networkSettingsDiffer(left, right) == true);
    right.wifiStaIp = left.wifiStaIp;

    right.wifiStaGateway = "B";
    assert(networkSettingsDiffer(left, right) == true);
    right.wifiStaGateway = left.wifiStaGateway;

    right.wifiStaSubnet = "B";
    assert(networkSettingsDiffer(left, right) == true);
    right.wifiStaSubnet = left.wifiStaSubnet;

    right.wifiApIp = "B";
    assert(networkSettingsDiffer(left, right) == true);
    right.wifiApIp = left.wifiApIp;

    right.wifiApGateway = "B";
    assert(networkSettingsDiffer(left, right) == true);
    right.wifiApGateway = left.wifiApGateway;

    right.wifiApSubnet = "B";
    assert(networkSettingsDiffer(left, right) == true);
    right.wifiApSubnet = left.wifiApSubnet;

    right.wifiStaConnectTimeoutMs = 20000;
    assert(networkSettingsDiffer(left, right) == true);
    right.wifiStaConnectTimeoutMs = left.wifiStaConnectTimeoutMs;
}

int main() {
    test_arePinsUnique_all_unique();
    test_arePinsUnique_duplicate_trig_echo();
    test_arePinsUnique_duplicate_water_err();
    test_arePinsUnique_all_same();
    std::cout << "All arePinsUnique tests passed!" << std::endl;

    test_networkSettingsDiffer_identical();
    test_networkSettingsDiffer_different_fields();
    std::cout << "All networkSettingsDiffer tests passed!" << std::endl;

    return 0;
}
