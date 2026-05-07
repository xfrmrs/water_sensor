#include "mocks/Arduino.h"


#include "../common.h"
#include <iostream>
#include <cassert>

// Globals from common.h
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

HardwareSerial Serial;
JSONClass JSON;
LittleFSClass LittleFS;

// Redefine F macro to avoid compile errors with String
#undef F
#define F(x) x


// Include our dynamically extracted SUT that contains the real functions
#include "extracted_sut.cpp"

// Test helper
Config getValidConfig() {
    Config c;
    c.waterMaxDuration = 10;
    c.loopDelayMs = 10;
    c.waterDelayMs = 10;
    c.waterHighUs = 100;
    c.waterLowUs = 200;
    c.waterErrUs = 300;
    c.pulseTimeoutUs = 400;
    c.maxConfigurablePings = 10;
    c.nPings = 5;
    c.minValidPings = 3;
    c.pingGapMs = 10;
    c.minValidEchoUs = 10;
    c.shortConfirmCount = 5;
    c.maxHeldInvalidBursts = 5;
    c.trigPin = 4;
    c.echoPin = 5;
    c.waterPin = 12;
    c.errLedPin = 13;
    c.serialBaud = 115200;
    c.httpPort = 80;
    c.websocketPort = 81;
    c.historyCapacity = 100;
    c.kalmanMeasurementError = 1.0;
    c.kalmanEstimateError = 1.0;
    c.kalmanProcessNoise = 1.0;
    c.wifiApSsid = "TestAP";
    c.wifiApPassword = "testpassword";
    c.adminPassword = "admin";
    c.wifiStaConnectTimeoutMs = 10000;
    c.enableStationDhcp = true;
    c.wifiStaIp = "192.168.1.100";
    c.wifiStaGateway = "192.168.1.1";
    c.wifiStaSubnet = "255.255.255.0";
    c.wifiApIp = "192.168.4.1";
    c.wifiApGateway = "192.168.4.1";
    c.wifiApSubnet = "255.255.255.0";
    return c;
}

void runTests() {
    String errorMsg;
    Config c;

    std::cout << "Testing valid config..." << std::endl;
    c = getValidConfig();
    assert(validateConfig(c, errorMsg) == true);

    std::cout << "Testing waterMaxDuration == 0..." << std::endl;
    c = getValidConfig(); c.waterMaxDuration = 0;
    assert(validateConfig(c, errorMsg) == false);

    std::cout << "Testing loopDelayMs == 0..." << std::endl;
    c = getValidConfig(); c.loopDelayMs = 0;
    assert(validateConfig(c, errorMsg) == false);

    std::cout << "Testing waterDelayMs == 0..." << std::endl;
    c = getValidConfig(); c.waterDelayMs = 0;
    assert(validateConfig(c, errorMsg) == false);

    std::cout << "Testing waterHighUs >= waterLowUs..." << std::endl;
    c = getValidConfig(); c.waterHighUs = 200; c.waterLowUs = 200;
    assert(validateConfig(c, errorMsg) == false);
    c.waterHighUs = 201;
    assert(validateConfig(c, errorMsg) == false);

    std::cout << "Testing waterLowUs >= waterErrUs..." << std::endl;
    c = getValidConfig(); c.waterLowUs = 300; c.waterErrUs = 300;
    assert(validateConfig(c, errorMsg) == false);

    std::cout << "Testing pulseTimeoutUs < waterErrUs..." << std::endl;
    c = getValidConfig(); c.pulseTimeoutUs = 299;
    assert(validateConfig(c, errorMsg) == false);

    std::cout << "Testing nPings == 0..." << std::endl;
    c = getValidConfig(); c.nPings = 0;
    assert(validateConfig(c, errorMsg) == false);

    std::cout << "Testing nPings > maxConfigurablePings..." << std::endl;
    c = getValidConfig(); c.nPings = 11;
    assert(validateConfig(c, errorMsg) == false);

    std::cout << "Testing minValidPings == 0..." << std::endl;
    c = getValidConfig(); c.minValidPings = 0;
    assert(validateConfig(c, errorMsg) == false);

    std::cout << "Testing minValidPings > nPings..." << std::endl;
    c = getValidConfig(); c.minValidPings = 6;
    assert(validateConfig(c, errorMsg) == false);

    std::cout << "Testing pingGapMs == 0..." << std::endl;
    c = getValidConfig(); c.pingGapMs = 0;
    assert(validateConfig(c, errorMsg) == false);

    std::cout << "Testing minValidEchoUs == 0..." << std::endl;
    c = getValidConfig(); c.minValidEchoUs = 0;
    assert(validateConfig(c, errorMsg) == false);

    std::cout << "Testing shortConfirmCount == 0..." << std::endl;
    c = getValidConfig(); c.shortConfirmCount = 0;
    assert(validateConfig(c, errorMsg) == false);

    std::cout << "Testing maxHeldInvalidBursts == 0..." << std::endl;
    c = getValidConfig(); c.maxHeldInvalidBursts = 0;
    assert(validateConfig(c, errorMsg) == false);

    std::cout << "Testing isSafePinValue..." << std::endl;
    c = getValidConfig(); c.trigPin = 2; // Not in allowlist
    assert(validateConfig(c, errorMsg) == false);

    std::cout << "Testing arePinsUnique..." << std::endl;
    c = getValidConfig(); c.trigPin = 4; c.echoPin = 4;
    assert(validateConfig(c, errorMsg) == false);

    std::cout << "Testing isSupportedBaud..." << std::endl;
    c = getValidConfig(); c.serialBaud = 1234;
    assert(validateConfig(c, errorMsg) == false);

    std::cout << "Testing httpPort == 0 or > 65535..." << std::endl;
    c = getValidConfig(); c.httpPort = 0;
    assert(validateConfig(c, errorMsg) == false);
    c.httpPort = 65536;
    assert(validateConfig(c, errorMsg) == false);

    std::cout << "Testing websocketPort == 0 or > 65535..." << std::endl;
    c = getValidConfig(); c.websocketPort = 0;
    assert(validateConfig(c, errorMsg) == false);
    c.websocketPort = 65536;
    assert(validateConfig(c, errorMsg) == false);

    std::cout << "Testing historyCapacity..." << std::endl;
    c = getValidConfig(); c.historyCapacity = 0;
    assert(validateConfig(c, errorMsg) == false);
    c.historyCapacity = MAX_HISTORY_BUFFER_CAPACITY + 1;
    assert(validateConfig(c, errorMsg) == false);

    std::cout << "Testing maxConfigurablePings..." << std::endl;
    c = getValidConfig(); c.maxConfigurablePings = 0;
    assert(validateConfig(c, errorMsg) == false);
    c.maxConfigurablePings = MAX_PING_BUFFER_CAPACITY + 1;
    assert(validateConfig(c, errorMsg) == false);

    std::cout << "Testing kalman parameters..." << std::endl;
    c = getValidConfig(); c.kalmanMeasurementError = 0.0;
    assert(validateConfig(c, errorMsg) == false);
    c = getValidConfig(); c.kalmanEstimateError = -1.0;
    assert(validateConfig(c, errorMsg) == false);
    c = getValidConfig(); c.kalmanProcessNoise = 0.0;
    assert(validateConfig(c, errorMsg) == false);

    std::cout << "Testing wifiApSsid..." << std::endl;
    c = getValidConfig(); c.wifiApSsid = "";
    assert(validateConfig(c, errorMsg) == false);

    std::cout << "Testing wifiApPassword..." << std::endl;
    c = getValidConfig(); c.wifiApPassword = "short";
    assert(validateConfig(c, errorMsg) == false);
    c.wifiApPassword = ""; // Blank is allowed
    assert(validateConfig(c, errorMsg) == true);

    std::cout << "Testing adminPassword..." << std::endl;
    c = getValidConfig(); c.adminPassword = "123";
    assert(validateConfig(c, errorMsg) == false);

    std::cout << "Testing wifiStaConnectTimeoutMs..." << std::endl;
    c = getValidConfig(); c.wifiStaConnectTimeoutMs = 0;
    assert(validateConfig(c, errorMsg) == false);

    std::cout << "Testing DHCP disabled requires valid IP, Gateway, Subnet..." << std::endl;
    c = getValidConfig(); c.enableStationDhcp = false; c.wifiStaIp = "";
    assert(validateConfig(c, errorMsg) == false);
    c = getValidConfig(); c.enableStationDhcp = false; c.wifiStaIp = "invalid";
    assert(validateConfig(c, errorMsg) == false);
    c = getValidConfig(); c.enableStationDhcp = false;
    assert(validateConfig(c, errorMsg) == true);

    std::cout << "Testing wifiApIp, Gateway, Subnet..." << std::endl;
    c = getValidConfig(); c.wifiApIp = "invalid";
    assert(validateConfig(c, errorMsg) == false);

    std::cout << "All exhaustive tests passed!" << std::endl;
}

int main() {
    runTests();
    return 0;
}
