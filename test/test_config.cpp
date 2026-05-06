#include <iostream>
#include <cassert>
#include <cstdint>
#include <string>

// Lightweight String Mock for testing so we don't compile common.cpp which is a mess.
class String {
public:
    std::string s;
    String() {}
    String(const char* str) : s(str) {}
    bool operator!=(const String& other) const { return s != other.s; }
    bool operator==(const String& other) const { return s == other.s; }
    String& operator+=(const char* str) { s += str; return *this; }
    String& operator+=(const String& str) { s += str.s; return *this; }
    size_t length() const { return s.length(); }
};

struct Config {
  bool debugControlLogs;
  bool debugMeasurementLogs;
  uint8_t trigPin;
  uint8_t echoPin;
  uint8_t waterPin;
  uint8_t errLedPin;
  unsigned long serialBaud;
  float kalmanMeasurementError;
  float kalmanEstimateError;
  float kalmanProcessNoise;
  unsigned long waterMaxDuration;
  unsigned long loopDelayMs;
  unsigned long waterDelayMs;
  unsigned long waterLowUs;
  unsigned long waterHighUs;
  unsigned long waterErrUs;
  unsigned long pulseTimeoutUs;
  uint8_t nPings;
  uint8_t minValidPings;
  unsigned long pingGapMs;
  unsigned long minValidEchoUs;
  unsigned long shortJumpUs;
  unsigned long shortConfirmDeltaUs;
  uint8_t shortConfirmCount;
  uint8_t maxHeldInvalidBursts;
  String wifiStaSsid;
  String wifiStaPassword;
  bool enableStationDhcp;
  String wifiApSsid;
  String wifiApPassword;
  String wifiStaIp;
  String wifiStaGateway;
  String wifiStaSubnet;
  String wifiApIp;
  String wifiApGateway;
  String wifiApSubnet;
  String adminPassword;
  unsigned long wifiStaConnectTimeoutMs;
  unsigned long httpPort;
  unsigned long websocketPort;
  unsigned long historyCapacity;
  unsigned long maxConfigurablePings;
};

// Instead of copy-pasting, we #include config.cpp which we generate by extracting the two functions.
// Actually, extracting functions is cleaner. But to strictly test the actual functions in config.ino, we can use a python script to extract them to a testable header.
#include "config_funcs.cpp"

void testRestartSensitiveSettingsDiffer() {
    Config config1 = {};
    config1.trigPin = 1;
    config1.echoPin = 2;
    config1.waterPin = 3;
    config1.errLedPin = 4;
    config1.serialBaud = 9600;
    config1.httpPort = 80;
    config1.websocketPort = 81;
    config1.historyCapacity = 100;
    config1.maxConfigurablePings = 12;

    Config config2 = config1;

    assert(!restartSensitiveSettingsDiffer(config1, config2));

    config2.trigPin = 5;
    assert(restartSensitiveSettingsDiffer(config1, config2));
    config2.trigPin = config1.trigPin;

    config2.echoPin = 5;
    assert(restartSensitiveSettingsDiffer(config1, config2));
    config2.echoPin = config1.echoPin;

    config2.waterPin = 5;
    assert(restartSensitiveSettingsDiffer(config1, config2));
    config2.waterPin = config1.waterPin;

    config2.errLedPin = 5;
    assert(restartSensitiveSettingsDiffer(config1, config2));
    config2.errLedPin = config1.errLedPin;

    config2.serialBaud = 115200;
    assert(restartSensitiveSettingsDiffer(config1, config2));
    config2.serialBaud = config1.serialBaud;

    config2.httpPort = 8080;
    assert(restartSensitiveSettingsDiffer(config1, config2));
    config2.httpPort = config1.httpPort;

    config2.websocketPort = 8081;
    assert(restartSensitiveSettingsDiffer(config1, config2));
    config2.websocketPort = config1.websocketPort;

    config2.historyCapacity = 120;
    assert(restartSensitiveSettingsDiffer(config1, config2));
    config2.historyCapacity = config1.historyCapacity;

    config2.maxConfigurablePings = 15;
    assert(restartSensitiveSettingsDiffer(config1, config2));
    config2.maxConfigurablePings = config1.maxConfigurablePings;

    config2.debugControlLogs = !config1.debugControlLogs;
    assert(!restartSensitiveSettingsDiffer(config1, config2));

    std::cout << "All tests passed for restartSensitiveSettingsDiffer!" << std::endl;
}

void testNetworkSettingsDiffer() {
    Config config1 = {};
    config1.wifiStaSsid = "SSID1";
    config1.wifiStaPassword = "PASS1";
    config1.wifiApSsid = "AP1";
    config1.wifiApPassword = "APPASS1";
    config1.wifiStaIp = "192.168.1.100";
    config1.wifiStaGateway = "192.168.1.1";
    config1.wifiStaSubnet = "255.255.255.0";
    config1.wifiApIp = "192.168.4.1";
    config1.wifiApGateway = "192.168.4.1";
    config1.wifiApSubnet = "255.255.255.0";
    config1.wifiStaConnectTimeoutMs = 10000;

    Config config2 = config1;

    assert(!networkSettingsDiffer(config1, config2));

    config2.wifiStaSsid = "SSID2";
    assert(networkSettingsDiffer(config1, config2));
    config2.wifiStaSsid = config1.wifiStaSsid;

    config2.wifiStaPassword = "PASS2";
    assert(networkSettingsDiffer(config1, config2));
    config2.wifiStaPassword = config1.wifiStaPassword;

    config2.wifiApSsid = "AP2";
    assert(networkSettingsDiffer(config1, config2));
    config2.wifiApSsid = config1.wifiApSsid;

    config2.wifiApPassword = "APPASS2";
    assert(networkSettingsDiffer(config1, config2));
    config2.wifiApPassword = config1.wifiApPassword;

    config2.wifiStaIp = "192.168.1.101";
    assert(networkSettingsDiffer(config1, config2));
    config2.wifiStaIp = config1.wifiStaIp;

    config2.wifiStaGateway = "192.168.1.2";
    assert(networkSettingsDiffer(config1, config2));
    config2.wifiStaGateway = config1.wifiStaGateway;

    config2.wifiStaSubnet = "255.255.0.0";
    assert(networkSettingsDiffer(config1, config2));
    config2.wifiStaSubnet = config1.wifiStaSubnet;

    config2.wifiApIp = "192.168.4.2";
    assert(networkSettingsDiffer(config1, config2));
    config2.wifiApIp = config1.wifiApIp;

    config2.wifiApGateway = "192.168.4.2";
    assert(networkSettingsDiffer(config1, config2));
    config2.wifiApGateway = config1.wifiApGateway;

    config2.wifiApSubnet = "255.255.0.0";
    assert(networkSettingsDiffer(config1, config2));
    config2.wifiApSubnet = config1.wifiApSubnet;

    config2.wifiStaConnectTimeoutMs = 20000;
    assert(networkSettingsDiffer(config1, config2));
    config2.wifiStaConnectTimeoutMs = config1.wifiStaConnectTimeoutMs;

    config2.debugControlLogs = !config1.debugControlLogs;
    assert(!networkSettingsDiffer(config1, config2));

    std::cout << "All tests passed for networkSettingsDiffer!" << std::endl;
}

int main() {
    testRestartSensitiveSettingsDiffer();
    testNetworkSettingsDiffer();
    return 0;
}
