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
  {"wifiStaSsid", offsetof(Config, wifiStaSsid), CONFIG_FIELD_STRING, false, true},
  {"wifiStaPassword", offsetof(Config, wifiStaPassword), CONFIG_FIELD_STRING, false, false},
  {"enableStationDhcp", offsetof(Config, enableStationDhcp), CONFIG_FIELD_BOOL, false, true},
  {"enableApDhcp", offsetof(Config, enableApDhcp), CONFIG_FIELD_BOOL, false, true},
  {"wifiApSsid", offsetof(Config, wifiApSsid), CONFIG_FIELD_STRING, false, true},
  {"wifiStaIp", offsetof(Config, wifiStaIp), CONFIG_FIELD_STRING, false, true},
  {"wifiStaGateway", offsetof(Config, wifiStaGateway), CONFIG_FIELD_STRING, false, true},
  {"wifiStaSubnet", offsetof(Config, wifiStaSubnet), CONFIG_FIELD_STRING, false, true},
  {"wifiApIp", offsetof(Config, wifiApIp), CONFIG_FIELD_STRING, false, true},
  {"wifiApGateway", offsetof(Config, wifiApGateway), CONFIG_FIELD_STRING, false, true},
  {"wifiApSubnet", offsetof(Config, wifiApSubnet), CONFIG_FIELD_STRING, false, true},
  {"wifiStaConnectTimeoutMs", offsetof(Config, wifiStaConnectTimeoutMs), CONFIG_FIELD_ULONG, false, true},
  {"httpPort", offsetof(Config, httpPort), CONFIG_FIELD_ULONG, false, true},
  {"websocketPort", offsetof(Config, websocketPort), CONFIG_FIELD_ULONG, false, true},
  {"dnsPort", offsetof(Config, dnsPort), CONFIG_FIELD_ULONG, false, true},
  {"wifiApChannel", offsetof(Config, wifiApChannel), CONFIG_FIELD_UINT8, false, true},
  {"wifiApHidden", offsetof(Config, wifiApHidden), CONFIG_FIELD_BOOL, false, true},
  {"wifiApMaxConnections", offsetof(Config, wifiApMaxConnections), CONFIG_FIELD_UINT8, false, true},
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
  {"wifiStaPassword", offsetof(Config, wifiStaPassword), CONFIG_FIELD_STRING, 0},
  {"enableStationDhcp", offsetof(Config, enableStationDhcp), CONFIG_FIELD_BOOL, 0},
  {"enableApDhcp", offsetof(Config, enableApDhcp), CONFIG_FIELD_BOOL, 0},
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
  {"dnsPort", offsetof(Config, dnsPort), CONFIG_FIELD_ULONG, 0},
  {"wifiApChannel", offsetof(Config, wifiApChannel), CONFIG_FIELD_UINT8, 0},
  {"wifiApHidden", offsetof(Config, wifiApHidden), CONFIG_FIELD_BOOL, 0},
  {"wifiApMaxConnections", offsetof(Config, wifiApMaxConnections), CONFIG_FIELD_UINT8, 0},
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
    const char *fieldName = CONFIG_JSON_FIELDS[i].name;
    if (strcmp(fieldName, "wifiStaPassword") == 0 || strcmp(fieldName, "wifiApPassword") == 0) {
      continue;
    }
    writeConfigJsonField(output, first, CONFIG_JSON_FIELDS[i], source);
  }

  if (includeSecrets) {
    writeJsonStringField(output, first, "wifiStaPassword", source.wifiStaPassword);
    writeJsonStringField(output, first, "wifiApPassword", source.wifiApPassword);
  }

  if (includePasswordFlags) {
    writeJsonBoolField(output, first, "hasStaPassword", source.wifiStaPassword.length() > 0);
    writeJsonBoolField(output, first, "hasApPassword", source.wifiApPassword.length() > 0);
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

  if (candidate.dnsPort == 0 || candidate.dnsPort > 65535UL) {
    errorMessage = F("DNS port must be a valid UDP port number.");
    return false;
  }

  if (candidate.wifiApChannel < 1 || candidate.wifiApChannel > 13) {
    errorMessage = F("Setup AP channel must be between 1 and 13.");
    return false;
  }

  if (candidate.wifiApMaxConnections == 0 || candidate.wifiApMaxConnections > 8) {
    errorMessage = F("Setup AP max connections must be between 1 and 8.");
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

  if (candidate.wifiStaConnectTimeoutMs == 0) {
    errorMessage = F("Wi-Fi station timeout must be greater than 0.");
    return false;
  }

  if (candidate.wifiStaIp.length() == 0 ||
      candidate.wifiStaGateway.length() == 0 ||
      candidate.wifiStaSubnet.length() == 0) {
    errorMessage = F("Station IP, gateway, and subnet must be provided in config.json.");
    return false;
  }

  if (!parseIpAddressString(candidate.wifiStaIp, parsedIp) ||
      !parseIpAddressString(candidate.wifiStaGateway, parsedIp) ||
      !parseIpAddressString(candidate.wifiStaSubnet, parsedIp)) {
    errorMessage = F("Station IP, gateway, and subnet must be valid dotted-quad IPv4 addresses.");
    return false;
  }

  if (!parseIpAddressString(candidate.wifiApIp, parsedIp) ||
      !parseIpAddressString(candidate.wifiApGateway, parsedIp) ||
      !parseIpAddressString(candidate.wifiApSubnet, parsedIp)) {
    errorMessage = F("Setup AP IP, gateway, and subnet values must be valid dotted-quad IPv4 addresses.");
    return false;
  }

  return true;
}

void printLittleFsInventory() {
  if (!fileSystemReady) {
    Serial.println(F("LittleFS inventory unavailable because the filesystem is not mounted."));
    return;
  }

  Serial.println(F("LittleFS root inventory:"));
  Dir root = LittleFS.openDir("/");
  bool anyFile = false;
  while (root.next()) {
    anyFile = true;
    Serial.print(F("  "));
    Serial.print(root.fileName());
    File file = root.openFile("r");
    if (file) {
      Serial.print(F("  "));
      Serial.print(file.size());
      Serial.println(F(" bytes"));
      file.close();
    } else {
      Serial.println(F("  size unavailable"));
    }
  }

  if (!anyFile) {
    Serial.println(F("  <empty>"));
  }
}

static bool readConfigFilePayload(String &payload, String &errorMessage) {
  if (!fileSystemReady) {
    errorMessage = F("LittleFS is not mounted.");
    return false;
  }

  if (!LittleFS.exists(CONFIG_FILE_PATH)) {
    errorMessage = F("/config.json is not present on LittleFS.");
    return false;
  }

  File configFile = LittleFS.open(CONFIG_FILE_PATH, "r");
  if (!configFile) {
    errorMessage = F("/config.json is present but cannot be opened for reading.");
    return false;
  }

  const size_t fileSize = configFile.size();
  payload = configFile.readString();
  configFile.close();

  if (payload.length() == 0) {
    errorMessage = F("/config.json is empty.");
    return false;
  }

  if (payload.length() != fileSize) {
    errorMessage = F("/config.json read length differs from file size.");
    return false;
  }

  return true;
}


static bool isJsonWhitespace(char value) {
  return value == ' ' || value == '\t' || value == '\r' || value == '\n';
}

static size_t skipJsonWhitespace(const String &payload, size_t pos) {
  while (pos < payload.length() && isJsonWhitespace(payload.charAt(pos))) {
    ++pos;
  }
  return pos;
}

static uint8_t hexDigitValue(char value) {
  if (value >= '0' && value <= '9') {
    return (uint8_t)(value - '0');
  }
  if (value >= 'a' && value <= 'f') {
    return (uint8_t)(10 + value - 'a');
  }
  if (value >= 'A' && value <= 'F') {
    return (uint8_t)(10 + value - 'A');
  }
  return 255;
}

static bool parseJsonStringAt(const String &payload, size_t &pos, String &value, String &errorMessage) {
  pos = skipJsonWhitespace(payload, pos);
  if (pos >= payload.length() || payload.charAt(pos) != '"') {
    errorMessage = F("Expected JSON string.");
    return false;
  }

  ++pos;
  value = "";

  while (pos < payload.length()) {
    char current = payload.charAt(pos++);

    if (current == '"') {
      return true;
    }

    if (current != '\\') {
      value += current;
      continue;
    }

    if (pos >= payload.length()) {
      errorMessage = F("Unterminated JSON string escape sequence.");
      return false;
    }

    char escaped = payload.charAt(pos++);
    switch (escaped) {
      case '"': value += '"'; break;
      case '\\': value += '\\'; break;
      case '/': value += '/'; break;
      case 'b': value += '\b'; break;
      case 'f': value += '\f'; break;
      case 'n': value += '\n'; break;
      case 'r': value += '\r'; break;
      case 't': value += '\t'; break;
      case 'u': {
        if (pos + 4 > payload.length()) {
          errorMessage = F("Incomplete JSON unicode escape sequence.");
          return false;
        }
        uint16_t codepoint = 0;
        for (uint8_t i = 0; i < 4; ++i) {
          uint8_t digit = hexDigitValue(payload.charAt(pos + i));
          if (digit > 15) {
            errorMessage = F("Invalid JSON unicode escape sequence.");
            return false;
          }
          codepoint = (uint16_t)((codepoint << 4) | digit);
        }
        pos += 4;
        value += codepoint <= 0x7F ? (char)codepoint : '?';
        break;
      }
      default:
        errorMessage = F("Invalid JSON string escape sequence.");
        return false;
    }
  }

  errorMessage = F("Unterminated JSON string.");
  return false;
}

static bool scanJsonValueEnd(const String &payload, size_t valueStart, size_t &valueEnd, String &errorMessage) {
  size_t pos = skipJsonWhitespace(payload, valueStart);
  if (pos >= payload.length()) {
    errorMessage = F("Expected JSON value.");
    return false;
  }

  char first = payload.charAt(pos);
  if (first == '"') {
    String ignored;
    if (!parseJsonStringAt(payload, pos, ignored, errorMessage)) {
      return false;
    }
    valueEnd = skipJsonWhitespace(payload, pos);
    return true;
  }

  if (first == '{' || first == '[') {
    uint8_t depth = 0;
    bool inString = false;
    bool escaped = false;

    for (size_t i = pos; i < payload.length(); ++i) {
      char current = payload.charAt(i);

      if (inString) {
        if (escaped) {
          escaped = false;
        } else if (current == '\\') {
          escaped = true;
        } else if (current == '"') {
          inString = false;
        }
        continue;
      }

      if (current == '"') {
        inString = true;
      } else if (current == '{' || current == '[') {
        ++depth;
      } else if (current == '}' || current == ']') {
        if (depth == 0) {
          errorMessage = F("Mismatched JSON container close.");
          return false;
        }
        --depth;
        if (depth == 0) {
          valueEnd = skipJsonWhitespace(payload, i + 1);
          return true;
        }
      }
    }

    errorMessage = F("Unterminated JSON container value.");
    return false;
  }

  while (pos < payload.length()) {
    char current = payload.charAt(pos);
    if (current == ',' || current == '}' || current == ']') {
      break;
    }
    ++pos;
  }

  valueEnd = pos;
  String raw = payload.substring(valueStart, valueEnd);
  raw.trim();
  if (raw.length() == 0) {
    errorMessage = F("Expected JSON scalar value.");
    return false;
  }

  return true;
}

static bool findTopLevelJsonField(const String &payload, const char *fieldName, size_t &valueStart, size_t &valueEnd, bool &found, String &errorMessage) {
  found = false;
  size_t pos = skipJsonWhitespace(payload, 0);

  if (pos >= payload.length() || payload.charAt(pos) != '{') {
    errorMessage = F("JSON root value must be an object.");
    return false;
  }
  ++pos;

  while (true) {
    pos = skipJsonWhitespace(payload, pos);
    if (pos >= payload.length()) {
      errorMessage = F("Unterminated JSON object.");
      return false;
    }

    if (payload.charAt(pos) == '}') {
      pos = skipJsonWhitespace(payload, pos + 1);
      if (pos != payload.length()) {
        errorMessage = F("Unexpected data after JSON object.");
        return false;
      }
      return true;
    }

    String key;
    if (!parseJsonStringAt(payload, pos, key, errorMessage)) {
      errorMessage = F("Invalid JSON object key.");
      return false;
    }

    pos = skipJsonWhitespace(payload, pos);
    if (pos >= payload.length() || payload.charAt(pos) != ':') {
      errorMessage = F("Expected ':' after JSON object key.");
      return false;
    }
    ++pos;

    valueStart = skipJsonWhitespace(payload, pos);
    if (!scanJsonValueEnd(payload, valueStart, valueEnd, errorMessage)) {
      return false;
    }

    if (key == fieldName) {
      found = true;
      return true;
    }

    pos = skipJsonWhitespace(payload, valueEnd);
    if (pos >= payload.length()) {
      errorMessage = F("Unterminated JSON object.");
      return false;
    }

    char delimiter = payload.charAt(pos);
    if (delimiter == ',') {
      ++pos;
      continue;
    }
    if (delimiter == '}') {
      pos = skipJsonWhitespace(payload, pos + 1);
      if (pos != payload.length()) {
        errorMessage = F("Unexpected data after JSON object.");
        return false;
      }
      return true;
    }

    errorMessage = F("Expected ',' or '}' after JSON object value.");
    return false;
  }
}

static bool readFlatJsonRawField(const String &payload, const char *name, bool required, String &raw, bool &present, String &errorMessage) {
  size_t valueStart = 0;
  size_t valueEnd = 0;
  if (!findTopLevelJsonField(payload, name, valueStart, valueEnd, present, errorMessage)) {
    return false;
  }

  if (!present) {
    if (required) {
      errorMessage = F("Missing required field: ");
      errorMessage += name;
      return false;
    }
    return true;
  }

  raw = payload.substring(valueStart, valueEnd);
  raw.trim();
  if (raw.length() == 0) {
    errorMessage = F("Invalid empty field value: ");
    errorMessage += name;
    return false;
  }

  return true;
}

static bool readFlatJsonStringField(const String &payload, const char *name, bool required, String &target, bool &present, String &errorMessage) {
  String raw;
  if (!readFlatJsonRawField(payload, name, required, raw, present, errorMessage)) {
    return false;
  }
  if (!present) {
    return true;
  }

  size_t pos = 0;
  String parsed;
  if (!parseJsonStringAt(raw, pos, parsed, errorMessage)) {
    errorMessage = F("Invalid string field: ");
    errorMessage += name;
    return false;
  }
  pos = skipJsonWhitespace(raw, pos);
  if (pos != raw.length()) {
    errorMessage = F("Invalid trailing data in string field: ");
    errorMessage += name;
    return false;
  }

  target = parsed;
  return true;
}

static bool readFlatJsonBoolField(const String &payload, const char *name, bool required, bool &target, bool &present, String &errorMessage) {
  String raw;
  if (!readFlatJsonRawField(payload, name, required, raw, present, errorMessage)) {
    return false;
  }
  if (!present) {
    return true;
  }

  if (raw == "true") {
    target = true;
    return true;
  }
  if (raw == "false") {
    target = false;
    return true;
  }

  errorMessage = F("Invalid boolean field: ");
  errorMessage += name;
  return false;
}

static bool readFlatJsonULongField(const String &payload, const char *name, bool required, unsigned long &target, bool &present, String &errorMessage) {
  String raw;
  if (!readFlatJsonRawField(payload, name, required, raw, present, errorMessage)) {
    return false;
  }
  if (!present) {
    return true;
  }

  char *endPtr = nullptr;
  unsigned long parsed = strtoul(raw.c_str(), &endPtr, 10);
  if (endPtr == raw.c_str() || *endPtr != '\0') {
    errorMessage = F("Invalid unsigned integer field: ");
    errorMessage += name;
    return false;
  }

  target = parsed;
  return true;
}

static bool readFlatJsonUint8Field(const String &payload, const char *name, bool required, uint8_t &target, bool &present, String &errorMessage) {
  unsigned long parsed = 0;
  if (!readFlatJsonULongField(payload, name, required, parsed, present, errorMessage)) {
    return false;
  }
  if (!present) {
    return true;
  }
  if (parsed > 255UL) {
    errorMessage = F("Invalid uint8 field: ");
    errorMessage += name;
    return false;
  }

  target = (uint8_t)parsed;
  return true;
}

static bool readFlatJsonFloatField(const String &payload, const char *name, bool required, float &target, bool &present, String &errorMessage) {
  String raw;
  if (!readFlatJsonRawField(payload, name, required, raw, present, errorMessage)) {
    return false;
  }
  if (!present) {
    return true;
  }

  char *endPtr = nullptr;
  float parsed = strtof(raw.c_str(), &endPtr);
  if (endPtr == raw.c_str() || *endPtr != '\0') {
    errorMessage = F("Invalid floating-point field: ");
    errorMessage += name;
    return false;
  }

  target = parsed;
  return true;
}

static bool parseFlatConfigField(const String &payload, const ConfigFieldDescriptor &desc, Config &target, bool strictRuntimeFields, String &errorMessage) {
  bool present = false;
  bool required = desc.requiredAlways || (desc.requiredWhenStrict && strictRuntimeFields);
  void *fieldPtr = (uint8_t *)&target + desc.offset;

  switch (desc.type) {
    case CONFIG_FIELD_BOOL:
      return readFlatJsonBoolField(payload, desc.name, required, *(bool *)fieldPtr, present, errorMessage);
    case CONFIG_FIELD_UINT8:
      return readFlatJsonUint8Field(payload, desc.name, required, *(uint8_t *)fieldPtr, present, errorMessage);
    case CONFIG_FIELD_ULONG:
      return readFlatJsonULongField(payload, desc.name, required, *(unsigned long *)fieldPtr, present, errorMessage);
    case CONFIG_FIELD_FLOAT:
      return readFlatJsonFloatField(payload, desc.name, required, *(float *)fieldPtr, present, errorMessage);
    case CONFIG_FIELD_STRING:
      return readFlatJsonStringField(payload, desc.name, required, *(String *)fieldPtr, present, errorMessage);
  }

  errorMessage = F("Unsupported config field type.");
  return false;
}

static bool parseFlatSharedConfigFields(const String &payload, Config &parsed, bool strictRuntimeFields, String &errorMessage) {
  size_t valueStart = 0;
  size_t valueEnd = 0;
  bool found = false;
  if (!findTopLevelJsonField(payload, "__json_root_probe__", valueStart, valueEnd, found, errorMessage)) {
    return false;
  }

  for (size_t i = 0; i < sizeof(SHARED_CONFIG_FIELDS) / sizeof(SHARED_CONFIG_FIELDS[0]); ++i) {
    if (!parseFlatConfigField(payload, SHARED_CONFIG_FIELDS[i], parsed, strictRuntimeFields, errorMessage)) {
      return false;
    }
  }

  trimConfigTextFields(parsed);
  return true;
}

static bool parseConfigFromFlatJsonPayload(const String &payload, Config &candidate, String &errorMessage) {
  Config parsed = {};
  if (!parseFlatSharedConfigFields(payload, parsed, true, errorMessage)) {
    return false;
  }

  bool present = false;
  if (!readFlatJsonStringField(payload, "wifiStaPassword", true, parsed.wifiStaPassword, present, errorMessage)) {
    return false;
  }
  if (!readFlatJsonStringField(payload, "wifiApPassword", true, parsed.wifiApPassword, present, errorMessage)) {
    return false;
  }

  candidate = parsed;
  return true;
}


bool loadConfigFromFs(String &errorMessage) {
  String payload;
  if (!readConfigFilePayload(payload, errorMessage)) {
    return false;
  }

  Config candidate = {};
  if (!parseConfigFromFlatJsonPayload(payload, candidate, errorMessage)) {
    return false;
  }

  if (!validateConfig(candidate, errorMessage)) {
    return false;
  }

  config = candidate;
  Serial.print(F("Loaded /config.json bytes: "));
  Serial.println(payload.length());
  return true;
}

bool loadConfigFromFs() {
  String errorMessage;
  const bool loaded = loadConfigFromFs(errorMessage);
  if (!loaded) {
    Serial.print(F("Config load error: "));
    Serial.println(errorMessage);
  }
  return loaded;
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
         left.enableStationDhcp != right.enableStationDhcp ||
         left.enableApDhcp != right.enableApDhcp ||
         left.wifiApSsid != right.wifiApSsid ||
         left.wifiApPassword != right.wifiApPassword ||
         left.wifiStaIp != right.wifiStaIp ||
         left.wifiStaGateway != right.wifiStaGateway ||
         left.wifiStaSubnet != right.wifiStaSubnet ||
         left.wifiApIp != right.wifiApIp ||
         left.wifiApGateway != right.wifiApGateway ||
         left.wifiApSubnet != right.wifiApSubnet ||
         left.dnsPort != right.dnsPort ||
         left.wifiApChannel != right.wifiApChannel ||
         left.wifiApHidden != right.wifiApHidden ||
         left.wifiApMaxConnections != right.wifiApMaxConnections ||
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
  Serial.print(F("  enableStationDhcp="));
  Serial.println(source.enableStationDhcp ? F("true") : F("false"));
  Serial.print(F("  wifiStaIp="));
  Serial.println(source.wifiStaIp);
  Serial.print(F("  enableApDhcp="));
  Serial.println(source.enableApDhcp ? F("true") : F("false"));
  Serial.print(F("  wifiApSsid="));
  Serial.println(source.wifiApSsid);
  Serial.print(F("  wifiApIp="));
  Serial.println(source.wifiApIp);
  Serial.print(F("  wifiApChannel="));
  Serial.println(source.wifiApChannel);
  Serial.print(F("  wifiApHidden="));
  Serial.println(source.wifiApHidden ? F("true") : F("false"));
  Serial.print(F("  wifiApMaxConnections="));
  Serial.println(source.wifiApMaxConnections);
  Serial.print(F("  dnsPort="));
  Serial.println(source.dnsPort);
}

bool parseConfigFromRequestBody(Config &candidate, String &errorMessage) {
  String body = server->arg("plain");
  if (body.length() == 0) {
    errorMessage = F("Request body is empty.");
    return false;
  }

  Config parsed = config;
  if (!parseFlatSharedConfigFields(body, parsed, true, errorMessage)) {
    return false;
  }

  bool clearStaPassword = false;
  bool clearApPassword = false;
  String staPasswordInput = "";
  String apPasswordInput = "";
  bool present = false;

  if (!readFlatJsonBoolField(body, "clearStaPassword", false, clearStaPassword, present, errorMessage)) {
    return false;
  }
  if (!readFlatJsonBoolField(body, "clearApPassword", false, clearApPassword, present, errorMessage)) {
    return false;
  }
  bool staPasswordPresent = false;
  if (!readFlatJsonStringField(body, "wifiStaPassword", false, staPasswordInput, staPasswordPresent, errorMessage)) {
    return false;
  }
  bool apPasswordPresent = false;
  if (!readFlatJsonStringField(body, "wifiApPassword", false, apPasswordInput, apPasswordPresent, errorMessage)) {
    return false;
  }

  if (clearStaPassword) {
    parsed.wifiStaPassword = "";
  } else if (staPasswordPresent) {
    parsed.wifiStaPassword = staPasswordInput;
  } else {
    parsed.wifiStaPassword = config.wifiStaPassword;
  }

  if (clearApPassword) {
    parsed.wifiApPassword = "";
  } else if (apPasswordPresent) {
    parsed.wifiApPassword = apPasswordInput;
  } else {
    parsed.wifiApPassword = config.wifiApPassword;
  }

  candidate = parsed;
  return true;
}
