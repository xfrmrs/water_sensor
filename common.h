#ifndef WATER_SENSOR_COMMON_H
#define WATER_SENSOR_COMMON_H

#include <Arduino.h>
#include <Arduino_JSON.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Hash.h>
#include <LittleFS.h>
#include <SimpleKalmanFilter.h>
#include <WebSocketsServer.h>

static const char *CONFIG_FILE_PATH = "/config.json";
static const char *CONFIG_TEMP_PATH = "/config.tmp";
static const char *INDEX_FILE_PATH = "/index.html";
static const char *APP_JS_FILE_PATH = "/app.js";
static const char *STYLE_CSS_FILE_PATH = "/style.css";
static const uint16_t BOOT_HTTP_PORT = 80;
static const uint16_t BOOT_WEBSOCKET_PORT = 81;
static const uint8_t MAX_PING_BUFFER_CAPACITY = 12;
static const size_t MAX_HISTORY_BUFFER_CAPACITY = 120;
static const unsigned long BOOT_SERIAL_BAUD = 74880UL;
// IMPORTANT: If you update SAFE_GPIO_VALUES, you must also update SAFE_GPIO_MASK.
static const uint8_t SAFE_GPIO_VALUES[] = {4, 5, 12, 13, 14, 16};
static const uint32_t SAFE_GPIO_MASK = (1UL << 4) | (1UL << 5) | (1UL << 12) | (1UL << 13) | (1UL << 14) | (1UL << 16);
static const unsigned long SUPPORTED_SERIAL_BAUDS[] = {
  9600UL,
  19200UL,
  38400UL,
  57600UL,
  74880UL,
  115200UL,
  230400UL
};

enum NetworkMode : uint8_t {
  NETWORK_MODE_SOFTAP,
  NETWORK_MODE_STA
};

enum MeasurementState : uint8_t {
  MEASUREMENT_STATE_ERROR,
  MEASUREMENT_STATE_HIGH,
  MEASUREMENT_STATE_NORMAL,
  MEASUREMENT_STATE_LOW
};

enum MeasurementFlags : uint8_t {
  MEASUREMENT_FLAG_VALID = 0x01,
  MEASUREMENT_FLAG_FILLING = 0x02,
  MEASUREMENT_FLAG_WATER_OUTPUT_ON = 0x04
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
  unsigned long wifiStaConnectTimeoutMs;
  unsigned long httpPort;
  unsigned long websocketPort;
  unsigned long historyCapacity;
  unsigned long maxConfigurablePings;
};

struct HistorySample {
  uint32_t rawUs;
  uint32_t acceptedUs;
  uint32_t filteredUs;
  uint32_t sampleMs;
};

struct MeasurementSnapshot {
  uint32_t rawUs;
  uint32_t acceptedUs;
  uint32_t filteredUs;
  uint32_t sampleMs;
  uint16_t rawCm;
  uint16_t acceptedCm;
  uint16_t filteredCm;
  MeasurementState state;
  uint8_t flags;
};

struct ConfigFieldDescriptor;
struct ConfigJsonFieldDescriptor;

typedef void (*JsonWriteCStringFn)(void *context, const char *value);
typedef void (*JsonWriteStringFn)(void *context, const String &value);

struct JsonOutput {
  void *context;
  JsonWriteCStringFn writeCString;
  JsonWriteStringFn writeString;
};

enum JsonFieldType : uint8_t {
  JSON_FIELD_BOOL,
  JSON_FIELD_UINT,
  JSON_FIELD_ULONG,
  JSON_FIELD_FLOAT,
  JSON_FIELD_STRING
};

extern Config config;
extern Config bootConfig;
extern ESP8266WebServer *server;
extern WebSocketsServer *webSocket;
extern SimpleKalmanFilter *kalmanFilter;
extern MeasurementSnapshot latestMeasurement;
extern HistorySample measurementHistory[MAX_HISTORY_BUFFER_CAPACITY];
extern uint8_t measurementHistoryCount;
extern uint8_t measurementHistoryHead;
extern int count;
extern bool filling;
extern bool fileSystemReady;
extern bool networkReady;
extern bool networkReconnectPending;
extern bool restartPending;
extern unsigned long networkReconnectAfterMs;
extern unsigned long restartAfterMs;
extern String uiStatusMessage;
extern String reconnectHint;
extern NetworkMode currentNetworkMode;
extern String currentNetworkIp;
extern uint8_t activeTrigPin;
extern uint8_t activeEchoPin;
extern uint8_t activeWaterPin;
extern uint8_t activeErrLedPin;
extern unsigned long activeSerialBaud;
extern unsigned long lastAcceptedUs;
extern unsigned long pendingShortUs;
extern uint8_t pendingShortCount;
extern uint8_t invalidBurstCount;

#endif // WATER_SENSOR_COMMON_H
