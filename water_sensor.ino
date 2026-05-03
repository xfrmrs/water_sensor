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
static const uint16_t HTTP_PORT = 80;
static const uint16_t WEBSOCKET_PORT = 81;
static const uint8_t MAX_CONFIGURABLE_PINGS = 12;
static const size_t HISTORY_CAPACITY = 120;
static const unsigned long DEFAULT_SERIAL_BAUD = 74880UL;
static const unsigned long DEFAULT_WIFI_STA_CONNECT_TIMEOUT_MS = 15000UL;
static const char *DEFAULT_STA_IP = "10.0.0.47";
static const char *DEFAULT_STA_GATEWAY = "10.0.0.1";
static const char *DEFAULT_STA_SUBNET = "255.0.0.0";
static const char *DEFAULT_AP_IP = "10.0.0.47";
static const char *DEFAULT_AP_GATEWAY = "10.0.0.47";
static const char *DEFAULT_AP_SUBNET = "255.0.0.0";
static const uint8_t SAFE_GPIO_VALUES[] = {4, 5, 12, 13, 14, 16};
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
  String wifiApSsid;
  String wifiApPassword;
  String wifiStaIp;
  String wifiStaGateway;
  String wifiStaSubnet;
  String wifiApIp;
  String wifiApGateway;
  String wifiApSubnet;
  unsigned long wifiStaConnectTimeoutMs;
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

typedef void (*JsonWriteCStringFn)(void *context, const char *value);
typedef void (*JsonWriteStringFn)(void *context, const String &value);

struct JsonOutput {
  void *context;
  JsonWriteCStringFn writeCString;
  JsonWriteStringFn writeString;
};

JsonOutput makeStringJsonOutput(String &target);
JsonOutput makeHttpJsonOutput();
void jsonWrite(JsonOutput &output, const char *value);
void jsonWrite(JsonOutput &output, const String &value);
void writeJsonFieldPrefix(JsonOutput &output, bool &first, const char *key);
void writeJsonStringField(JsonOutput &output, bool &first, const char *key, const char *value);
void writeJsonStringField(JsonOutput &output, bool &first, const char *key, const String &value);
void writeJsonBoolField(JsonOutput &output, bool &first, const char *key, bool value);
void writeJsonULongField(JsonOutput &output, bool &first, const char *key, unsigned long value);
void writeJsonFloatField(JsonOutput &output, bool &first, const char *key, float value, uint8_t decimals);
void writeJsonArrayStringValue(JsonOutput &output, bool &first, const char *value);
void writeJsonArrayULongValue(JsonOutput &output, bool &first, unsigned long value);
void writeConfigJsonObject(JsonOutput &output, const Config &source, bool includeSecrets, bool includePasswordFlags);
void writeMeasurementJsonObject(JsonOutput &output, const MeasurementSnapshot &snapshot);

const Config DEFAULT_CONFIG = {
  false,
  true,
  4,
  5,
  13,
  16,
  DEFAULT_SERIAL_BAUD,
  5.0f,
  2.0f,
  0.01f,
  90UL,
  1000UL,
  1500UL,
  1200UL,
  500UL,
  2500UL,
  5000UL,
  5,
  3,
  60UL,
  232UL,
  250UL,
  120UL,
  2,
  3,
  "",
  "",
  "WaterSensorSetup",
  "",
  DEFAULT_STA_IP,
  DEFAULT_STA_GATEWAY,
  DEFAULT_STA_SUBNET,
  DEFAULT_AP_IP,
  DEFAULT_AP_GATEWAY,
  DEFAULT_AP_SUBNET,
  DEFAULT_WIFI_STA_CONNECT_TIMEOUT_MS
};

Config config = DEFAULT_CONFIG;
Config bootConfig = DEFAULT_CONFIG;

ESP8266WebServer server(HTTP_PORT);
WebSocketsServer webSocket(WEBSOCKET_PORT);
SimpleKalmanFilter *kalmanFilter = nullptr;

MeasurementSnapshot latestMeasurement = {0, 0, 0, 0, 0, 0, 0, MEASUREMENT_STATE_ERROR, 0};
HistorySample measurementHistory[HISTORY_CAPACITY];
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
String uiStatusMessage = "";
String reconnectHint = "";
NetworkMode currentNetworkMode = NETWORK_MODE_SOFTAP;
String currentNetworkIp = "";

uint8_t activeTrigPin = DEFAULT_CONFIG.trigPin;
uint8_t activeEchoPin = DEFAULT_CONFIG.echoPin;
uint8_t activeWaterPin = DEFAULT_CONFIG.waterPin;
uint8_t activeErrLedPin = DEFAULT_CONFIG.errLedPin;
unsigned long activeSerialBaud = DEFAULT_CONFIG.serialBaud;

unsigned long lastAcceptedUs = 0;
unsigned long pendingShortUs = 0;
uint8_t pendingShortCount = 0;
uint8_t invalidBurstCount = 0;

void setup() {
  bool configLoaded = false;

  beginFileSystem();
  configLoaded = loadConfigFromFs();

  if (!configLoaded) {
    config = DEFAULT_CONFIG;
  }

  Serial.begin(config.serialBaud);
  delay(10);

  if (fileSystemReady) {
    Serial.println(F("LittleFS mounted"));
  } else {
    Serial.println(F("LittleFS mount failed; using in-memory config only"));
  }

  if (!configLoaded) {
    Serial.println(F("Config missing or invalid; restoring built-in defaults"));
    if (fileSystemReady && !saveConfigToFs()) {
      Serial.println(F("Failed to write default config to LittleFS"));
    }
  }

  bootConfig = config;
  applyActiveRuntimeSettings(config);
  rebuildKalmanFilter();
  resetMeasurementState();
  initializePins();
  printConfigSummary(config, F("Active runtime config"));

  if (config.debugControlLogs) {
    Serial.println(F("Config runtime initialized"));
  }

  applyNetworkMode();
  reconnectHint = "";
  uiStatusMessage = F("Device ready.");
  printNetworkStatus();
  configureWebServer();
  configureWebSocket();
}

void loop() {
  servicePendingIo();
  serviceRuntime(filling ? config.waterDelayMs : config.loopDelayMs);
  servicePendingIo();

  MeasurementSnapshot snapshot = measureWaterLevel();
  applyMeasurementControl(snapshot);
  latestMeasurement = snapshot;
  pushHistory(snapshot);
  broadcastTelemetry(snapshot);
}
