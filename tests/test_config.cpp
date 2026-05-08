#include <cassert>
#include <iostream>
#include <string>

#define TEST_SUPPORT_IMPLEMENTATION
#include "test_support.h"

void writeEscapedJsonString(JsonOutput &output, const String &value);

#include "../json_helpers.ino"
#include "../src/ip_utils.cpp"
#include "../config.ino"

static Config makeValidConfig() {
  Config candidate = {};
  candidate.debugControlLogs = false;
  candidate.debugMeasurementLogs = false;
  candidate.trigPin = 4;
  candidate.echoPin = 5;
  candidate.waterPin = 12;
  candidate.errLedPin = 13;
  candidate.serialBaud = 115200;
  candidate.kalmanMeasurementError = 1.0f;
  candidate.kalmanEstimateError = 1.0f;
  candidate.kalmanProcessNoise = 0.1f;
  candidate.waterMaxDuration = 10;
  candidate.loopDelayMs = 10;
  candidate.waterDelayMs = 10;
  candidate.waterHighUs = 100;
  candidate.waterLowUs = 200;
  candidate.waterErrUs = 300;
  candidate.pulseTimeoutUs = 400;
  candidate.nPings = 5;
  candidate.minValidPings = 3;
  candidate.pingGapMs = 10;
  candidate.minValidEchoUs = 10;
  candidate.shortJumpUs = 10;
  candidate.shortConfirmDeltaUs = 5;
  candidate.shortConfirmCount = 2;
  candidate.maxHeldInvalidBursts = 2;
  candidate.wifiStaSsid = "HomeNet";
  candidate.wifiStaPassword = "station-pass";
  candidate.enableStationDhcp = false;
  candidate.enableApDhcp = true;
  candidate.wifiApSsid = "WaterSensorSetup";
  candidate.wifiApPassword = "setup-pass";
  candidate.wifiStaIp = "192.168.1.50";
  candidate.wifiStaGateway = "192.168.1.1";
  candidate.wifiStaSubnet = "255.255.255.0";
  candidate.wifiApIp = "10.0.0.47";
  candidate.wifiApGateway = "10.0.0.47";
  candidate.wifiApSubnet = "255.0.0.0";
  candidate.adminPassword = "admin";
  candidate.wifiStaConnectTimeoutMs = 15000;
  candidate.httpPort = 80;
  candidate.websocketPort = 81;
  candidate.dnsPort = 53;
  candidate.wifiApChannel = 6;
  candidate.wifiApHidden = false;
  candidate.wifiApMaxConnections = 4;
  candidate.historyCapacity = 50;
  candidate.maxConfigurablePings = 12;
  return candidate;
}

static void testValidateConfig() {
  String error;
  Config candidate = makeValidConfig();
  assert(validateConfig(candidate, error));

  candidate = makeValidConfig();
  candidate.adminPassword = "";
  assert(validateConfig(candidate, error));

  candidate = makeValidConfig();
  candidate.adminPassword = "123";
  assert(!validateConfig(candidate, error));

  candidate = makeValidConfig();
  candidate.dnsPort = 0;
  assert(!validateConfig(candidate, error));

  candidate = makeValidConfig();
  candidate.wifiApChannel = 14;
  assert(!validateConfig(candidate, error));

  candidate = makeValidConfig();
  candidate.enableStationDhcp = false;
  candidate.wifiStaIp = "";
  assert(!validateConfig(candidate, error));

  candidate = makeValidConfig();
  candidate.enableStationDhcp = true;
  candidate.wifiStaIp = "";
  candidate.wifiStaGateway = "";
  candidate.wifiStaSubnet = "";
  assert(validateConfig(candidate, error));
}

static void testNetworkSettingsDiffer() {
  Config left = makeValidConfig();
  Config right = left;
  assert(!networkSettingsDiffer(left, right));

  right.dnsPort = 54;
  assert(networkSettingsDiffer(left, right));

  right = left;
  right.adminPassword = "changed";
  assert(!networkSettingsDiffer(left, right));
}

static void testReconnectHint() {
  Config candidate = makeValidConfig();
  String hint = buildReconnectHint(candidate);
  assert(std::string(hint.c_str()).find("192.168.1.50") != std::string::npos);
  assert(std::string(hint.c_str()).find("WaterSensorSetup") != std::string::npos);

  candidate.enableStationDhcp = true;
  hint = buildReconnectHint(candidate);
  assert(std::string(hint.c_str()).find("DHCP address") != std::string::npos);
  assert(std::string(hint.c_str()).find("HomeNet") != std::string::npos);
}

int main() {
  testValidateConfig();
  testNetworkSettingsDiffer();
  testReconnectHint();

  std::cout << "config tests passed\n";
  return 0;
}
