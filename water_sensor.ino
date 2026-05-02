#include <Arduino.h>
#include <Arduino_JSON.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include <SimpleKalmanFilter.h>

// Error condition constants
#define WATER_MAX_DURATION 90
#define DEBUG false
#define DEBUG_M true

//pin assignments 
#define TRIG 4    // yellow
#define ECHO 5    // green
#define WATER 13  // white w/black stripe
#define ERRLED 16 // (on-board)

static const uint8_t MAX_CONFIGURABLE_PINGS = 12;
static const char *CONFIG_FILE_PATH = "/config.json";
static const char *CONFIG_TEMP_PATH = "/config.tmp";
static const unsigned long WIFI_STA_CONNECT_TIMEOUT_MS = 15000UL;

struct Config {
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
};

const Config DEFAULT_CONFIG = {
  90,
  1000,
  1500,
  1200UL,
  500UL,
  2500UL,
  5000UL,
  5,
  3,
  60,
  232UL,
  250UL,
  120UL,
  2,
  3,
  "",
  "",
  "WaterSensorSetup",
  ""
};

// Global variables
Config config = DEFAULT_CONFIG;

int count = 0;
bool filling = false;
bool fileSystemReady = false;
bool networkReady = false;

enum NetworkMode {
  NETWORK_MODE_SOFTAP,
  NETWORK_MODE_STA
};

NetworkMode currentNetworkMode = NETWORK_MODE_SOFTAP;
String currentNetworkIp = "";

unsigned long lastAcceptedUs = 0;
unsigned long pendingShortUs = 0;
uint8_t pendingShortCount = 0;
uint8_t invalidBurstCount = 0;

/*
 SimpleKalmanFilter(e_mea, e_est, q);
 e_mea: Measurement Uncertainty
 e_est: Estimation Uncertainty
 q: Process Noise
*/
SimpleKalmanFilter simpleKalmanFilter(5, 2, 0.01);

// Function declarations
unsigned long WaterLevel();
bool WaterLow(unsigned long waterLevelUs);
bool WaterHigh(unsigned long waterLevelUs);
void resetMeasurementState();
bool validateConfig(const Config &candidate);
bool beginFileSystem();
bool loadConfigFromFs();
bool saveConfigToFs();
bool configFromJson(const JSONVar &json, Config &candidate);
JSONVar configToJson(const Config &source);
bool jsonVarToUnsignedLong(const JSONVar &value, unsigned long &parsedValue);
bool jsonVarToUint8(const JSONVar &value, uint8_t &parsedValue);
bool jsonVarToString(const JSONVar &value, String &parsedValue);
bool connectToStationMode();
void applyNetworkMode();
void startSoftApMode();

void resetMeasurementState() {
  lastAcceptedUs = 0;
  pendingShortUs = 0;
  pendingShortCount = 0;
  invalidBurstCount = 0;
}

bool validateConfig(const Config &candidate) {
  if (candidate.waterMaxDuration == 0) return false;
  if (candidate.loopDelayMs == 0) return false;
  if (candidate.waterDelayMs == 0) return false;
  if (candidate.waterHighUs >= candidate.waterLowUs) return false;
  if (candidate.waterLowUs >= candidate.waterErrUs) return false;
  if (candidate.pulseTimeoutUs < candidate.waterErrUs) return false;
  if (candidate.nPings == 0 || candidate.nPings > MAX_CONFIGURABLE_PINGS) return false;
  if (candidate.minValidPings == 0 || candidate.minValidPings > candidate.nPings) return false;
  if (candidate.pingGapMs == 0) return false;
  if (candidate.minValidEchoUs == 0) return false;
  if (candidate.shortConfirmCount == 0) return false;
  if (candidate.maxHeldInvalidBursts == 0) return false;
  if (candidate.wifiApSsid.length() == 0) return false;
  if (candidate.wifiApPassword.length() > 0 && candidate.wifiApPassword.length() < 8) return false;
  return true;
}

bool beginFileSystem() {
  fileSystemReady = LittleFS.begin();

  if (fileSystemReady) {
    Serial.println(F("LittleFS mounted"));
  } else {
    Serial.println(F("LittleFS mount failed; using in-memory config only"));
  }

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

bool configFromJson(const JSONVar &json, Config &candidate) {
  if (!json.hasOwnProperty("waterMaxDuration")) return false;
  if (!json.hasOwnProperty("loopDelayMs")) return false;
  if (!json.hasOwnProperty("waterDelayMs")) return false;
  if (!json.hasOwnProperty("waterLowUs")) return false;
  if (!json.hasOwnProperty("waterHighUs")) return false;
  if (!json.hasOwnProperty("waterErrUs")) return false;
  if (!json.hasOwnProperty("pulseTimeoutUs")) return false;
  if (!json.hasOwnProperty("nPings")) return false;
  if (!json.hasOwnProperty("minValidPings")) return false;
  if (!json.hasOwnProperty("pingGapMs")) return false;
  if (!json.hasOwnProperty("minValidEchoUs")) return false;
  if (!json.hasOwnProperty("shortJumpUs")) return false;
  if (!json.hasOwnProperty("shortConfirmDeltaUs")) return false;
  if (!json.hasOwnProperty("shortConfirmCount")) return false;
  if (!json.hasOwnProperty("maxHeldInvalidBursts")) return false;
  if (!json.hasOwnProperty("wifiStaSsid")) return false;
  if (!json.hasOwnProperty("wifiStaPassword")) return false;
  if (!json.hasOwnProperty("wifiApSsid")) return false;
  if (!json.hasOwnProperty("wifiApPassword")) return false;

  if (!jsonVarToUnsignedLong(json["waterMaxDuration"], candidate.waterMaxDuration)) return false;
  if (!jsonVarToUnsignedLong(json["loopDelayMs"], candidate.loopDelayMs)) return false;
  if (!jsonVarToUnsignedLong(json["waterDelayMs"], candidate.waterDelayMs)) return false;
  if (!jsonVarToUnsignedLong(json["waterLowUs"], candidate.waterLowUs)) return false;
  if (!jsonVarToUnsignedLong(json["waterHighUs"], candidate.waterHighUs)) return false;
  if (!jsonVarToUnsignedLong(json["waterErrUs"], candidate.waterErrUs)) return false;
  if (!jsonVarToUnsignedLong(json["pulseTimeoutUs"], candidate.pulseTimeoutUs)) return false;
  if (!jsonVarToUint8(json["nPings"], candidate.nPings)) return false;
  if (!jsonVarToUint8(json["minValidPings"], candidate.minValidPings)) return false;
  if (!jsonVarToUnsignedLong(json["pingGapMs"], candidate.pingGapMs)) return false;
  if (!jsonVarToUnsignedLong(json["minValidEchoUs"], candidate.minValidEchoUs)) return false;
  if (!jsonVarToUnsignedLong(json["shortJumpUs"], candidate.shortJumpUs)) return false;
  if (!jsonVarToUnsignedLong(json["shortConfirmDeltaUs"], candidate.shortConfirmDeltaUs)) return false;
  if (!jsonVarToUint8(json["shortConfirmCount"], candidate.shortConfirmCount)) return false;
  if (!jsonVarToUint8(json["maxHeldInvalidBursts"], candidate.maxHeldInvalidBursts)) return false;
  if (!jsonVarToString(json["wifiStaSsid"], candidate.wifiStaSsid)) return false;
  if (!jsonVarToString(json["wifiStaPassword"], candidate.wifiStaPassword)) return false;
  if (!jsonVarToString(json["wifiApSsid"], candidate.wifiApSsid)) return false;
  if (!jsonVarToString(json["wifiApPassword"], candidate.wifiApPassword)) return false;

  return true;
}

JSONVar configToJson(const Config &source) {
  JSONVar json;
  json["waterMaxDuration"] = source.waterMaxDuration;
  json["loopDelayMs"] = source.loopDelayMs;
  json["waterDelayMs"] = source.waterDelayMs;
  json["waterLowUs"] = source.waterLowUs;
  json["waterHighUs"] = source.waterHighUs;
  json["waterErrUs"] = source.waterErrUs;
  json["pulseTimeoutUs"] = source.pulseTimeoutUs;
  json["nPings"] = source.nPings;
  json["minValidPings"] = source.minValidPings;
  json["pingGapMs"] = source.pingGapMs;
  json["minValidEchoUs"] = source.minValidEchoUs;
  json["shortJumpUs"] = source.shortJumpUs;
  json["shortConfirmDeltaUs"] = source.shortConfirmDeltaUs;
  json["shortConfirmCount"] = source.shortConfirmCount;
  json["maxHeldInvalidBursts"] = source.maxHeldInvalidBursts;
  json["wifiStaSsid"] = source.wifiStaSsid;
  json["wifiStaPassword"] = source.wifiStaPassword;
  json["wifiApSsid"] = source.wifiApSsid;
  json["wifiApPassword"] = source.wifiApPassword;
  return json;
}

bool loadConfigFromFs() {
  if (!fileSystemReady) {
    return false;
  }

  if (!LittleFS.exists(CONFIG_FILE_PATH)) {
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

  Config candidate = config;
  if (!configFromJson(json, candidate)) {
    return false;
  }

  if (!validateConfig(candidate)) {
    return false;
  }

  config = candidate;
  resetMeasurementState();
  return true;
}

bool saveConfigToFs() {
  if (!fileSystemReady) {
    return false;
  }

  File configFile = LittleFS.open(CONFIG_TEMP_PATH, "w");
  if (!configFile) {
    return false;
  }

  String payload = JSON.stringify(configToJson(config));
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

  return true;
}

bool connectToStationMode() {
  if (config.wifiStaSsid.length() == 0) {
    return false;
  }

  WiFi.softAPdisconnect(true);
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.begin(config.wifiStaSsid.c_str(), config.wifiStaPassword.c_str());

  unsigned long startedAt = millis();
  while ((WiFi.status() != WL_CONNECTED) &&
         ((millis() - startedAt) < WIFI_STA_CONNECT_TIMEOUT_MS)) {
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
  WiFi.softAPdisconnect(true);
  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP);

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

static inline unsigned int usToCm(unsigned long echoUs) {
  return (unsigned int)((echoUs + 29UL) / 58UL);
}

static inline unsigned long absDiffUs(unsigned long a, unsigned long b) {
  return (a >= b) ? (a - b) : (b - a);
}

static unsigned long readEchoUsOnce() {
  // Give a short LOW pulse beforehand to ensure a clean HIGH pulse.
  digitalWrite(TRIG, LOW);
  delayMicroseconds(5);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  return pulseIn(ECHO, HIGH, config.pulseTimeoutUs);
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

void setup() {
  Serial.begin(74800);

  beginFileSystem();
  if (!loadConfigFromFs() || !validateConfig(config)) {
    config = DEFAULT_CONFIG;
    resetMeasurementState();
    saveConfigToFs();
  }
  resetMeasurementState();

  pinMode(WATER, OUTPUT);
  pinMode(ERRLED, OUTPUT);
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  // Water OFF by default
  digitalWrite(WATER, HIGH);
  digitalWrite(ERRLED, HIGH);
  digitalWrite(TRIG, LOW);

  if (DEBUG) {
    Serial.println(F("Config runtime initialized"));
  }

  applyNetworkMode();
}

// The loop routine runs over and over again forever:
void loop() {
  delay(filling ? config.waterDelayMs : config.loopDelayMs);

  unsigned long waterLevelUs = WaterLevel();

  if (waterLevelUs >= config.waterErrUs) {
    if (DEBUG) Serial.println("water reading invalid, STOP water");
    digitalWrite(WATER, HIGH);
    digitalWrite(ERRLED, LOW);
    filling = false;
    count = 0;
    return;
  }

  digitalWrite(ERRLED, HIGH);

  if (WaterHigh(waterLevelUs)) {
    if (DEBUG) Serial.println("water level is HIGH, STOP water...");
    digitalWrite(WATER, HIGH);
    filling = false;
    count = 0;
    return;
  }

  if (WaterLow(waterLevelUs) || filling) {
    filling = true;
    if (DEBUG) Serial.println("water low, FILLING water");
    digitalWrite(WATER, LOW);

    // Stop watering if watering for too long.
    if (count++ > (int)config.waterMaxDuration) {
      digitalWrite(WATER, HIGH);
      digitalWrite(ERRLED, LOW);
      filling = false;
      count = 0;
      if (DEBUG) Serial.print("water has been ON for too long, ");
      if (DEBUG) Serial.println("emergency STOP, water off");
    }

    return;
  }

  if (DEBUG) Serial.println("water level is NORMAL, STOP water...");
  digitalWrite(WATER, HIGH);
  filling = false;
  count = 0;
}

unsigned long WaterLevel() {
  unsigned long measuredUs = readMedianEchoUs();
  unsigned long acceptedUs = deglitchShortEchoUs(measuredUs);
  unsigned long estimateUs = acceptedUs;

  if (acceptedUs < config.waterErrUs) {
    estimateUs = (unsigned long)(simpleKalmanFilter.updateEstimate((float)acceptedUs) + 0.5f);
  }

  if (DEBUG_M) {
    Serial.print("Measured: ");
    Serial.print(measuredUs);
    Serial.print(" us (");
    Serial.print(usToCm(measuredUs));
    Serial.print(" cm), Accepted: ");
    Serial.print(acceptedUs);
    Serial.print(" us (");
    Serial.print(usToCm(acceptedUs));
    Serial.print(" cm), Estimate: ");
    Serial.print(estimateUs);
    Serial.print(" us (");
    Serial.print(usToCm(estimateUs));
    Serial.println(" cm)");
  }

  return acceptedUs;
}

bool WaterLow(unsigned long waterLevelUs) {
  return (waterLevelUs >= config.waterLowUs) && (waterLevelUs < config.waterErrUs);
}

bool WaterHigh(unsigned long waterLevelUs) {
  return (waterLevelUs <= config.waterHighUs);
}
