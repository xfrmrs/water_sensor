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

enum NetworkMode {
  NETWORK_MODE_SOFTAP,
  NETWORK_MODE_STA
};

enum MeasurementState {
  MEASUREMENT_STATE_ERROR,
  MEASUREMENT_STATE_HIGH,
  MEASUREMENT_STATE_NORMAL,
  MEASUREMENT_STATE_LOW
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

struct MeasurementSnapshot {
  unsigned long rawUs;
  unsigned long acceptedUs;
  unsigned long filteredUs;
  unsigned int rawCm;
  unsigned int acceptedCm;
  unsigned int filteredCm;
  bool valid;
  MeasurementState state;
  bool filling;
  bool waterOutputOn;
  unsigned long sampleMs;
};

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

MeasurementSnapshot latestMeasurement = {0, 0, 0, 0, 0, 0, false, MEASUREMENT_STATE_ERROR, false, false, 0};
MeasurementSnapshot measurementHistory[HISTORY_CAPACITY];
size_t measurementHistoryCount = 0;
size_t measurementHistoryHead = 0;

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

static inline unsigned int usToCm(unsigned long echoUs);
static inline unsigned long absDiffUs(unsigned long a, unsigned long b);
bool beginFileSystem();
bool loadConfigFromFs();
bool saveConfigToFs();
bool validateConfig(const Config &candidate, String &errorMessage);
void resetMeasurementState();
void rebuildKalmanFilter();
void applyActiveRuntimeSettings(const Config &source);
void initializePins();
void setWaterOutput(bool enabled);
void setErrorIndicator(bool error);
MeasurementSnapshot measureWaterLevel();
void applyMeasurementControl(MeasurementSnapshot &snapshot);
bool WaterLow(unsigned long waterLevelUs);
bool WaterHigh(unsigned long waterLevelUs);
bool connectToStationMode();
void startSoftApMode();
void applyNetworkMode();
void applyPendingNetworkChange();
void applyPendingRestart();
void serviceRuntime(unsigned long durationMs);
void configureWebServer();
void configureWebSocket();
void handleRoot();
void handleBootstrap();
void handleConfigSave();
void handleRestart();
void handleNotFound();
bool sendStaticFile(const char *path, const char *contentType);
String detectContentType(const String &path);
void printConfigSummary(const Config &source, const __FlashStringHelper *label);
void printNetworkStatus();
String currentNetworkModeName();
String currentNetworkName();
String measurementStateName(MeasurementState state);
bool restartRequired();
bool restartSensitiveSettingsDiffer(const Config &left, const Config &right);
bool networkSettingsDiffer(const Config &left, const Config &right);
String buildReconnectHint(const Config &targetConfig);
void pushHistory(const MeasurementSnapshot &snapshot);
String buildMeasurementJson(const MeasurementSnapshot &snapshot);
String buildHistoryPointJson(const MeasurementSnapshot &snapshot);
String buildHistoryJson();
String buildRestartFieldsJson();
String buildActiveRuntimeJson();
String buildOptionsJson();
String buildPublicConfigJson();
String buildPersistedConfigJson();
String buildStatusJson();
String buildBootstrapJson();
String buildConfigSaveResponseJson(bool ok);
String buildSimpleOkResponseJson(const String &message);
String buildErrorResponseJson(const String &message);
String buildWebSocketMessage(const char *type, const String &dataJson);
void broadcastStatus();
void broadcastTelemetry(const MeasurementSnapshot &snapshot);
void handleWebSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length);
bool parseConfigFromRequestBody(Config &candidate, String &errorMessage);
bool configFromJson(const JSONVar &json, Config &candidate);
bool jsonVarToUnsignedLong(const JSONVar &value, unsigned long &parsedValue);
bool jsonVarToUint8(const JSONVar &value, uint8_t &parsedValue);
bool jsonVarToString(const JSONVar &value, String &parsedValue);
bool jsonVarToBool(const JSONVar &value, bool &parsedValue);
bool jsonVarToFloat(const JSONVar &value, float &parsedValue);
bool parseIpAddressString(const String &value, IPAddress &parsedValue);
bool isSafePinValue(uint8_t pinValue);
bool arePinsUnique(const Config &candidate);
bool isSupportedBaud(unsigned long baudRate);
String jsonEscape(const String &value);
void appendJsonStringField(String &json, bool &first, const char *key, const String &value);
void appendJsonBoolField(String &json, bool &first, const char *key, bool value);
void appendJsonULongField(String &json, bool &first, const char *key, unsigned long value);
void appendJsonUIntField(String &json, bool &first, const char *key, unsigned int value);
void appendJsonFloatField(String &json, bool &first, const char *key, float value, uint8_t decimals);
void appendJsonRawField(String &json, bool &first, const char *key, const String &rawJson);
void appendJsonArrayStringValue(String &json, bool &first, const char *value);
void appendJsonArrayULongValue(String &json, bool &first, unsigned long value);
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

static inline unsigned int usToCm(unsigned long echoUs) {
  return (unsigned int)((echoUs + 29UL) / 58UL);
}

static inline unsigned long absDiffUs(unsigned long a, unsigned long b) {
  return (a >= b) ? (a - b) : (b - a);
}

bool beginFileSystem() {
  fileSystemReady = LittleFS.begin();
  return fileSystemReady;
}

bool jsonVarToUnsignedLong(const JSONVar &value, unsigned long &parsedValue) {
  String rendered = JSON.stringify(value);
  char *endPtr = nullptr;
  unsigned long parsed = strtoul(rendered.c_str(), &endPtr, 10);

  if (endPtr == rendered.c_str() || *endPtr != '\0') {
    return false;
  }

  parsedValue = parsed;
  return true;
}

bool jsonVarToUint8(const JSONVar &value, uint8_t &parsedValue) {
  unsigned long parsed = 0;
  if (!jsonVarToUnsignedLong(value, parsed) || parsed > 255UL) {
    return false;
  }

  parsedValue = (uint8_t)parsed;
  return true;
}

bool jsonVarToString(const JSONVar &value, String &parsedValue) {
  if (JSON.typeof(value) != "string") {
    return false;
  }

  parsedValue = String((const char *)value);
  return true;
}

bool jsonVarToBool(const JSONVar &value, bool &parsedValue) {
  String type = JSON.typeof(value);
  if (type == "boolean") {
    parsedValue = JSON.stringify(value) == "true";
    return true;
  }

  if (type == "number") {
    unsigned long parsed = 0;
    if (!jsonVarToUnsignedLong(value, parsed)) {
      return false;
    }

    parsedValue = parsed != 0;
    return true;
  }

  return false;
}

bool jsonVarToFloat(const JSONVar &value, float &parsedValue) {
  String rendered = JSON.stringify(value);
  char *endPtr = nullptr;
  float parsed = strtof(rendered.c_str(), &endPtr);

  if (endPtr == rendered.c_str() || *endPtr != '\0') {
    return false;
  }

  parsedValue = parsed;
  return true;
}

bool requireUnsignedLongField(const JSONVar &json, const char *name, unsigned long &target) {
  if (!json.hasOwnProperty(name)) {
    return false;
  }

  return jsonVarToUnsignedLong(json[name], target);
}

bool requireUint8Field(const JSONVar &json, const char *name, uint8_t &target) {
  if (!json.hasOwnProperty(name)) {
    return false;
  }

  return jsonVarToUint8(json[name], target);
}

bool requireStringField(const JSONVar &json, const char *name, String &target) {
  if (!json.hasOwnProperty(name)) {
    return false;
  }

  return jsonVarToString(json[name], target);
}

bool requireBoolField(const JSONVar &json, const char *name, bool &target) {
  if (!json.hasOwnProperty(name)) {
    return false;
  }

  return jsonVarToBool(json[name], target);
}

bool requireFloatField(const JSONVar &json, const char *name, float &target) {
  if (!json.hasOwnProperty(name)) {
    return false;
  }

  return jsonVarToFloat(json[name], target);
}

bool optionalUnsignedLongField(const JSONVar &json, const char *name, unsigned long &target) {
  if (!json.hasOwnProperty(name)) {
    return true;
  }

  return jsonVarToUnsignedLong(json[name], target);
}

bool optionalUint8Field(const JSONVar &json, const char *name, uint8_t &target) {
  if (!json.hasOwnProperty(name)) {
    return true;
  }

  return jsonVarToUint8(json[name], target);
}

bool optionalStringField(const JSONVar &json, const char *name, String &target) {
  if (!json.hasOwnProperty(name)) {
    return true;
  }

  return jsonVarToString(json[name], target);
}

bool optionalBoolField(const JSONVar &json, const char *name, bool &target) {
  if (!json.hasOwnProperty(name)) {
    return true;
  }

  return jsonVarToBool(json[name], target);
}

bool optionalFloatField(const JSONVar &json, const char *name, float &target) {
  if (!json.hasOwnProperty(name)) {
    return true;
  }

  return jsonVarToFloat(json[name], target);
}

bool configFromJson(const JSONVar &json, Config &candidate) {
  Config parsed = DEFAULT_CONFIG;

  if (!requireUnsignedLongField(json, "waterMaxDuration", parsed.waterMaxDuration)) return false;
  if (!requireUnsignedLongField(json, "loopDelayMs", parsed.loopDelayMs)) return false;
  if (!requireUnsignedLongField(json, "waterDelayMs", parsed.waterDelayMs)) return false;
  if (!requireUnsignedLongField(json, "waterLowUs", parsed.waterLowUs)) return false;
  if (!requireUnsignedLongField(json, "waterHighUs", parsed.waterHighUs)) return false;
  if (!requireUnsignedLongField(json, "waterErrUs", parsed.waterErrUs)) return false;
  if (!requireUnsignedLongField(json, "pulseTimeoutUs", parsed.pulseTimeoutUs)) return false;
  if (!requireUint8Field(json, "nPings", parsed.nPings)) return false;
  if (!requireUint8Field(json, "minValidPings", parsed.minValidPings)) return false;
  if (!requireUnsignedLongField(json, "pingGapMs", parsed.pingGapMs)) return false;
  if (!requireUnsignedLongField(json, "minValidEchoUs", parsed.minValidEchoUs)) return false;
  if (!requireUnsignedLongField(json, "shortJumpUs", parsed.shortJumpUs)) return false;
  if (!requireUnsignedLongField(json, "shortConfirmDeltaUs", parsed.shortConfirmDeltaUs)) return false;
  if (!requireUint8Field(json, "shortConfirmCount", parsed.shortConfirmCount)) return false;
  if (!requireUint8Field(json, "maxHeldInvalidBursts", parsed.maxHeldInvalidBursts)) return false;
  if (!requireStringField(json, "wifiStaSsid", parsed.wifiStaSsid)) return false;
  if (!requireStringField(json, "wifiStaPassword", parsed.wifiStaPassword)) return false;
  if (!requireStringField(json, "wifiApSsid", parsed.wifiApSsid)) return false;
  if (!requireStringField(json, "wifiApPassword", parsed.wifiApPassword)) return false;

  if (!optionalBoolField(json, "debugControlLogs", parsed.debugControlLogs)) return false;
  if (!optionalBoolField(json, "debugMeasurementLogs", parsed.debugMeasurementLogs)) return false;
  if (!optionalUint8Field(json, "trigPin", parsed.trigPin)) return false;
  if (!optionalUint8Field(json, "echoPin", parsed.echoPin)) return false;
  if (!optionalUint8Field(json, "waterPin", parsed.waterPin)) return false;
  if (!optionalUint8Field(json, "errLedPin", parsed.errLedPin)) return false;
  if (!optionalUnsignedLongField(json, "serialBaud", parsed.serialBaud)) return false;
  if (!optionalFloatField(json, "kalmanMeasurementError", parsed.kalmanMeasurementError)) return false;
  if (!optionalFloatField(json, "kalmanEstimateError", parsed.kalmanEstimateError)) return false;
  if (!optionalFloatField(json, "kalmanProcessNoise", parsed.kalmanProcessNoise)) return false;
  if (!optionalStringField(json, "wifiStaIp", parsed.wifiStaIp)) return false;
  if (!optionalStringField(json, "wifiStaGateway", parsed.wifiStaGateway)) return false;
  if (!optionalStringField(json, "wifiStaSubnet", parsed.wifiStaSubnet)) return false;
  if (!optionalStringField(json, "wifiApIp", parsed.wifiApIp)) return false;
  if (!optionalStringField(json, "wifiApGateway", parsed.wifiApGateway)) return false;
  if (!optionalStringField(json, "wifiApSubnet", parsed.wifiApSubnet)) return false;
  if (!optionalUnsignedLongField(json, "wifiStaConnectTimeoutMs", parsed.wifiStaConnectTimeoutMs)) return false;

  parsed.wifiStaSsid.trim();
  parsed.wifiApSsid.trim();
  parsed.wifiStaIp.trim();
  parsed.wifiStaGateway.trim();
  parsed.wifiStaSubnet.trim();
  parsed.wifiApIp.trim();
  parsed.wifiApGateway.trim();
  parsed.wifiApSubnet.trim();

  candidate = parsed;
  return true;
}

bool parseIpAddressString(const String &value, IPAddress &parsedValue) {
  IPAddress candidate;
  if (!candidate.fromString(value)) {
    return false;
  }

  parsedValue = candidate;
  return true;
}

bool isSafePinValue(uint8_t pinValue) {
  for (size_t i = 0; i < (sizeof(SAFE_GPIO_VALUES) / sizeof(SAFE_GPIO_VALUES[0])); ++i) {
    if (SAFE_GPIO_VALUES[i] == pinValue) {
      return true;
    }
  }

  return false;
}

bool arePinsUnique(const Config &candidate) {
  const uint8_t pins[] = {
    candidate.trigPin,
    candidate.echoPin,
    candidate.waterPin,
    candidate.errLedPin
  };

  for (size_t i = 0; i < 4; ++i) {
    for (size_t j = i + 1; j < 4; ++j) {
      if (pins[i] == pins[j]) {
        return false;
      }
    }
  }

  return true;
}

bool isSupportedBaud(unsigned long baudRate) {
  for (size_t i = 0; i < (sizeof(SUPPORTED_SERIAL_BAUDS) / sizeof(SUPPORTED_SERIAL_BAUDS[0])); ++i) {
    if (SUPPORTED_SERIAL_BAUDS[i] == baudRate) {
      return true;
    }
  }

  return false;
}

bool validateConfig(const Config &candidate, String &errorMessage) {
  IPAddress parsedIp;

  if (candidate.waterMaxDuration == 0) {
    errorMessage = F("Water max duration must be greater than 0.");
    return false;
  }

  if (candidate.loopDelayMs == 0 || candidate.waterDelayMs == 0) {
    errorMessage = F("Loop and water delays must be greater than 0.");
    return false;
  }

  if (candidate.waterHighUs >= candidate.waterLowUs) {
    errorMessage = F("The high threshold must be lower than the low threshold.");
    return false;
  }

  if (candidate.waterLowUs >= candidate.waterErrUs) {
    errorMessage = F("The low threshold must be lower than the error threshold.");
    return false;
  }

  if (candidate.pulseTimeoutUs < candidate.waterErrUs) {
    errorMessage = F("Pulse timeout must be at least the error threshold.");
    return false;
  }

  if (candidate.nPings == 0 || candidate.nPings > MAX_CONFIGURABLE_PINGS) {
    errorMessage = F("Ping count is out of range.");
    return false;
  }

  if (candidate.minValidPings == 0 || candidate.minValidPings > candidate.nPings) {
    errorMessage = F("Minimum valid pings must be between 1 and ping count.");
    return false;
  }

  if (candidate.pingGapMs == 0 || candidate.minValidEchoUs == 0) {
    errorMessage = F("Ping gap and minimum valid echo must be greater than 0.");
    return false;
  }

  if (candidate.shortConfirmCount == 0 || candidate.maxHeldInvalidBursts == 0) {
    errorMessage = F("Short confirm count and held invalid bursts must be greater than 0.");
    return false;
  }

  if (!isSafePinValue(candidate.trigPin) ||
      !isSafePinValue(candidate.echoPin) ||
      !isSafePinValue(candidate.waterPin) ||
      !isSafePinValue(candidate.errLedPin)) {
    errorMessage = F("Pin assignments must use the supported GPIO allowlist.");
    return false;
  }

  if (!arePinsUnique(candidate)) {
    errorMessage = F("Pin assignments must be unique.");
    return false;
  }

  if (!isSupportedBaud(candidate.serialBaud)) {
    errorMessage = F("Serial baud must match a supported rate.");
    return false;
  }

  if (candidate.kalmanMeasurementError <= 0.0f ||
      candidate.kalmanEstimateError <= 0.0f ||
      candidate.kalmanProcessNoise <= 0.0f) {
    errorMessage = F("Kalman filter values must be greater than 0.");
    return false;
  }

  if (candidate.wifiApSsid.length() == 0) {
    errorMessage = F("Setup AP SSID is required.");
    return false;
  }

  if (candidate.wifiApPassword.length() > 0 && candidate.wifiApPassword.length() < 8) {
    errorMessage = F("Setup AP password must be blank or at least 8 characters.");
    return false;
  }

  if (candidate.wifiStaConnectTimeoutMs == 0) {
    errorMessage = F("Wi-Fi station timeout must be greater than 0.");
    return false;
  }

  if (!parseIpAddressString(candidate.wifiStaIp, parsedIp) ||
      !parseIpAddressString(candidate.wifiStaGateway, parsedIp) ||
      !parseIpAddressString(candidate.wifiStaSubnet, parsedIp) ||
      !parseIpAddressString(candidate.wifiApIp, parsedIp) ||
      !parseIpAddressString(candidate.wifiApGateway, parsedIp) ||
      !parseIpAddressString(candidate.wifiApSubnet, parsedIp)) {
    errorMessage = F("IP, gateway, and subnet values must be valid dotted-quad IPv4 addresses.");
    return false;
  }

  return true;
}

bool loadConfigFromFs() {
  if (!fileSystemReady || !LittleFS.exists(CONFIG_FILE_PATH)) {
    return false;
  }

  File configFile = LittleFS.open(CONFIG_FILE_PATH, "r");
  if (!configFile) {
    return false;
  }

  String payload = configFile.readString();
  configFile.close();

  JSONVar json = JSON.parse(payload);
  if (JSON.typeof(json) == "undefined") {
    return false;
  }

  Config candidate = DEFAULT_CONFIG;
  String validationError;
  if (!configFromJson(json, candidate)) {
    return false;
  }

  if (!validateConfig(candidate, validationError)) {
    return false;
  }

  config = candidate;
  return true;
}

String buildPublicConfigJson() {
  String json;
  json.reserve(1800);
  json += '{';
  bool first = true;

  appendJsonBoolField(json, first, "debugControlLogs", config.debugControlLogs);
  appendJsonBoolField(json, first, "debugMeasurementLogs", config.debugMeasurementLogs);
  appendJsonULongField(json, first, "trigPin", config.trigPin);
  appendJsonULongField(json, first, "echoPin", config.echoPin);
  appendJsonULongField(json, first, "waterPin", config.waterPin);
  appendJsonULongField(json, first, "errLedPin", config.errLedPin);
  appendJsonULongField(json, first, "serialBaud", config.serialBaud);
  appendJsonFloatField(json, first, "kalmanMeasurementError", config.kalmanMeasurementError, 4);
  appendJsonFloatField(json, first, "kalmanEstimateError", config.kalmanEstimateError, 4);
  appendJsonFloatField(json, first, "kalmanProcessNoise", config.kalmanProcessNoise, 5);
  appendJsonULongField(json, first, "waterMaxDuration", config.waterMaxDuration);
  appendJsonULongField(json, first, "loopDelayMs", config.loopDelayMs);
  appendJsonULongField(json, first, "waterDelayMs", config.waterDelayMs);
  appendJsonULongField(json, first, "waterLowUs", config.waterLowUs);
  appendJsonULongField(json, first, "waterHighUs", config.waterHighUs);
  appendJsonULongField(json, first, "waterErrUs", config.waterErrUs);
  appendJsonULongField(json, first, "pulseTimeoutUs", config.pulseTimeoutUs);
  appendJsonULongField(json, first, "nPings", config.nPings);
  appendJsonULongField(json, first, "minValidPings", config.minValidPings);
  appendJsonULongField(json, first, "pingGapMs", config.pingGapMs);
  appendJsonULongField(json, first, "minValidEchoUs", config.minValidEchoUs);
  appendJsonULongField(json, first, "shortJumpUs", config.shortJumpUs);
  appendJsonULongField(json, first, "shortConfirmDeltaUs", config.shortConfirmDeltaUs);
  appendJsonULongField(json, first, "shortConfirmCount", config.shortConfirmCount);
  appendJsonULongField(json, first, "maxHeldInvalidBursts", config.maxHeldInvalidBursts);
  appendJsonStringField(json, first, "wifiStaSsid", config.wifiStaSsid);
  appendJsonStringField(json, first, "wifiApSsid", config.wifiApSsid);
  appendJsonStringField(json, first, "wifiStaIp", config.wifiStaIp);
  appendJsonStringField(json, first, "wifiStaGateway", config.wifiStaGateway);
  appendJsonStringField(json, first, "wifiStaSubnet", config.wifiStaSubnet);
  appendJsonStringField(json, first, "wifiApIp", config.wifiApIp);
  appendJsonStringField(json, first, "wifiApGateway", config.wifiApGateway);
  appendJsonStringField(json, first, "wifiApSubnet", config.wifiApSubnet);
  appendJsonULongField(json, first, "wifiStaConnectTimeoutMs", config.wifiStaConnectTimeoutMs);
  appendJsonBoolField(json, first, "hasStaPassword", config.wifiStaPassword.length() > 0);
  appendJsonBoolField(json, first, "hasApPassword", config.wifiApPassword.length() > 0);

  json += '}';
  return json;
}

String buildPersistedConfigJson() {
  String json;
  json.reserve(1900);
  json += '{';
  bool first = true;

  appendJsonBoolField(json, first, "debugControlLogs", config.debugControlLogs);
  appendJsonBoolField(json, first, "debugMeasurementLogs", config.debugMeasurementLogs);
  appendJsonULongField(json, first, "trigPin", config.trigPin);
  appendJsonULongField(json, first, "echoPin", config.echoPin);
  appendJsonULongField(json, first, "waterPin", config.waterPin);
  appendJsonULongField(json, first, "errLedPin", config.errLedPin);
  appendJsonULongField(json, first, "serialBaud", config.serialBaud);
  appendJsonFloatField(json, first, "kalmanMeasurementError", config.kalmanMeasurementError, 4);
  appendJsonFloatField(json, first, "kalmanEstimateError", config.kalmanEstimateError, 4);
  appendJsonFloatField(json, first, "kalmanProcessNoise", config.kalmanProcessNoise, 5);
  appendJsonULongField(json, first, "waterMaxDuration", config.waterMaxDuration);
  appendJsonULongField(json, first, "loopDelayMs", config.loopDelayMs);
  appendJsonULongField(json, first, "waterDelayMs", config.waterDelayMs);
  appendJsonULongField(json, first, "waterLowUs", config.waterLowUs);
  appendJsonULongField(json, first, "waterHighUs", config.waterHighUs);
  appendJsonULongField(json, first, "waterErrUs", config.waterErrUs);
  appendJsonULongField(json, first, "pulseTimeoutUs", config.pulseTimeoutUs);
  appendJsonULongField(json, first, "nPings", config.nPings);
  appendJsonULongField(json, first, "minValidPings", config.minValidPings);
  appendJsonULongField(json, first, "pingGapMs", config.pingGapMs);
  appendJsonULongField(json, first, "minValidEchoUs", config.minValidEchoUs);
  appendJsonULongField(json, first, "shortJumpUs", config.shortJumpUs);
  appendJsonULongField(json, first, "shortConfirmDeltaUs", config.shortConfirmDeltaUs);
  appendJsonULongField(json, first, "shortConfirmCount", config.shortConfirmCount);
  appendJsonULongField(json, first, "maxHeldInvalidBursts", config.maxHeldInvalidBursts);
  appendJsonStringField(json, first, "wifiStaSsid", config.wifiStaSsid);
  appendJsonStringField(json, first, "wifiStaPassword", config.wifiStaPassword);
  appendJsonStringField(json, first, "wifiApSsid", config.wifiApSsid);
  appendJsonStringField(json, first, "wifiApPassword", config.wifiApPassword);
  appendJsonStringField(json, first, "wifiStaIp", config.wifiStaIp);
  appendJsonStringField(json, first, "wifiStaGateway", config.wifiStaGateway);
  appendJsonStringField(json, first, "wifiStaSubnet", config.wifiStaSubnet);
  appendJsonStringField(json, first, "wifiApIp", config.wifiApIp);
  appendJsonStringField(json, first, "wifiApGateway", config.wifiApGateway);
  appendJsonStringField(json, first, "wifiApSubnet", config.wifiApSubnet);
  appendJsonULongField(json, first, "wifiStaConnectTimeoutMs", config.wifiStaConnectTimeoutMs);
  json += '}';
  return json;
}

bool saveConfigToFs() {
  if (!fileSystemReady) {
    return false;
  }

  File configFile = LittleFS.open(CONFIG_TEMP_PATH, "w");
  if (!configFile) {
    return false;
  }

  String payload = buildPersistedConfigJson();

  size_t bytesWritten = configFile.print(payload);
  configFile.close();

  if (bytesWritten != payload.length()) {
    LittleFS.remove(CONFIG_TEMP_PATH);
    return false;
  }

  if (LittleFS.exists(CONFIG_FILE_PATH) && !LittleFS.remove(CONFIG_FILE_PATH)) {
    LittleFS.remove(CONFIG_TEMP_PATH);
    return false;
  }

  if (!LittleFS.rename(CONFIG_TEMP_PATH, CONFIG_FILE_PATH)) {
    LittleFS.remove(CONFIG_TEMP_PATH);
    return false;
  }

  printConfigSummary(config, F("Saved config to LittleFS"));
  return true;
}

void resetMeasurementState() {
  lastAcceptedUs = 0;
  pendingShortUs = 0;
  pendingShortCount = 0;
  invalidBurstCount = 0;
  count = 0;
  filling = false;
}

void rebuildKalmanFilter() {
  if (kalmanFilter != nullptr) {
    delete kalmanFilter;
    kalmanFilter = nullptr;
  }

  kalmanFilter = new SimpleKalmanFilter(
    config.kalmanMeasurementError,
    config.kalmanEstimateError,
    config.kalmanProcessNoise
  );
}

void applyActiveRuntimeSettings(const Config &source) {
  activeTrigPin = source.trigPin;
  activeEchoPin = source.echoPin;
  activeWaterPin = source.waterPin;
  activeErrLedPin = source.errLedPin;
  activeSerialBaud = source.serialBaud;
}

void initializePins() {
  pinMode(activeWaterPin, OUTPUT);
  pinMode(activeErrLedPin, OUTPUT);
  pinMode(activeTrigPin, OUTPUT);
  pinMode(activeEchoPin, INPUT);

  setWaterOutput(false);
  setErrorIndicator(false);
  digitalWrite(activeTrigPin, LOW);
}

void setWaterOutput(bool enabled) {
  digitalWrite(activeWaterPin, enabled ? LOW : HIGH);
}

void setErrorIndicator(bool error) {
  digitalWrite(activeErrLedPin, error ? LOW : HIGH);
}

bool restartSensitiveSettingsDiffer(const Config &left, const Config &right) {
  return left.trigPin != right.trigPin ||
         left.echoPin != right.echoPin ||
         left.waterPin != right.waterPin ||
         left.errLedPin != right.errLedPin ||
         left.serialBaud != right.serialBaud;
}

bool networkSettingsDiffer(const Config &left, const Config &right) {
  return left.wifiStaSsid != right.wifiStaSsid ||
         left.wifiStaPassword != right.wifiStaPassword ||
         left.wifiApSsid != right.wifiApSsid ||
         left.wifiApPassword != right.wifiApPassword ||
         left.wifiStaIp != right.wifiStaIp ||
         left.wifiStaGateway != right.wifiStaGateway ||
         left.wifiStaSubnet != right.wifiStaSubnet ||
         left.wifiApIp != right.wifiApIp ||
         left.wifiApGateway != right.wifiApGateway ||
         left.wifiApSubnet != right.wifiApSubnet ||
         left.wifiStaConnectTimeoutMs != right.wifiStaConnectTimeoutMs;
}

bool restartRequired() {
  return restartSensitiveSettingsDiffer(bootConfig, config);
}

String currentNetworkModeName() {
  return currentNetworkMode == NETWORK_MODE_STA ? String(F("Local Wi-Fi")) : String(F("Setup AP"));
}

String currentNetworkName() {
  return currentNetworkMode == NETWORK_MODE_STA ? config.wifiStaSsid : config.wifiApSsid;
}

String measurementStateName(MeasurementState state) {
  switch (state) {
    case MEASUREMENT_STATE_HIGH:
      return F("HIGH");
    case MEASUREMENT_STATE_NORMAL:
      return F("NORMAL");
    case MEASUREMENT_STATE_LOW:
      return F("LOW");
    case MEASUREMENT_STATE_ERROR:
    default:
      return F("ERROR");
  }
}

String buildReconnectHint(const Config &targetConfig) {
  String hint = F("Reconnect to ");
  hint += targetConfig.wifiStaIp;
  hint += F(" if the station join succeeds, or to setup AP ");
  hint += targetConfig.wifiApSsid;
  hint += F(" at ");
  hint += targetConfig.wifiApIp;
  hint += F(" if it falls back.");
  return hint;
}

void printConfigSummary(const Config &source, const __FlashStringHelper *label) {
  Serial.println(label);
  Serial.print(F("  debugControlLogs="));
  Serial.println(source.debugControlLogs ? F("true") : F("false"));
  Serial.print(F("  debugMeasurementLogs="));
  Serial.println(source.debugMeasurementLogs ? F("true") : F("false"));
  Serial.print(F("  pins trig/echo/water/err="));
  Serial.print(source.trigPin);
  Serial.print('/');
  Serial.print(source.echoPin);
  Serial.print('/');
  Serial.print(source.waterPin);
  Serial.print('/');
  Serial.println(source.errLedPin);
  Serial.print(F("  serialBaud="));
  Serial.println(source.serialBaud);
  Serial.print(F("  water thresholds high/low/err="));
  Serial.print(source.waterHighUs);
  Serial.print('/');
  Serial.print(source.waterLowUs);
  Serial.print('/');
  Serial.println(source.waterErrUs);
  Serial.print(F("  wifiStaSsid="));
  Serial.println(source.wifiStaSsid);
  Serial.print(F("  wifiStaIp="));
  Serial.println(source.wifiStaIp);
  Serial.print(F("  wifiApSsid="));
  Serial.println(source.wifiApSsid);
  Serial.print(F("  wifiApIp="));
  Serial.println(source.wifiApIp);
}

void printNetworkStatus() {
  Serial.print(F("Network mode: "));
  Serial.println(currentNetworkModeName());
  Serial.print(F("Network name: "));
  Serial.println(currentNetworkName());
  Serial.print(F("Network ready: "));
  Serial.println(networkReady ? F("yes") : F("no"));
  Serial.print(F("Network IP: "));
  Serial.println(currentNetworkIp);
}

bool connectToStationMode() {
  if (config.wifiStaSsid.length() == 0) {
    return false;
  }

  IPAddress staIp;
  IPAddress staGateway;
  IPAddress staSubnet;
  if (!parseIpAddressString(config.wifiStaIp, staIp) ||
      !parseIpAddressString(config.wifiStaGateway, staGateway) ||
      !parseIpAddressString(config.wifiStaSubnet, staSubnet)) {
    return false;
  }

  WiFi.softAPdisconnect(true);
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.config(staIp, staGateway, staSubnet);
  WiFi.begin(config.wifiStaSsid.c_str(), config.wifiStaPassword.c_str());

  unsigned long startedAt = millis();
  while ((WiFi.status() != WL_CONNECTED) &&
         ((millis() - startedAt) < config.wifiStaConnectTimeoutMs)) {
    delay(250);
    yield();
  }

  if (WiFi.status() != WL_CONNECTED) {
    WiFi.disconnect(true);
    networkReady = false;
    currentNetworkIp = "";
    return false;
  }

  networkReady = true;
  currentNetworkMode = NETWORK_MODE_STA;
  currentNetworkIp = WiFi.localIP().toString();
  return true;
}

void startSoftApMode() {
  IPAddress apIp;
  IPAddress apGateway;
  IPAddress apSubnet;
  if (!parseIpAddressString(config.wifiApIp, apIp) ||
      !parseIpAddressString(config.wifiApGateway, apGateway) ||
      !parseIpAddressString(config.wifiApSubnet, apSubnet)) {
    apIp.fromString(DEFAULT_AP_IP);
    apGateway.fromString(DEFAULT_AP_GATEWAY);
    apSubnet.fromString(DEFAULT_AP_SUBNET);
  }

  WiFi.softAPdisconnect(true);
  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIp, apGateway, apSubnet);

  const char *apPassword = config.wifiApPassword.length() > 0 ? config.wifiApPassword.c_str() : nullptr;
  networkReady = WiFi.softAP(config.wifiApSsid.c_str(), apPassword);
  currentNetworkMode = NETWORK_MODE_SOFTAP;
  currentNetworkIp = WiFi.softAPIP().toString();
}

void applyNetworkMode() {
  if (!connectToStationMode()) {
    startSoftApMode();
  }
}

void applyPendingNetworkChange() {
  if (!networkReconnectPending) {
    return;
  }

  long msUntilReconnect = (long)(networkReconnectAfterMs - millis());
  if (msUntilReconnect > 0) {
    return;
  }

  networkReconnectPending = false;
  applyNetworkMode();
  printNetworkStatus();
  server.stop();
  server.begin();

  if (currentNetworkMode == NETWORK_MODE_STA) {
    uiStatusMessage = String(F("Network settings applied. Reconnect using the device LAN IP: ")) + currentNetworkIp;
  } else {
    uiStatusMessage = String(F("LAN join failed. Reconnect to the setup AP '")) +
                      config.wifiApSsid + F("' at ") + currentNetworkIp;
  }

  reconnectHint = "";
  broadcastStatus();
}

void applyPendingRestart() {
  if (!restartPending) {
    return;
  }

  long msUntilRestart = (long)(restartAfterMs - millis());
  if (msUntilRestart > 0) {
    return;
  }

  delay(50);
  ESP.restart();
}

void serviceRuntime(unsigned long durationMs) {
  unsigned long startedAt = millis();
  while ((millis() - startedAt) < durationMs) {
    server.handleClient();
    webSocket.loop();
    applyPendingNetworkChange();
    applyPendingRestart();

    unsigned long elapsed = millis() - startedAt;
    unsigned long remaining = durationMs > elapsed ? (durationMs - elapsed) : 0;
    unsigned long sliceMs = remaining > 25 ? 25 : remaining;
    if (sliceMs == 0) {
      break;
    }

    delay(sliceMs);
    yield();
  }
}

static unsigned long readEchoUsOnce() {
  digitalWrite(activeTrigPin, LOW);
  delayMicroseconds(5);
  digitalWrite(activeTrigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(activeTrigPin, LOW);
  return pulseIn(activeEchoPin, HIGH, config.pulseTimeoutUs);
}

static void sortEchoSamples(unsigned long *samples, uint8_t countSamples) {
  for (uint8_t i = 1; i < countSamples; ++i) {
    unsigned long key = samples[i];
    int8_t j = (int8_t)i - 1;
    while ((j >= 0) && (samples[j] > key)) {
      samples[j + 1] = samples[j];
      --j;
    }
    samples[j + 1] = key;
  }
}

static unsigned long readMedianEchoUs() {
  unsigned long samples[MAX_CONFIGURABLE_PINGS];
  uint8_t validCount = 0;

  for (uint8_t i = 0; i < config.nPings; ++i) {
    unsigned long echoUs = readEchoUsOnce();

    if ((echoUs >= config.minValidEchoUs) && (echoUs <= config.waterErrUs)) {
      samples[validCount++] = echoUs;
    }

    if (i + 1 < config.nPings) {
      delay(config.pingGapMs);
      yield();
    }
  }

  if (validCount < config.minValidPings) {
    return 0;
  }

  sortEchoSamples(samples, validCount);
  return samples[validCount / 2];
}

static unsigned long deglitchShortEchoUs(unsigned long candidateUs) {
  if (candidateUs == 0) {
    pendingShortCount = 0;
    ++invalidBurstCount;

    if ((lastAcceptedUs > 0) && (invalidBurstCount <= config.maxHeldInvalidBursts)) {
      return lastAcceptedUs;
    }

    return config.waterErrUs;
  }

  invalidBurstCount = 0;

  if (lastAcceptedUs == 0) {
    lastAcceptedUs = candidateUs;
    pendingShortCount = 0;
    return candidateUs;
  }

  if ((candidateUs + config.shortJumpUs) < lastAcceptedUs) {
    if ((pendingShortCount > 0) &&
        (absDiffUs(candidateUs, pendingShortUs) <= config.shortConfirmDeltaUs)) {
      ++pendingShortCount;
    } else {
      pendingShortUs = candidateUs;
      pendingShortCount = 1;
    }

    if (pendingShortCount < config.shortConfirmCount) {
      return lastAcceptedUs;
    }
  }

  pendingShortCount = 0;
  lastAcceptedUs = candidateUs;
  return candidateUs;
}

MeasurementSnapshot measureWaterLevel() {
  MeasurementSnapshot snapshot = {0, 0, 0, 0, 0, 0, false, MEASUREMENT_STATE_ERROR, filling, false, millis()};
  unsigned long measuredUs = readMedianEchoUs();
  unsigned long acceptedUs = deglitchShortEchoUs(measuredUs);
  unsigned long filteredUs = acceptedUs;

  if (acceptedUs < config.waterErrUs && kalmanFilter != nullptr) {
    filteredUs = (unsigned long)(kalmanFilter->updateEstimate((float)acceptedUs) + 0.5f);
  }

  snapshot.rawUs = measuredUs;
  snapshot.acceptedUs = acceptedUs;
  snapshot.filteredUs = filteredUs;
  snapshot.rawCm = usToCm(measuredUs);
  snapshot.acceptedCm = usToCm(acceptedUs);
  snapshot.filteredCm = usToCm(filteredUs);
  snapshot.valid = acceptedUs < config.waterErrUs;

  if (!snapshot.valid) {
    snapshot.state = MEASUREMENT_STATE_ERROR;
  } else if (WaterHigh(snapshot.filteredUs)) {
    snapshot.state = MEASUREMENT_STATE_HIGH;
  } else if (WaterLow(snapshot.filteredUs)) {
    snapshot.state = MEASUREMENT_STATE_LOW;
  } else {
    snapshot.state = MEASUREMENT_STATE_NORMAL;
  }

  if (config.debugMeasurementLogs) {
    Serial.print(F("Measured: "));
    Serial.print(snapshot.rawUs);
    Serial.print(F(" us ("));
    Serial.print(snapshot.rawCm);
    Serial.print(F(" cm), Accepted: "));
    Serial.print(snapshot.acceptedUs);
    Serial.print(F(" us ("));
    Serial.print(snapshot.acceptedCm);
    Serial.print(F(" cm), Filtered: "));
    Serial.print(snapshot.filteredUs);
    Serial.print(F(" us ("));
    Serial.print(snapshot.filteredCm);
    Serial.print(F(" cm), Valid: "));
    Serial.println(snapshot.valid ? F("yes") : F("no"));
  }

  return snapshot;
}

void applyMeasurementControl(MeasurementSnapshot &snapshot) {
  if (!snapshot.valid) {
    if (config.debugControlLogs) {
      Serial.println(F("water reading invalid, STOP water"));
    }

    setWaterOutput(false);
    setErrorIndicator(true);
    filling = false;
    count = 0;
    snapshot.filling = false;
    snapshot.waterOutputOn = false;
    snapshot.state = MEASUREMENT_STATE_ERROR;
    return;
  }

  setErrorIndicator(false);

  if (WaterHigh(snapshot.filteredUs)) {
    if (config.debugControlLogs) {
      Serial.println(F("water level is HIGH, STOP water..."));
    }

    setWaterOutput(false);
    filling = false;
    count = 0;
    snapshot.filling = false;
    snapshot.waterOutputOn = false;
    snapshot.state = MEASUREMENT_STATE_HIGH;
    return;
  }

  if (WaterLow(snapshot.filteredUs) || filling) {
    filling = true;
    if (config.debugControlLogs) {
      Serial.println(F("water low, FILLING water"));
    }

    setWaterOutput(true);
    snapshot.filling = true;
    snapshot.waterOutputOn = true;
    snapshot.state = MEASUREMENT_STATE_LOW;

    if (count++ > (int)config.waterMaxDuration) {
      setWaterOutput(false);
      setErrorIndicator(true);
      filling = false;
      count = 0;
      snapshot.filling = false;
      snapshot.waterOutputOn = false;

      if (config.debugControlLogs) {
        Serial.print(F("water has been ON for too long, "));
        Serial.println(F("emergency STOP, water off"));
      }
    }

    return;
  }

  if (config.debugControlLogs) {
    Serial.println(F("water level is NORMAL, STOP water..."));
  }

  setWaterOutput(false);
  filling = false;
  count = 0;
  snapshot.filling = false;
  snapshot.waterOutputOn = false;
  snapshot.state = MEASUREMENT_STATE_NORMAL;
}

bool WaterLow(unsigned long waterLevelUs) {
  return (waterLevelUs >= config.waterLowUs) && (waterLevelUs < config.waterErrUs);
}

bool WaterHigh(unsigned long waterLevelUs) {
  return (waterLevelUs <= config.waterHighUs);
}

void pushHistory(const MeasurementSnapshot &snapshot) {
  size_t writeIndex = (measurementHistoryHead + measurementHistoryCount) % HISTORY_CAPACITY;
  if (measurementHistoryCount == HISTORY_CAPACITY) {
    writeIndex = measurementHistoryHead;
    measurementHistoryHead = (measurementHistoryHead + 1) % HISTORY_CAPACITY;
  } else {
    ++measurementHistoryCount;
  }

  measurementHistory[writeIndex] = snapshot;
}

String jsonEscape(const String &value) {
  String escaped;
  escaped.reserve(value.length() + 8);

  for (size_t i = 0; i < value.length(); ++i) {
    char current = value.charAt(i);
    switch (current) {
      case '\\':
        escaped += F("\\\\");
        break;
      case '"':
        escaped += F("\\\"");
        break;
      case '\n':
        escaped += F("\\n");
        break;
      case '\r':
        escaped += F("\\r");
        break;
      case '\t':
        escaped += F("\\t");
        break;
      default:
        escaped += current;
        break;
    }
  }

  return escaped;
}

void appendJsonStringField(String &json, bool &first, const char *key, const String &value) {
  if (!first) {
    json += ',';
  }
  first = false;
  json += '"';
  json += key;
  json += F("\":\"");
  json += jsonEscape(value);
  json += '"';
}

void appendJsonBoolField(String &json, bool &first, const char *key, bool value) {
  if (!first) {
    json += ',';
  }
  first = false;
  json += '"';
  json += key;
  json += F("\":");
  json += value ? F("true") : F("false");
}

void appendJsonULongField(String &json, bool &first, const char *key, unsigned long value) {
  if (!first) {
    json += ',';
  }
  first = false;
  json += '"';
  json += key;
  json += F("\":");
  json += String(value);
}

void appendJsonUIntField(String &json, bool &first, const char *key, unsigned int value) {
  appendJsonULongField(json, first, key, (unsigned long)value);
}

void appendJsonFloatField(String &json, bool &first, const char *key, float value, uint8_t decimals) {
  if (!first) {
    json += ',';
  }
  first = false;
  json += '"';
  json += key;
  json += F("\":");
  json += String(value, decimals);
}

void appendJsonRawField(String &json, bool &first, const char *key, const String &rawJson) {
  if (!first) {
    json += ',';
  }
  first = false;
  json += '"';
  json += key;
  json += F("\":");
  json += rawJson;
}

void appendJsonArrayStringValue(String &json, bool &first, const char *value) {
  if (!first) {
    json += ',';
  }
  first = false;
  json += '"';
  json += value;
  json += '"';
}

void appendJsonArrayULongValue(String &json, bool &first, unsigned long value) {
  if (!first) {
    json += ',';
  }
  first = false;
  json += String(value);
}

String buildMeasurementJson(const MeasurementSnapshot &snapshot) {
  String json;
  json.reserve(320);
  json += '{';
  bool first = true;
  appendJsonULongField(json, first, "rawUs", snapshot.rawUs);
  appendJsonULongField(json, first, "acceptedUs", snapshot.acceptedUs);
  appendJsonULongField(json, first, "filteredUs", snapshot.filteredUs);
  appendJsonUIntField(json, first, "rawCm", snapshot.rawCm);
  appendJsonUIntField(json, first, "acceptedCm", snapshot.acceptedCm);
  appendJsonUIntField(json, first, "filteredCm", snapshot.filteredCm);
  appendJsonBoolField(json, first, "valid", snapshot.valid);
  appendJsonStringField(json, first, "state", measurementStateName(snapshot.state));
  appendJsonBoolField(json, first, "filling", snapshot.filling);
  appendJsonBoolField(json, first, "waterOutputOn", snapshot.waterOutputOn);
  appendJsonULongField(json, first, "sampleMs", snapshot.sampleMs);
  json += '}';
  return json;
}

String buildHistoryPointJson(const MeasurementSnapshot &snapshot) {
  String json;
  json.reserve(120);
  json += '{';
  bool first = true;
  appendJsonULongField(json, first, "rawUs", snapshot.rawUs);
  appendJsonULongField(json, first, "acceptedUs", snapshot.acceptedUs);
  appendJsonULongField(json, first, "filteredUs", snapshot.filteredUs);
  appendJsonULongField(json, first, "sampleMs", snapshot.sampleMs);
  json += '}';
  return json;
}

String buildHistoryJson() {
  String json;
  json.reserve(80 + (measurementHistoryCount * 120));
  json += '[';
  bool first = true;

  for (size_t i = 0; i < measurementHistoryCount; ++i) {
    size_t index = (measurementHistoryHead + i) % HISTORY_CAPACITY;
    if (!first) {
      json += ',';
    }
    first = false;
    json += buildHistoryPointJson(measurementHistory[index]);
  }

  json += ']';
  return json;
}

String buildRestartFieldsJson() {
  String json;
  json.reserve(96);
  json += '[';
  bool first = true;

  if (bootConfig.trigPin != config.trigPin) appendJsonArrayStringValue(json, first, "trigPin");
  if (bootConfig.echoPin != config.echoPin) appendJsonArrayStringValue(json, first, "echoPin");
  if (bootConfig.waterPin != config.waterPin) appendJsonArrayStringValue(json, first, "waterPin");
  if (bootConfig.errLedPin != config.errLedPin) appendJsonArrayStringValue(json, first, "errLedPin");
  if (bootConfig.serialBaud != config.serialBaud) appendJsonArrayStringValue(json, first, "serialBaud");

  json += ']';
  return json;
}

String buildActiveRuntimeJson() {
  String json;
  json.reserve(160);
  json += '{';
  bool first = true;
  appendJsonULongField(json, first, "trigPin", activeTrigPin);
  appendJsonULongField(json, first, "echoPin", activeEchoPin);
  appendJsonULongField(json, first, "waterPin", activeWaterPin);
  appendJsonULongField(json, first, "errLedPin", activeErrLedPin);
  appendJsonULongField(json, first, "serialBaud", activeSerialBaud);
  json += '}';
  return json;
}

String buildOptionsJson() {
  String pinArray = "[";
  bool firstPin = true;
  for (size_t i = 0; i < (sizeof(SAFE_GPIO_VALUES) / sizeof(SAFE_GPIO_VALUES[0])); ++i) {
    appendJsonArrayULongValue(pinArray, firstPin, SAFE_GPIO_VALUES[i]);
  }
  pinArray += ']';

  String baudArray = "[";
  bool firstBaud = true;
  for (size_t i = 0; i < (sizeof(SUPPORTED_SERIAL_BAUDS) / sizeof(SUPPORTED_SERIAL_BAUDS[0])); ++i) {
    appendJsonArrayULongValue(baudArray, firstBaud, SUPPORTED_SERIAL_BAUDS[i]);
  }
  baudArray += ']';

  String json;
  json.reserve(256);
  json += '{';
  bool first = true;
  appendJsonRawField(json, first, "safePins", pinArray);
  appendJsonRawField(json, first, "supportedBauds", baudArray);
  appendJsonULongField(json, first, "historyCapacity", HISTORY_CAPACITY);
  appendJsonULongField(json, first, "maxConfigurablePings", MAX_CONFIGURABLE_PINGS);
  json += '}';
  return json;
}

String buildStatusJson() {
  String json;
  json.reserve(1200);
  json += '{';
  bool first = true;
  appendJsonStringField(json, first, "message", uiStatusMessage);
  appendJsonStringField(json, first, "reconnectHint", reconnectHint);
  appendJsonStringField(json, first, "networkMode", currentNetworkModeName());
  appendJsonStringField(json, first, "networkName", currentNetworkName());
  appendJsonStringField(json, first, "networkIp", currentNetworkIp);
  appendJsonBoolField(json, first, "networkReady", networkReady);
  appendJsonBoolField(json, first, "restartRequired", restartRequired());
  appendJsonBoolField(json, first, "restartPending", restartPending);
  appendJsonRawField(json, first, "restartFields", buildRestartFieldsJson());
  appendJsonRawField(json, first, "activeRuntime", buildActiveRuntimeJson());
  appendJsonRawField(json, first, "latestMeasurement", buildMeasurementJson(latestMeasurement));
  appendJsonULongField(json, first, "historyCount", measurementHistoryCount);
  json += '}';
  return json;
}

String buildBootstrapJson() {
  String json;
  json.reserve(12000);
  json += '{';
  bool first = true;
  appendJsonRawField(json, first, "config", buildPublicConfigJson());
  appendJsonRawField(json, first, "status", buildStatusJson());
  appendJsonRawField(json, first, "history", buildHistoryJson());
  appendJsonRawField(json, first, "options", buildOptionsJson());
  json += '}';
  return json;
}

String buildConfigSaveResponseJson(bool ok) {
  String json;
  json.reserve(4000);
  json += '{';
  bool first = true;
  appendJsonBoolField(json, first, "ok", ok);
  appendJsonBoolField(json, first, "restartRequired", restartRequired());
  appendJsonRawField(json, first, "restartFields", buildRestartFieldsJson());
  appendJsonStringField(json, first, "reconnectHint", reconnectHint);
  appendJsonRawField(json, first, "config", buildPublicConfigJson());
  appendJsonRawField(json, first, "status", buildStatusJson());
  json += '}';
  return json;
}

String buildSimpleOkResponseJson(const String &message) {
  String json;
  json.reserve(512);
  json += '{';
  bool first = true;
  appendJsonBoolField(json, first, "ok", true);
  appendJsonStringField(json, first, "message", message);
  appendJsonRawField(json, first, "status", buildStatusJson());
  json += '}';
  return json;
}

String buildErrorResponseJson(const String &message) {
  String json;
  json.reserve(256);
  json += '{';
  bool first = true;
  appendJsonBoolField(json, first, "ok", false);
  appendJsonStringField(json, first, "message", message);
  json += '}';
  return json;
}

String buildWebSocketMessage(const char *type, const String &dataJson) {
  String json;
  json.reserve(dataJson.length() + 48);
  json += F("{\"type\":\"");
  json += type;
  json += F("\",\"data\":");
  json += dataJson;
  json += '}';
  return json;
}

void broadcastStatus() {
  String payload = buildWebSocketMessage("status", buildStatusJson());
  webSocket.broadcastTXT(payload);
}

void broadcastTelemetry(const MeasurementSnapshot &snapshot) {
  String payload = buildWebSocketMessage("telemetry", buildMeasurementJson(snapshot));
  webSocket.broadcastTXT(payload);
}

void handleWebSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  (void)payload;
  (void)length;

  if (type == WStype_CONNECTED) {
    String payload = buildWebSocketMessage("status", buildStatusJson());
    webSocket.sendTXT(num, payload);
  }
}

bool sendStaticFile(const char *path, const char *contentType) {
  if (!fileSystemReady || !LittleFS.exists(path)) {
    return false;
  }

  File file = LittleFS.open(path, "r");
  if (!file) {
    return false;
  }

  server.sendHeader("Cache-Control", "no-store");
  server.streamFile(file, contentType);
  file.close();
  return true;
}

String detectContentType(const String &path) {
  if (path.endsWith(F(".html"))) return F("text/html");
  if (path.endsWith(F(".css"))) return F("text/css");
  if (path.endsWith(F(".js"))) return F("application/javascript");
  if (path.endsWith(F(".json"))) return F("application/json");
  if (path.endsWith(F(".svg"))) return F("image/svg+xml");
  if (path.endsWith(F(".png"))) return F("image/png");
  return F("text/plain");
}

bool parseConfigFromRequestBody(Config &candidate, String &errorMessage) {
  String body = server.arg("plain");
  if (body.length() == 0) {
    errorMessage = F("Request body is empty.");
    return false;
  }

  JSONVar json = JSON.parse(body);
  if (JSON.typeof(json) == "undefined") {
    errorMessage = F("Invalid JSON request body.");
    return false;
  }

  Config parsed = config;
  bool clearStaPassword = false;
  bool clearApPassword = false;
  String staPasswordInput = "";
  String apPasswordInput = "";

  if (!requireBoolField(json, "debugControlLogs", parsed.debugControlLogs)) {
    errorMessage = F("Missing or invalid field: debugControlLogs");
    return false;
  }
  if (!requireBoolField(json, "debugMeasurementLogs", parsed.debugMeasurementLogs)) {
    errorMessage = F("Missing or invalid field: debugMeasurementLogs");
    return false;
  }
  if (!requireUint8Field(json, "trigPin", parsed.trigPin)) {
    errorMessage = F("Missing or invalid field: trigPin");
    return false;
  }
  if (!requireUint8Field(json, "echoPin", parsed.echoPin)) {
    errorMessage = F("Missing or invalid field: echoPin");
    return false;
  }
  if (!requireUint8Field(json, "waterPin", parsed.waterPin)) {
    errorMessage = F("Missing or invalid field: waterPin");
    return false;
  }
  if (!requireUint8Field(json, "errLedPin", parsed.errLedPin)) {
    errorMessage = F("Missing or invalid field: errLedPin");
    return false;
  }
  if (!requireUnsignedLongField(json, "serialBaud", parsed.serialBaud)) {
    errorMessage = F("Missing or invalid field: serialBaud");
    return false;
  }
  if (!requireFloatField(json, "kalmanMeasurementError", parsed.kalmanMeasurementError)) {
    errorMessage = F("Missing or invalid field: kalmanMeasurementError");
    return false;
  }
  if (!requireFloatField(json, "kalmanEstimateError", parsed.kalmanEstimateError)) {
    errorMessage = F("Missing or invalid field: kalmanEstimateError");
    return false;
  }
  if (!requireFloatField(json, "kalmanProcessNoise", parsed.kalmanProcessNoise)) {
    errorMessage = F("Missing or invalid field: kalmanProcessNoise");
    return false;
  }
  if (!requireUnsignedLongField(json, "waterMaxDuration", parsed.waterMaxDuration)) {
    errorMessage = F("Missing or invalid field: waterMaxDuration");
    return false;
  }
  if (!requireUnsignedLongField(json, "loopDelayMs", parsed.loopDelayMs)) {
    errorMessage = F("Missing or invalid field: loopDelayMs");
    return false;
  }
  if (!requireUnsignedLongField(json, "waterDelayMs", parsed.waterDelayMs)) {
    errorMessage = F("Missing or invalid field: waterDelayMs");
    return false;
  }
  if (!requireUnsignedLongField(json, "waterLowUs", parsed.waterLowUs)) {
    errorMessage = F("Missing or invalid field: waterLowUs");
    return false;
  }
  if (!requireUnsignedLongField(json, "waterHighUs", parsed.waterHighUs)) {
    errorMessage = F("Missing or invalid field: waterHighUs");
    return false;
  }
  if (!requireUnsignedLongField(json, "waterErrUs", parsed.waterErrUs)) {
    errorMessage = F("Missing or invalid field: waterErrUs");
    return false;
  }
  if (!requireUnsignedLongField(json, "pulseTimeoutUs", parsed.pulseTimeoutUs)) {
    errorMessage = F("Missing or invalid field: pulseTimeoutUs");
    return false;
  }
  if (!requireUint8Field(json, "nPings", parsed.nPings)) {
    errorMessage = F("Missing or invalid field: nPings");
    return false;
  }
  if (!requireUint8Field(json, "minValidPings", parsed.minValidPings)) {
    errorMessage = F("Missing or invalid field: minValidPings");
    return false;
  }
  if (!requireUnsignedLongField(json, "pingGapMs", parsed.pingGapMs)) {
    errorMessage = F("Missing or invalid field: pingGapMs");
    return false;
  }
  if (!requireUnsignedLongField(json, "minValidEchoUs", parsed.minValidEchoUs)) {
    errorMessage = F("Missing or invalid field: minValidEchoUs");
    return false;
  }
  if (!requireUnsignedLongField(json, "shortJumpUs", parsed.shortJumpUs)) {
    errorMessage = F("Missing or invalid field: shortJumpUs");
    return false;
  }
  if (!requireUnsignedLongField(json, "shortConfirmDeltaUs", parsed.shortConfirmDeltaUs)) {
    errorMessage = F("Missing or invalid field: shortConfirmDeltaUs");
    return false;
  }
  if (!requireUint8Field(json, "shortConfirmCount", parsed.shortConfirmCount)) {
    errorMessage = F("Missing or invalid field: shortConfirmCount");
    return false;
  }
  if (!requireUint8Field(json, "maxHeldInvalidBursts", parsed.maxHeldInvalidBursts)) {
    errorMessage = F("Missing or invalid field: maxHeldInvalidBursts");
    return false;
  }
  if (!requireStringField(json, "wifiStaSsid", parsed.wifiStaSsid)) {
    errorMessage = F("Missing or invalid field: wifiStaSsid");
    return false;
  }
  if (!requireStringField(json, "wifiApSsid", parsed.wifiApSsid)) {
    errorMessage = F("Missing or invalid field: wifiApSsid");
    return false;
  }
  if (!requireStringField(json, "wifiStaIp", parsed.wifiStaIp)) {
    errorMessage = F("Missing or invalid field: wifiStaIp");
    return false;
  }
  if (!requireStringField(json, "wifiStaGateway", parsed.wifiStaGateway)) {
    errorMessage = F("Missing or invalid field: wifiStaGateway");
    return false;
  }
  if (!requireStringField(json, "wifiStaSubnet", parsed.wifiStaSubnet)) {
    errorMessage = F("Missing or invalid field: wifiStaSubnet");
    return false;
  }
  if (!requireStringField(json, "wifiApIp", parsed.wifiApIp)) {
    errorMessage = F("Missing or invalid field: wifiApIp");
    return false;
  }
  if (!requireStringField(json, "wifiApGateway", parsed.wifiApGateway)) {
    errorMessage = F("Missing or invalid field: wifiApGateway");
    return false;
  }
  if (!requireStringField(json, "wifiApSubnet", parsed.wifiApSubnet)) {
    errorMessage = F("Missing or invalid field: wifiApSubnet");
    return false;
  }
  if (!requireUnsignedLongField(json, "wifiStaConnectTimeoutMs", parsed.wifiStaConnectTimeoutMs)) {
    errorMessage = F("Missing or invalid field: wifiStaConnectTimeoutMs");
    return false;
  }

  if (json.hasOwnProperty("clearStaPassword") && !jsonVarToBool(json["clearStaPassword"], clearStaPassword)) {
    errorMessage = F("Invalid field: clearStaPassword");
    return false;
  }
  if (json.hasOwnProperty("clearApPassword") && !jsonVarToBool(json["clearApPassword"], clearApPassword)) {
    errorMessage = F("Invalid field: clearApPassword");
    return false;
  }
  if (json.hasOwnProperty("wifiStaPassword") && !jsonVarToString(json["wifiStaPassword"], staPasswordInput)) {
    errorMessage = F("Invalid field: wifiStaPassword");
    return false;
  }
  if (json.hasOwnProperty("wifiApPassword") && !jsonVarToString(json["wifiApPassword"], apPasswordInput)) {
    errorMessage = F("Invalid field: wifiApPassword");
    return false;
  }

  parsed.wifiStaSsid.trim();
  parsed.wifiApSsid.trim();
  parsed.wifiStaIp.trim();
  parsed.wifiStaGateway.trim();
  parsed.wifiStaSubnet.trim();
  parsed.wifiApIp.trim();
  parsed.wifiApGateway.trim();
  parsed.wifiApSubnet.trim();

  if (staPasswordInput.length() > 0) {
    parsed.wifiStaPassword = staPasswordInput;
  } else if (clearStaPassword) {
    parsed.wifiStaPassword = "";
  }

  if (apPasswordInput.length() > 0) {
    parsed.wifiApPassword = apPasswordInput;
  } else if (clearApPassword) {
    parsed.wifiApPassword = "";
  }

  candidate = parsed;
  return true;
}

void handleRoot() {
  if (!sendStaticFile(INDEX_FILE_PATH, "text/html")) {
    server.send(500, "text/plain", "Dashboard assets are unavailable.");
  }
}

void handleBootstrap() {
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", buildBootstrapJson());
}

void handleConfigSave() {
  Config previousConfig = config;
  Config candidate = config;
  String errorMessage;

  if (!parseConfigFromRequestBody(candidate, errorMessage)) {
    server.send(400, "application/json", buildErrorResponseJson(errorMessage));
    return;
  }

  if (!validateConfig(candidate, errorMessage)) {
    server.send(400, "application/json", buildErrorResponseJson(errorMessage));
    return;
  }

  bool networkChanged = networkSettingsDiffer(previousConfig, candidate);
  config = candidate;
  rebuildKalmanFilter();
  resetMeasurementState();

  if (!saveConfigToFs()) {
    config = previousConfig;
    rebuildKalmanFilter();
    resetMeasurementState();
    server.send(500, "application/json", buildErrorResponseJson(F("Settings could not be saved to LittleFS.")));
    return;
  }

  reconnectHint = networkChanged ? buildReconnectHint(config) : "";

  if (networkChanged) {
    uiStatusMessage = F("Settings saved. Network settings will be applied shortly.");
    networkReconnectPending = true;
    networkReconnectAfterMs = millis() + 1000UL;
    server.sendHeader("Connection", "close");
  } else if (restartRequired()) {
    uiStatusMessage = F("Settings saved. Restart required for low-level changes.");
  } else {
    uiStatusMessage = F("Settings saved.");
  }

  String payload = buildConfigSaveResponseJson(true);
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", payload);
  broadcastStatus();
}

void handleRestart() {
  uiStatusMessage = F("Restart requested. The device is rebooting.");
  restartPending = true;
  restartAfterMs = millis() + 750UL;
  broadcastStatus();
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", buildSimpleOkResponseJson(uiStatusMessage));
}

void handleNotFound() {
  String path = server.uri();
  String contentType = detectContentType(path);
  if (sendStaticFile(path.c_str(), contentType.c_str())) {
    return;
  }

  server.send(404, "application/json", buildErrorResponseJson(F("Not found")));
}

void configureWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/app.js", HTTP_GET, []() {
    if (!sendStaticFile(APP_JS_FILE_PATH, "application/javascript")) {
      server.send(404, "text/plain", "app.js not found");
    }
  });
  server.on("/style.css", HTTP_GET, []() {
    if (!sendStaticFile(STYLE_CSS_FILE_PATH, "text/css")) {
      server.send(404, "text/plain", "style.css not found");
    }
  });
  server.on("/api/bootstrap", HTTP_GET, handleBootstrap);
  server.on("/api/config", HTTP_POST, handleConfigSave);
  server.on("/api/restart", HTTP_POST, handleRestart);
  server.onNotFound(handleNotFound);
  server.begin();
}

void configureWebSocket() {
  webSocket.begin();
  webSocket.onEvent(handleWebSocketEvent);
}

void setup() {
  bool configLoaded = false;
  String validationError;

  beginFileSystem();
  configLoaded = loadConfigFromFs();

  if (!configLoaded || !validateConfig(config, validationError)) {
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
    config = DEFAULT_CONFIG;
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
  server.handleClient();
  webSocket.loop();
  applyPendingNetworkChange();
  applyPendingRestart();

  serviceRuntime(filling ? config.waterDelayMs : config.loopDelayMs);

  server.handleClient();
  webSocket.loop();
  applyPendingNetworkChange();
  applyPendingRestart();

  MeasurementSnapshot snapshot = measureWaterLevel();
  applyMeasurementControl(snapshot);
  latestMeasurement = snapshot;
  pushHistory(snapshot);
  broadcastTelemetry(snapshot);
}
