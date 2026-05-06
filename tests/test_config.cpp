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
    return 0;
}
