#pragma once

#include "mock_arduino.h"

// Fake out Arduino includes
#define ESP8266WiFi_h
#define ESP8266WebServer_h
#define Hash_h
#define LittleFS_h
#define SimpleKalmanFilter_h
#define WebSocketsServer_h
#define ARDUINO_JSON_H

// Replace original common.h
#define WATER_SENSOR_COMMON_H

static const char *CONFIG_FILE_PATH = "/config.json";
static const char *CONFIG_TEMP_PATH = "/config.tmp";
static const uint32_t SAFE_GPIO_MASK = (1UL << 4) | (1UL << 5) | (1UL << 12) | (1UL << 13) | (1UL << 14) | (1UL << 16);
static const unsigned long SUPPORTED_SERIAL_BAUDS[] = {
  9600UL, 19200UL, 38400UL, 57600UL, 74880UL, 115200UL, 230400UL
};
static const size_t MAX_HISTORY_BUFFER_CAPACITY = 120;
static const uint8_t MAX_PING_BUFFER_CAPACITY = 12;

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

struct JsonOutput {
  void *context;
};

extern Config config;
extern Config bootConfig;
extern bool fileSystemReady;
extern ESP8266WebServer *server;
