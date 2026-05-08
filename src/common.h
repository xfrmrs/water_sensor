#ifndef WATER_SENSOR_COMMON_H
#define WATER_SENSOR_COMMON_H

#include <Arduino.h>
#include <string.h>
#include <Arduino_JSON.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <Hash.h>
#include <LittleFS.h>
#include <SimpleKalmanFilter.h>
#include <WebSocketsServer.h>

#include "ip_utils.h"

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
// Keep SAFE_GPIO_MASK aligned with SAFE_GPIO_VALUES.
static const uint8_t SAFE_GPIO_VALUES[] = {4, 5, 12, 13, 14, 16};
static const uint32_t SAFE_GPIO_MASK =
  (1UL << 4) | (1UL << 5) | (1UL << 12) | (1UL << 13) | (1UL << 14) | (1UL << 16);
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
  MEASUREMENT_FLAG_WATER_OUTPUT_ON = 0x04,
  MEASUREMENT_FLAG_EMERGENCY_STOP = 0x08
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
  bool enableApDhcp;
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
  unsigned long dnsPort;
  uint8_t wifiApChannel;
  bool wifiApHidden;
  uint8_t wifiApMaxConnections;
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
extern DNSServer *dnsServer;
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
extern bool emergencyStopActive;
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

bool beginFileSystem();
void printLittleFsInventory();
bool loadConfigFromFs();
bool loadConfigFromFs(String &errorMessage);
bool saveConfigToFs();
bool parseConfigFromRequestBody(Config &candidate, String &errorMessage);
bool validateConfig(const Config &candidate, String &errorMessage);
bool networkSettingsDiffer(const Config &left, const Config &right);
bool restartRequired();
String buildReconnectHint(const Config &targetConfig);
void printConfigSummary(const Config &source, const __FlashStringHelper *label);
void applyActiveRuntimeSettings(const Config &source);
void rebuildKalmanFilter();
void resetMeasurementState();
void initializePins();
void applyNetworkMode();
void printNetworkStatus();
void configureWebServer();
void configureWebSocket();
void servicePendingIo();
void serviceRuntime(unsigned long durationMs);
MeasurementSnapshot measureWaterLevel();
void applyMeasurementControl(MeasurementSnapshot &snapshot);
void pushHistory(const MeasurementSnapshot &snapshot);
void broadcastTelemetry(const MeasurementSnapshot &snapshot);
String buildTelemetryMessage(const MeasurementSnapshot &snapshot);
void writeMeasurementJsonObject(JsonOutput &output, const MeasurementSnapshot &snapshot);
void writeHistoryPointJsonObject(JsonOutput &output, const HistorySample &sample);
JsonOutput makeStringJsonOutput(String &target);
JsonOutput makeHttpJsonOutput();
void jsonWrite(JsonOutput &output, const char *value);
void jsonWrite(JsonOutput &output, const String &value);
void writeJsonFieldPrefix(JsonOutput &output, bool &first, const char *key);
void writeJsonStringField(JsonOutput &output, bool &first, const char *key, const char *value);
void writeJsonStringField(JsonOutput &output, bool &first, const char *key, const String &value);
void writeJsonBoolField(JsonOutput &output, bool &first, const char *key, bool value);
void writeJsonULongField(JsonOutput &output, bool &first, const char *key, unsigned long value);
void writeJsonUIntField(JsonOutput &output, bool &first, const char *key, unsigned int value);
void writeJsonFloatField(JsonOutput &output, bool &first, const char *key, float value, uint8_t decimals);
void writeJsonArrayStringValue(JsonOutput &output, bool &first, const char *value);
void writeJsonArrayULongValue(JsonOutput &output, bool &first, unsigned long value);
void writeConfigJsonObject(JsonOutput &output, const Config &source, bool includeSecrets, bool includePasswordFlags);
bool jsonVarToBool(const JSONVar &value, bool &parsedValue);
bool jsonVarToString(const JSONVar &value, String &parsedValue);
bool jsonVarToUnsignedLong(const JSONVar &value, unsigned long &parsedValue);
bool jsonVarToUint8(const JSONVar &value, uint8_t &parsedValue);
bool jsonVarToFloat(const JSONVar &value, float &parsedValue);
bool requireUnsignedLongField(const JSONVar &json, const char *name, unsigned long &target);
bool requireUint8Field(const JSONVar &json, const char *name, uint8_t &target);
bool requireStringField(const JSONVar &json, const char *name, String &target);
bool requireBoolField(const JSONVar &json, const char *name, bool &target);
bool requireFloatField(const JSONVar &json, const char *name, float &target);
bool optionalUnsignedLongField(const JSONVar &json, const char *name, unsigned long &target);
bool optionalUint8Field(const JSONVar &json, const char *name, uint8_t &target);
bool optionalStringField(const JSONVar &json, const char *name, String &target);
bool optionalBoolField(const JSONVar &json, const char *name, bool &target);
bool optionalFloatField(const JSONVar &json, const char *name, float &target);

void setWaterOutput(bool enabled);
void setErrorIndicator(bool error);
void activateEmergencyStop();
String buildStatusMessage();
void broadcastStatus();

#endif // WATER_SENSOR_COMMON_H
