#include "common.h"

bool beginFileSystem() {
  fileSystemReady = LittleFS.begin();
  return fileSystemReady;
}

static void setFieldError(String &errorMessage, const char *fieldName) {
  errorMessage = F("Missing or invalid field: ");
  errorMessage += fieldName;
}

static void trimConfigTextFields(Config &target) {
  target.wifiStaSsid.trim();
  target.wifiApSsid.trim();
  target.wifiStaIp.trim();
  target.wifiStaGateway.trim();
  target.wifiStaSubnet.trim();
  target.wifiApIp.trim();
  target.wifiApGateway.trim();
  target.wifiApSubnet.trim();
}

enum ConfigFieldType : uint8_t {
  CONFIG_FIELD_BOOL,
  CONFIG_FIELD_UINT8,
  CONFIG_FIELD_ULONG,
  CONFIG_FIELD_FLOAT,
  CONFIG_FIELD_STRING
};

struct ConfigFieldDescriptor {
  const char *name;
  size_t offset;
  ConfigFieldType type;
  bool requiredAlways;
  bool requiredWhenStrict;
};

static const ConfigFieldDescriptor SHARED_CONFIG_FIELDS[] = {
  {"debugControlLogs", offsetof(Config, debugControlLogs), CONFIG_FIELD_BOOL, false, true},
  {"debugMeasurementLogs", offsetof(Config, debugMeasurementLogs), CONFIG_FIELD_BOOL, false, true},
  {"trigPin", offsetof(Config, trigPin), CONFIG_FIELD_UINT8, false, true},
  {"echoPin", offsetof(Config, echoPin), CONFIG_FIELD_UINT8, false, true},
  {"waterPin", offsetof(Config, waterPin), CONFIG_FIELD_UINT8, false, true},
  {"errLedPin", offsetof(Config, errLedPin), CONFIG_FIELD_UINT8, false, true},
  {"serialBaud", offsetof(Config, serialBaud), CONFIG_FIELD_ULONG, false, true},
  {"kalmanMeasurementError", offsetof(Config, kalmanMeasurementError), CONFIG_FIELD_FLOAT, false, true},
  {"kalmanEstimateError", offsetof(Config, kalmanEstimateError), CONFIG_FIELD_FLOAT, false, true},
  {"kalmanProcessNoise", offsetof(Config, kalmanProcessNoise), CONFIG_FIELD_FLOAT, false, true},
  {"waterMaxDuration", offsetof(Config, waterMaxDuration), CONFIG_FIELD_ULONG, true, false},
  {"loopDelayMs", offsetof(Config, loopDelayMs), CONFIG_FIELD_ULONG, true, false},
  {"waterDelayMs", offsetof(Config, waterDelayMs), CONFIG_FIELD_ULONG, true, false},
  {"waterLowUs", offsetof(Config, waterLowUs), CONFIG_FIELD_ULONG, true, false},
  {"waterHighUs", offsetof(Config, waterHighUs), CONFIG_FIELD_ULONG, true, false},
  {"waterErrUs", offsetof(Config, waterErrUs), CONFIG_FIELD_ULONG, true, false},
  {"pulseTimeoutUs", offsetof(Config, pulseTimeoutUs), CONFIG_FIELD_ULONG, true, false},
  {"nPings", offsetof(Config, nPings), CONFIG_FIELD_UINT8, true, false},
  {"minValidPings", offsetof(Config, minValidPings), CONFIG_FIELD_UINT8, true, false},
  {"pingGapMs", offsetof(Config, pingGapMs), CONFIG_FIELD_ULONG, true, false},
  {"minValidEchoUs", offsetof(Config, minValidEchoUs), CONFIG_FIELD_ULONG, true, false},
  {"shortJumpUs", offsetof(Config, shortJumpUs), CONFIG_FIELD_ULONG, true, false},
  {"shortConfirmDeltaUs", offsetof(Config, shortConfirmDeltaUs), CONFIG_FIELD_ULONG, true, false},
  {"shortConfirmCount", offsetof(Config, shortConfirmCount), CONFIG_FIELD_UINT8, true, false},
  {"maxHeldInvalidBursts", offsetof(Config, maxHeldInvalidBursts), CONFIG_FIELD_UINT8, true, false},
  {"wifiStaSsid", offsetof(Config, wifiStaSsid), CONFIG_FIELD_STRING, false, false},
  {"wifiStaPassword", offsetof(Config, wifiStaPassword), CONFIG_FIELD_STRING, false, false},
  {"enableStationDhcp", offsetof(Config, enableStationDhcp), CONFIG_FIELD_BOOL, false, false},
  {"wifiApSsid", offsetof(Config, wifiApSsid), CONFIG_FIELD_STRING, true, false},
  {"wifiStaIp", offsetof(Config, wifiStaIp), CONFIG_FIELD_STRING, false, false},
  {"wifiStaGateway", offsetof(Config, wifiStaGateway), CONFIG_FIELD_STRING, false, false},
  {"wifiStaSubnet", offsetof(Config, wifiStaSubnet), CONFIG_FIELD_STRING, false, false},
  {"wifiApIp", offsetof(Config, wifiApIp), CONFIG_FIELD_STRING, true, false},
  {"wifiApGateway", offsetof(Config, wifiApGateway), CONFIG_FIELD_STRING, true, false},
  {"wifiApSubnet", offsetof(Config, wifiApSubnet), CONFIG_FIELD_STRING, true, false},
  {"adminPassword", offsetof(Config, adminPassword), CONFIG_FIELD_STRING, false, false},
  {"wifiStaConnectTimeoutMs", offsetof(Config, wifiStaConnectTimeoutMs), CONFIG_FIELD_ULONG, false, false},
  {"httpPort", offsetof(Config, httpPort), CONFIG_FIELD_ULONG, false, true},
  {"websocketPort", offsetof(Config, websocketPort), CONFIG_FIELD_ULONG, false, true},
  {"historyCapacity", offsetof(Config, historyCapacity), CONFIG_FIELD_ULONG, false, true},
  {"maxConfigurablePings", offsetof(Config, maxConfigurablePings), CONFIG_FIELD_ULONG, false, true}
};

static bool readConfigField(const JSONVar &json, const char *name, bool &target, bool strict, String &errorMessage) {
  bool ok = strict ? requireBoolField(json, name, target) : optionalBoolField(json, name, target);
  if (!ok) {
    setFieldError(errorMessage, name);
  }
  return ok;
}

static bool readConfigField(const JSONVar &json, const char *name, uint8_t &target, bool strict, String &errorMessage) {
  bool ok = strict ? requireUint8Field(json, name, target) : optionalUint8Field(json, name, target);
  if (!ok) {
    setFieldError(errorMessage, name);
  }
  return ok;
}

static bool readConfigField(const JSONVar &json, const char *name, unsigned long &target, bool strict, String &errorMessage) {
  bool ok = strict ? requireUnsignedLongField(json, name, target) : optionalUnsignedLongField(json, name, target);
  if (!ok) {
    setFieldError(errorMessage, name);
  }
  return ok;
}

static bool readConfigField(const JSONVar &json, const char *name, float &target, bool strict, String &errorMessage) {
  bool ok = strict ? requireFloatField(json, name, target) : optionalFloatField(json, name, target);
  if (!ok) {
    setFieldError(errorMessage, name);
  }
  return ok;
}

static bool readConfigField(const JSONVar &json, const char *name, String &target, bool strict, String &errorMessage) {
  bool ok = strict ? requireStringField(json, name, target) : optionalStringField(json, name, target);
  if (!ok) {
    setFieldError(errorMessage, name);
  }
  return ok;
}

static bool parseConfigField(const JSONVar &json, const ConfigFieldDescriptor &desc, Config &target, bool strictRuntimeFields, String &errorMessage) {
  bool required = desc.requiredAlways || (desc.requiredWhenStrict && strictRuntimeFields);
  void *fieldPtr = (uint8_t *)&target + desc.offset;

  switch (desc.type) {
    case CONFIG_FIELD_BOOL:
      return readConfigField(json, desc.name, *(bool *)fieldPtr, required, errorMessage);
    case CONFIG_FIELD_UINT8:
      return readConfigField(json, desc.name, *(uint8_t *)fieldPtr, required, errorMessage);
    case CONFIG_FIELD_ULONG:
      return readConfigField(json, desc.name, *(unsigned long *)fieldPtr, required, errorMessage);
    case CONFIG_FIELD_FLOAT:
      return readConfigField(json, desc.name, *(float *)fieldPtr, required, errorMessage);
    case CONFIG_FIELD_STRING:
      return readConfigField(json, desc.name, *(String *)fieldPtr, required, errorMessage);
  }

  return false;
}

static bool parseSharedConfigFields(const JSONVar &json, Config &parsed, bool strictRuntimeFields, String &errorMessage) {
  for (size_t i = 0; i < sizeof(SHARED_CONFIG_FIELDS) / sizeof(SHARED_CONFIG_FIELDS[0]); ++i) {
    if (!parseConfigField(json, SHARED_CONFIG_FIELDS[i], parsed, strictRuntimeFields, errorMessage)) {
      return false;
    }
  }

  trimConfigTextFields(parsed);
  return true;
}

struct ConfigJsonFieldDescriptor {
  const char *name;
  size_t offset;
  ConfigFieldType type;
  uint8_t decimals;
};

static const ConfigJsonFieldDescriptor CONFIG_JSON_FIELDS[] = {
  {"debugControlLogs", offsetof(Config, debugControlLogs), CONFIG_FIELD_BOOL, 0},
  {"debugMeasurementLogs", offsetof(Config, debugMeasurementLogs), CONFIG_FIELD_BOOL, 0},
  {"trigPin", offsetof(Config, trigPin), CONFIG_FIELD_UINT8, 0},
  {"echoPin", offsetof(Config, echoPin), CONFIG_FIELD_UINT8, 0},
  {"waterPin", offsetof(Config, waterPin), CONFIG_FIELD_UINT8, 0},
  {"errLedPin", offsetof(Config, errLedPin), CONFIG_FIELD_UINT8, 0},
  {"serialBaud", offsetof(Config, serialBaud), CONFIG_FIELD_ULONG, 0},
  {"kalmanMeasurementError", offsetof(Config, kalmanMeasurementError), CONFIG_FIELD_FLOAT, 4},
  {"kalmanEstimateError", offsetof(Config, kalmanEstimateError), CONFIG_FIELD_FLOAT, 4},
  {"kalmanProcessNoise", offsetof(Config, kalmanProcessNoise), CONFIG_FIELD_FLOAT, 5},
  {"waterMaxDuration", offsetof(Config, waterMaxDuration), CONFIG_FIELD_ULONG, 0},
  {"loopDelayMs", offsetof(Config, loopDelayMs), CONFIG_FIELD_ULONG, 0},
  {"waterDelayMs", offsetof(Config, waterDelayMs), CONFIG_FIELD_ULONG, 0},
  {"waterLowUs", offsetof(Config, waterLowUs), CONFIG_FIELD_ULONG, 0},
  {"waterHighUs", offsetof(Config, waterHighUs), CONFIG_FIELD_ULONG, 0},
  {"waterErrUs", offsetof(Config, waterErrUs), CONFIG_FIELD_ULONG, 0},
  {"pulseTimeoutUs", offsetof(Config, pulseTimeoutUs), CONFIG_FIELD_ULONG, 0},
  {"nPings", offsetof(Config, nPings), CONFIG_FIELD_UINT8, 0},
  {"minValidPings", offsetof(Config, minValidPings), CONFIG_FIELD_UINT8, 0},
  {"pingGapMs", offsetof(Config, pingGapMs), CONFIG_FIELD_ULONG, 0},
  {"minValidEchoUs", offsetof(Config, minValidEchoUs), CONFIG_FIELD_ULONG, 0},
  {"shortJumpUs", offsetof(Config, shortJumpUs), CONFIG_FIELD_ULONG, 0},
  {"shortConfirmDeltaUs", offsetof(Config, shortConfirmDeltaUs), CONFIG_FIELD_ULONG, 0},
  {"shortConfirmCount", offsetof(Config, shortConfirmCount), CONFIG_FIELD_UINT8, 0},
  {"maxHeldInvalidBursts", offsetof(Config, maxHeldInvalidBursts), CONFIG_FIELD_UINT8, 0},
  {"wifiStaSsid", offsetof(Config, wifiStaSsid), CONFIG_FIELD_STRING, 0},
  {"enableStationDhcp", offsetof(Config, enableStationDhcp), CONFIG_FIELD_BOOL, 0},
  {"wifiApSsid", offsetof(Config, wifiApSsid), CONFIG_FIELD_STRING, 0},
  {"wifiStaIp", offsetof(Config, wifiStaIp), CONFIG_FIELD_STRING, 0},
  {"wifiStaGateway", offsetof(Config, wifiStaGateway), CONFIG_FIELD_STRING, 0},
  {"wifiStaSubnet", offsetof(Config, wifiStaSubnet), CONFIG_FIELD_STRING, 0},
  {"wifiApIp", offsetof(Config, wifiApIp), CONFIG_FIELD_STRING, 0},
  {"wifiApGateway", offsetof(Config, wifiApGateway), CONFIG_FIELD_STRING, 0},
  {"wifiApSubnet", offsetof(Config, wifiApSubnet), CONFIG_FIELD_STRING, 0},
  {"wifiStaConnectTimeoutMs", offsetof(Config, wifiStaConnectTimeoutMs), CONFIG_FIELD_ULONG, 0},
  {"httpPort", offsetof(Config, httpPort), CONFIG_FIELD_ULONG, 0},
  {"websocketPort", offsetof(Config, websocketPort), CONFIG_FIELD_ULONG, 0},
  {"historyCapacity", offsetof(Config, historyCapacity), CONFIG_FIELD_ULONG, 0},
  {"maxConfigurablePings", offsetof(Config, maxConfigurablePings), CONFIG_FIELD_ULONG, 0}
};

static void writeConfigJsonField(JsonOutput &output, bool &first, const ConfigJsonFieldDescriptor &desc, const Config &source) {
  const void *fieldPtr = (const uint8_t *)&source + desc.offset;

  switch (desc.type) {
    case CONFIG_FIELD_BOOL:
      writeJsonBoolField(output, first, desc.name, *(const bool *)fieldPtr);
      break;
    case CONFIG_FIELD_UINT8:
      writeJsonUIntField(output, first, desc.name, *(const uint8_t *)fieldPtr);
      break;
    case CONFIG_FIELD_ULONG:
      writeJsonULongField(output, first, desc.name, *(const unsigned long *)fieldPtr);
      break;
    case CONFIG_FIELD_FLOAT:
      writeJsonFloatField(output, first, desc.name, *(const float *)fieldPtr, desc.decimals);
      break;
    case CONFIG_FIELD_STRING:
      writeJsonStringField(output, first, desc.name, *(const String *)fieldPtr);
      break;
  }
}

void writeConfigJsonObject(JsonOutput &output, const Config &source, bool includeSecrets, bool includePasswordFlags) {
  bool first = true;
  jsonWrite(output, "{");

  for (size_t i = 0; i < sizeof(CONFIG_JSON_FIELDS) / sizeof(CONFIG_JSON_FIELDS[0]); ++i) {
    writeConfigJsonField(output, first, CONFIG_JSON_FIELDS[i], source);
  }

  if (includeSecrets) {
    writeJsonStringField(output, first, "wifiStaPassword", source.wifiStaPassword);
    writeJsonStringField(output, first, "wifiApPassword", source.wifiApPassword);
    writeJsonStringField(output, first, "adminPassword", source.adminPassword);
  }

  if (includePasswordFlags) {
    writeJsonBoolField(output, first, "hasStaPassword", source.wifiStaPassword.length() > 0);
    writeJsonBoolField(output, first, "hasApPassword", source.wifiApPassword.length() > 0);
    writeJsonBoolField(output, first, "hasAdminPassword", source.adminPassword.length() > 0);
  }

  jsonWrite(output, "}");
}

bool configFromJson(const JSONVar &json, Config &candidate, String &errorMessage) {
  Config parsed = {};
  if (!parseSharedConfigFields(json, parsed, true, errorMessage)) {
    return false;
  }

  if (!requireStringField(json, "wifiStaPassword", parsed.wifiStaPassword)) {
    setFieldError(errorMessage, "wifiStaPassword");
    return false;
  }
  if (!requireStringField(json, "wifiApPassword", parsed.wifiApPassword)) {
    setFieldError(errorMessage, "wifiApPassword");
    return false;
  }
  if (!requireStringField(json, "adminPassword", parsed.adminPassword)) {
    setFieldError(errorMessage, "adminPassword");
    return false;
  }

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
  return pinValue < 32 && (SAFE_GPIO_MASK & (1UL << pinValue));
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

  if (candidate.nPings == 0 || candidate.nPings > candidate.maxConfigurablePings) {
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

  if (candidate.httpPort == 0 || candidate.httpPort > 65535UL) {
    errorMessage = F("HTTP port must be a valid TCP port number.");
    return false;
  }

  if (candidate.websocketPort == 0 || candidate.websocketPort > 65535UL) {
    errorMessage = F("WebSocket port must be a valid TCP port number.");
    return false;
  }

  if (candidate.historyCapacity == 0 || candidate.historyCapacity > MAX_HISTORY_BUFFER_CAPACITY) {
    errorMessage = F("History capacity must be between 1 and the supported maximum.");
    return false;
  }

  if (candidate.maxConfigurablePings == 0 || candidate.maxConfigurablePings > MAX_PING_BUFFER_CAPACITY) {
    errorMessage = F("Max configurable pings must be between 1 and the supported maximum.");
    return false;
  }

  if (candidate.nPings > candidate.maxConfigurablePings) {
    errorMessage = F("Ping count must not exceed the configured maximum pings.");
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

  if (candidate.adminPassword.length() < 4) {
    errorMessage = F("Admin password must be at least 4 characters.");
    return false;
  }

  if (candidate.wifiStaConnectTimeoutMs == 0) {
    errorMessage = F("Wi-Fi station timeout must be greater than 0.");
    return false;
  }

  if (!candidate.enableStationDhcp) {
    if (candidate.wifiStaIp.length() == 0 ||
        candidate.wifiStaGateway.length() == 0 ||
        candidate.wifiStaSubnet.length() == 0) {
      errorMessage = F("Station IP, gateway, and subnet must be provided when DHCP is disabled.");
      return false;
    }

    if (!parseIpAddressString(candidate.wifiStaIp, parsedIp) ||
        !parseIpAddressString(candidate.wifiStaGateway, parsedIp) ||
        !parseIpAddressString(candidate.wifiStaSubnet, parsedIp)) {
      errorMessage = F("Station IP, gateway, and subnet must be valid dotted-quad IPv4 addresses.");
      return false;
    }
  }

  if (!parseIpAddressString(candidate.wifiApIp, parsedIp) ||
      !parseIpAddressString(candidate.wifiApGateway, parsedIp) ||
      !parseIpAddressString(candidate.wifiApSubnet, parsedIp)) {
    errorMessage = F("Setup AP IP, gateway, and subnet values must be valid dotted-quad IPv4 addresses.");
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

  Config candidate = {};
  String validationError;
  if (!configFromJson(json, candidate, validationError)) {
    return false;
  }

  if (!validateConfig(candidate, validationError)) {
    return false;
  }

  config = candidate;
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

  String payload;
  payload.reserve(1900);
  JsonOutput output = makeStringJsonOutput(payload);
  writeConfigJsonObject(output, config, true, false);
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

bool restartSensitiveSettingsDiffer(const Config &left, const Config &right) {
  return left.trigPin != right.trigPin ||
         left.echoPin != right.echoPin ||
         left.waterPin != right.waterPin ||
         left.errLedPin != right.errLedPin ||
         left.serialBaud != right.serialBaud ||
         left.httpPort != right.httpPort ||
         left.websocketPort != right.websocketPort;
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

bool parseConfigFromRequestBody(Config &candidate, String &errorMessage) {
  String body = server->arg("plain");
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
  if (!parseSharedConfigFields(json, parsed, true, errorMessage)) {
    return false;
  }

  bool clearStaPassword = false;
  bool clearApPassword = false;
  bool clearAdminPassword = false;
  String staPasswordInput = "";
  String apPasswordInput = "";
  String adminPasswordInput = "";

  if (json.hasOwnProperty("clearStaPassword") && !jsonVarToBool(json["clearStaPassword"], clearStaPassword)) {
    errorMessage = F("Invalid field: clearStaPassword");
    return false;
  }
  if (json.hasOwnProperty("clearApPassword") && !jsonVarToBool(json["clearApPassword"], clearApPassword)) {
    errorMessage = F("Invalid field: clearApPassword");
    return false;
  }
  if (json.hasOwnProperty("clearAdminPassword") && !jsonVarToBool(json["clearAdminPassword"], clearAdminPassword)) {
    errorMessage = F("Invalid field: clearAdminPassword");
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
  if (json.hasOwnProperty("adminPassword") && !jsonVarToString(json["adminPassword"], adminPasswordInput)) {
    errorMessage = F("Invalid field: adminPassword");
    return false;
  }

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

  if (adminPasswordInput.length() > 0) {
    parsed.adminPassword = adminPasswordInput;
  } else if (clearAdminPassword) {
    parsed.adminPassword = "";
  }

  candidate = parsed;
  return true;
}
