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

String mock_fail_field = "";

bool requireBoolField(const JSONVar &json, const char *name, bool &target) {
    return mock_fail_field != String(name);
}
bool optionalBoolField(const JSONVar &json, const char *name, bool &target) { return true; }
bool requireUint8Field(const JSONVar &json, const char *name, uint8_t &target) {
    return mock_fail_field != String(name);
}
bool optionalUint8Field(const JSONVar &json, const char *name, uint8_t &target) { return true; }
bool requireUnsignedLongField(const JSONVar &json, const char *name, unsigned long &target) {
    return mock_fail_field != String(name);
}
bool optionalUnsignedLongField(const JSONVar &json, const char *name, unsigned long &target) { return true; }
bool requireFloatField(const JSONVar &json, const char *name, float &target) {
    return mock_fail_field != String(name);
}
bool optionalFloatField(const JSONVar &json, const char *name, float &target) { return true; }
bool requireStringField(const JSONVar &json, const char *name, String &target) {
    return mock_fail_field != String(name);
}
bool optionalStringField(const JSONVar &json, const char *name, String &target) { return true; }

void writeJsonBoolField(JsonOutput &output, bool &first, const char *key, bool value) {
  if (output.context) {
    String* str = static_cast<String*>(output.context);
    if (!first) *str += ",";
    first = false;
    *str += "\"";
    *str += key;
    *str += "\":";
    *str += (value ? "true" : "false");
  }
}
void writeJsonUIntField(JsonOutput &output, bool &first, const char *key, unsigned int value) {
  if (output.context) {
    String* str = static_cast<String*>(output.context);
    if (!first) *str += ",";
    first = false;
    *str += "\"";
    *str += key;
    *str += "\":";
    *str += std::to_string(value).c_str();
  }
}
void writeJsonULongField(JsonOutput &output, bool &first, const char *key, unsigned long value) {
  if (output.context) {
    String* str = static_cast<String*>(output.context);
    if (!first) *str += ",";
    first = false;
    *str += "\"";
    *str += key;
    *str += "\":";
    *str += std::to_string(value).c_str();
  }
}
void writeJsonFloatField(JsonOutput &output, bool &first, const char *key, float value, uint8_t decimals) {
  if (output.context) {
    String* str = static_cast<String*>(output.context);
    if (!first) *str += ",";
    first = false;
    *str += "\"";
    *str += key;
    *str += "\":";
    *str += std::to_string(value).c_str(); // simplified
  }
}
void writeJsonStringField(JsonOutput &output, bool &first, const char *key, const String &value) {
  if (output.context) {
    String* str = static_cast<String*>(output.context);
    if (!first) *str += ",";
    first = false;
    *str += "\"";
    *str += key;
    *str += "\":\"";
    *str += value;
    *str += "\"";
  }
}
void writeJsonStringField(JsonOutput &output, bool &first, const char *key, const char *value) {
  if (output.context) {
    String* str = static_cast<String*>(output.context);
    if (!first) *str += ",";
    first = false;
    *str += "\"";
    *str += key;
    *str += "\":\"";
    *str += value;
    *str += "\"";
  }
}
void jsonWrite(JsonOutput &output, const char *value) {
  if (output.context) {
    String* str = static_cast<String*>(output.context);
    *str += value;
  }
}

JsonOutput makeStringJsonOutput(String &target) {
  JsonOutput output;
  output.context = &target;
  return output;
}

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

void test_configFromJson_success() {
    JSONVar json;
    Config candidate;
    String errorMessage;

    mock_fail_field = ""; // ensure no failures in fields

    bool result = configFromJson(json, candidate, errorMessage);
    assert(result == true);
    assert(errorMessage == "");
}

void test_configFromJson_missing_wifiStaPassword() {
    JSONVar json;
    Config candidate;
    String errorMessage;

    mock_fail_field = "wifiStaPassword";

    bool result = configFromJson(json, candidate, errorMessage);
    assert(result == false);
    assert(errorMessage == "Missing or invalid field: wifiStaPassword");
}

void test_configFromJson_missing_wifiApPassword() {
    JSONVar json;
    Config candidate;
    String errorMessage;

    mock_fail_field = "wifiApPassword";

    bool result = configFromJson(json, candidate, errorMessage);
    assert(result == false);
    assert(errorMessage == "Missing or invalid field: wifiApPassword");
}

void test_configFromJson_missing_adminPassword() {
    JSONVar json;
    Config candidate;
    String errorMessage;

    mock_fail_field = "adminPassword";

    bool result = configFromJson(json, candidate, errorMessage);
    assert(result == false);
    assert(errorMessage == "Missing or invalid field: adminPassword");
}

void test_configFromJson_missing_shared_field() {
    JSONVar json;
    Config candidate;
    String errorMessage;

    mock_fail_field = "wifiApSsid";

    bool result = configFromJson(json, candidate, errorMessage);
    assert(result == false);
    assert(errorMessage == "Missing or invalid field: wifiApSsid");
}
void test_writeConfigJsonObject_WithSecretsAndFlags();
void test_writeConfigJsonObject_NoSecretsButWithFlags();
void test_writeConfigJsonObject_NoSecretsNoFlags();

void test_saveConfigToFs_fsNotReady();
void test_saveConfigToFs_openFails();
void test_saveConfigToFs_printIncomplete();
void test_saveConfigToFs_removeFails();
void test_saveConfigToFs_renameFails();
void test_saveConfigToFs_success();

int main() {
    test_arePinsUnique_all_unique();
    test_arePinsUnique_duplicate_trig_echo();
    test_arePinsUnique_duplicate_water_err();
    test_arePinsUnique_all_same();
    std::cout << "All arePinsUnique tests passed!" << std::endl;

    test_configFromJson_success();
    test_configFromJson_missing_wifiStaPassword();
    test_configFromJson_missing_wifiApPassword();
    test_configFromJson_missing_adminPassword();
    test_configFromJson_missing_shared_field();
    std::cout << "All configFromJson tests passed!" << std::endl;
    test_writeConfigJsonObject_WithSecretsAndFlags();
    test_writeConfigJsonObject_NoSecretsButWithFlags();
    test_writeConfigJsonObject_NoSecretsNoFlags();
    std::cout << "All writeConfigJsonObject tests passed!" << std::endl;

    test_saveConfigToFs_fsNotReady();
    test_saveConfigToFs_openFails();
    test_saveConfigToFs_printIncomplete();
    test_saveConfigToFs_removeFails();
    test_saveConfigToFs_renameFails();
    test_saveConfigToFs_success();
    std::cout << "All saveConfigToFs tests passed!" << std::endl;

    return 0;
}

void test_writeConfigJsonObject_WithSecretsAndFlags() {
    Config c;
    // Set some distinguishable values
    c.debugControlLogs = true;
    c.trigPin = 5;
    c.wifiStaSsid = "my_ssid";
    c.wifiStaPassword = "my_password";
    c.adminPassword = "admin_password";

    String buffer;
    JsonOutput out = makeStringJsonOutput(buffer);

    writeConfigJsonObject(out, c, true, true);

    std::string s = buffer.s;
    assert(s.find("\"trigPin\":5") != std::string::npos);
    assert(s.find("\"wifiStaSsid\":\"my_ssid\"") != std::string::npos);
    assert(s.find("\"wifiStaPassword\":\"my_password\"") != std::string::npos);
    assert(s.find("\"adminPassword\":\"admin_password\"") != std::string::npos);
    assert(s.find("\"hasStaPassword\":true") != std::string::npos);
    assert(s.find("\"hasAdminPassword\":true") != std::string::npos);
    std::cout << "test_writeConfigJsonObject_WithSecretsAndFlags passed!\n";
}

void test_writeConfigJsonObject_NoSecretsButWithFlags() {
    Config c;
    c.trigPin = 5;
    c.wifiStaPassword = "my_password";
    c.wifiApPassword = "ap_password";

    String buffer;
    JsonOutput out = makeStringJsonOutput(buffer);

    writeConfigJsonObject(out, c, false, true);

    std::string s = buffer.s;
    assert(s.find("\"trigPin\":5") != std::string::npos);
    assert(s.find("\"wifiStaPassword\":\"my_password\"") == std::string::npos);
    assert(s.find("\"wifiApPassword\":\"ap_password\"") == std::string::npos);
    assert(s.find("\"hasStaPassword\":true") != std::string::npos);
    assert(s.find("\"hasApPassword\":true") != std::string::npos);
    std::cout << "test_writeConfigJsonObject_NoSecretsButWithFlags passed!\n";
}

void test_writeConfigJsonObject_NoSecretsNoFlags() {
    Config c;
    c.trigPin = 5;
    c.wifiStaPassword = "my_password";

    String buffer;
    JsonOutput out = makeStringJsonOutput(buffer);

    writeConfigJsonObject(out, c, false, false);

    std::string s = buffer.s;
    assert(s.find("\"trigPin\":5") != std::string::npos);
    assert(s.find("\"wifiStaPassword\":\"my_password\"") == std::string::npos);
    assert(s.find("\"hasStaPassword\":true") == std::string::npos);
    std::cout << "test_writeConfigJsonObject_NoSecretsNoFlags passed!\n";
}

void register_writeConfigJsonObject_tests() {
    // Modify main below to call these tests or just do it here temporarily
}

void test_saveConfigToFs_fsNotReady() {
    littleFsMockState.reset();
    fileSystemReady = false;
    assert(saveConfigToFs() == false);
    std::cout << "test_saveConfigToFs_fsNotReady passed!\n";
}

void test_saveConfigToFs_openFails() {
    littleFsMockState.reset();
    fileSystemReady = true;
    littleFsMockState.openResult = false;
    assert(saveConfigToFs() == false);
    assert(littleFsMockState.openCount == 1);
    assert(littleFsMockState.lastOpenedPath == CONFIG_TEMP_PATH);
    assert(littleFsMockState.lastOpenedMode == "w");
    std::cout << "test_saveConfigToFs_openFails passed!\n";
}

void test_saveConfigToFs_printIncomplete() {
    littleFsMockState.reset();
    fileSystemReady = true;
    littleFsMockState.openResult = true;
    // Simulate printing less than the full payload
    littleFsMockState.printResult = 1;

    assert(saveConfigToFs() == false);
    assert(littleFsMockState.removeCount == 1);
    assert(littleFsMockState.lastRemovedPath == CONFIG_TEMP_PATH);
    std::cout << "test_saveConfigToFs_printIncomplete passed!\n";
}

void test_saveConfigToFs_removeFails() {
    littleFsMockState.reset();
    fileSystemReady = true;
    littleFsMockState.openResult = true;
    littleFsMockState.printResult = (size_t)-1; // Return full length
    littleFsMockState.existsResult = true; // CONFIG_FILE_PATH exists
    littleFsMockState.removeResult = false; // remove fails

    assert(saveConfigToFs() == false);
    assert(littleFsMockState.removeCount == 2); // 1 for exists, 1 for temp
    assert(littleFsMockState.lastRemovedPath == "/config.tmp");
    std::cout << "test_saveConfigToFs_removeFails passed!\n";
}

void test_saveConfigToFs_renameFails() {
    littleFsMockState.reset();
    fileSystemReady = true;
    littleFsMockState.openResult = true;
    littleFsMockState.printResult = (size_t)-1;
    littleFsMockState.existsResult = false; // CONFIG_FILE_PATH does not exist
    littleFsMockState.renameResult = false; // Rename fails

    assert(saveConfigToFs() == false);
    assert(littleFsMockState.renameCount == 1);
    assert(littleFsMockState.removeCount == 1);
    assert(littleFsMockState.lastRemovedPath == CONFIG_TEMP_PATH);
    std::cout << "test_saveConfigToFs_renameFails passed!\n";
}

void test_saveConfigToFs_success() {
    littleFsMockState.reset();
    fileSystemReady = true;
    littleFsMockState.openResult = true;
    littleFsMockState.printResult = (size_t)-1;
    littleFsMockState.existsResult = true;
    littleFsMockState.removeResult = true;
    littleFsMockState.renameResult = true;

    assert(saveConfigToFs() == true);
    assert(littleFsMockState.renameCount == 1);
    assert(littleFsMockState.lastRenamedFrom == CONFIG_TEMP_PATH);
    assert(littleFsMockState.lastRenamedTo == CONFIG_FILE_PATH);
    // removeCount should be 1 because it removed the existing file
    assert(littleFsMockState.removeCount == 1);
    assert(littleFsMockState.lastRemovedPath == CONFIG_FILE_PATH);
    std::cout << "test_saveConfigToFs_success passed!\n";
}
