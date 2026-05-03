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

static bool parseSharedConfigFields(const JSONVar &json, Config &parsed, bool strictRuntimeFields, String &errorMessage) {
  if (!readConfigField(json, "debugControlLogs", parsed.debugControlLogs, strictRuntimeFields, errorMessage)) return false;
  if (!readConfigField(json, "debugMeasurementLogs", parsed.debugMeasurementLogs, strictRuntimeFields, errorMessage)) return false;
  if (!readConfigField(json, "trigPin", parsed.trigPin, strictRuntimeFields, errorMessage)) return false;
  if (!readConfigField(json, "echoPin", parsed.echoPin, strictRuntimeFields, errorMessage)) return false;
  if (!readConfigField(json, "waterPin", parsed.waterPin, strictRuntimeFields, errorMessage)) return false;
  if (!readConfigField(json, "errLedPin", parsed.errLedPin, strictRuntimeFields, errorMessage)) return false;
  if (!readConfigField(json, "serialBaud", parsed.serialBaud, strictRuntimeFields, errorMessage)) return false;
  if (!readConfigField(json, "kalmanMeasurementError", parsed.kalmanMeasurementError, strictRuntimeFields, errorMessage)) return false;
  if (!readConfigField(json, "kalmanEstimateError", parsed.kalmanEstimateError, strictRuntimeFields, errorMessage)) return false;
  if (!readConfigField(json, "kalmanProcessNoise", parsed.kalmanProcessNoise, strictRuntimeFields, errorMessage)) return false;

  if (!readConfigField(json, "waterMaxDuration", parsed.waterMaxDuration, true, errorMessage)) return false;
  if (!readConfigField(json, "loopDelayMs", parsed.loopDelayMs, true, errorMessage)) return false;
  if (!readConfigField(json, "waterDelayMs", parsed.waterDelayMs, true, errorMessage)) return false;
  if (!readConfigField(json, "waterLowUs", parsed.waterLowUs, true, errorMessage)) return false;
  if (!readConfigField(json, "waterHighUs", parsed.waterHighUs, true, errorMessage)) return false;
  if (!readConfigField(json, "waterErrUs", parsed.waterErrUs, true, errorMessage)) return false;
  if (!readConfigField(json, "pulseTimeoutUs", parsed.pulseTimeoutUs, true, errorMessage)) return false;
  if (!readConfigField(json, "nPings", parsed.nPings, true, errorMessage)) return false;
  if (!readConfigField(json, "minValidPings", parsed.minValidPings, true, errorMessage)) return false;
  if (!readConfigField(json, "pingGapMs", parsed.pingGapMs, true, errorMessage)) return false;
  if (!readConfigField(json, "minValidEchoUs", parsed.minValidEchoUs, true, errorMessage)) return false;
  if (!readConfigField(json, "shortJumpUs", parsed.shortJumpUs, true, errorMessage)) return false;
  if (!readConfigField(json, "shortConfirmDeltaUs", parsed.shortConfirmDeltaUs, true, errorMessage)) return false;
  if (!readConfigField(json, "shortConfirmCount", parsed.shortConfirmCount, true, errorMessage)) return false;
  if (!readConfigField(json, "maxHeldInvalidBursts", parsed.maxHeldInvalidBursts, true, errorMessage)) return false;
  if (!readConfigField(json, "wifiStaSsid", parsed.wifiStaSsid, true, errorMessage)) return false;
  if (!readConfigField(json, "wifiApSsid", parsed.wifiApSsid, true, errorMessage)) return false;
  if (!readConfigField(json, "wifiStaIp", parsed.wifiStaIp, true, errorMessage)) return false;
  if (!readConfigField(json, "wifiStaGateway", parsed.wifiStaGateway, true, errorMessage)) return false;
  if (!readConfigField(json, "wifiStaSubnet", parsed.wifiStaSubnet, true, errorMessage)) return false;
  if (!readConfigField(json, "wifiApIp", parsed.wifiApIp, true, errorMessage)) return false;
  if (!readConfigField(json, "wifiApGateway", parsed.wifiApGateway, true, errorMessage)) return false;
  if (!readConfigField(json, "wifiApSubnet", parsed.wifiApSubnet, true, errorMessage)) return false;
  if (!readConfigField(json, "wifiStaConnectTimeoutMs", parsed.wifiStaConnectTimeoutMs, true, errorMessage)) return false;

  trimConfigTextFields(parsed);
  return true;
}

void writeConfigJsonObject(JsonOutput &output, const Config &source, bool includeSecrets, bool includePasswordFlags) {
  bool first = true;
  jsonWrite(output, "{");
  writeJsonBoolField(output, first, "debugControlLogs", source.debugControlLogs);
  writeJsonBoolField(output, first, "debugMeasurementLogs", source.debugMeasurementLogs);
  writeJsonULongField(output, first, "trigPin", source.trigPin);
  writeJsonULongField(output, first, "echoPin", source.echoPin);
  writeJsonULongField(output, first, "waterPin", source.waterPin);
  writeJsonULongField(output, first, "errLedPin", source.errLedPin);
  writeJsonULongField(output, first, "serialBaud", source.serialBaud);
  writeJsonFloatField(output, first, "kalmanMeasurementError", source.kalmanMeasurementError, 4);
  writeJsonFloatField(output, first, "kalmanEstimateError", source.kalmanEstimateError, 4);
  writeJsonFloatField(output, first, "kalmanProcessNoise", source.kalmanProcessNoise, 5);
  writeJsonULongField(output, first, "waterMaxDuration", source.waterMaxDuration);
  writeJsonULongField(output, first, "loopDelayMs", source.loopDelayMs);
  writeJsonULongField(output, first, "waterDelayMs", source.waterDelayMs);
  writeJsonULongField(output, first, "waterLowUs", source.waterLowUs);
  writeJsonULongField(output, first, "waterHighUs", source.waterHighUs);
  writeJsonULongField(output, first, "waterErrUs", source.waterErrUs);
  writeJsonULongField(output, first, "pulseTimeoutUs", source.pulseTimeoutUs);
  writeJsonULongField(output, first, "nPings", source.nPings);
  writeJsonULongField(output, first, "minValidPings", source.minValidPings);
  writeJsonULongField(output, first, "pingGapMs", source.pingGapMs);
  writeJsonULongField(output, first, "minValidEchoUs", source.minValidEchoUs);
  writeJsonULongField(output, first, "shortJumpUs", source.shortJumpUs);
  writeJsonULongField(output, first, "shortConfirmDeltaUs", source.shortConfirmDeltaUs);
  writeJsonULongField(output, first, "shortConfirmCount", source.shortConfirmCount);
  writeJsonULongField(output, first, "maxHeldInvalidBursts", source.maxHeldInvalidBursts);
  writeJsonStringField(output, first, "wifiStaSsid", source.wifiStaSsid);
  writeJsonStringField(output, first, "wifiApSsid", source.wifiApSsid);
  writeJsonStringField(output, first, "wifiStaIp", source.wifiStaIp);
  writeJsonStringField(output, first, "wifiStaGateway", source.wifiStaGateway);
  writeJsonStringField(output, first, "wifiStaSubnet", source.wifiStaSubnet);
  writeJsonStringField(output, first, "wifiApIp", source.wifiApIp);
  writeJsonStringField(output, first, "wifiApGateway", source.wifiApGateway);
  writeJsonStringField(output, first, "wifiApSubnet", source.wifiApSubnet);
  writeJsonULongField(output, first, "wifiStaConnectTimeoutMs", source.wifiStaConnectTimeoutMs);

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
  Config parsed = DEFAULT_CONFIG;
  if (!parseSharedConfigFields(json, parsed, false, errorMessage)) {
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

bool restartRequired() {
  return bootConfig.trigPin != config.trigPin ||
         bootConfig.echoPin != config.echoPin ||
         bootConfig.waterPin != config.waterPin ||
         bootConfig.errLedPin != config.errLedPin ||
         bootConfig.serialBaud != config.serialBaud;
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
  if (!parseSharedConfigFields(json, parsed, true, errorMessage)) {
    return false;
  }

  bool clearStaPassword = false;
  bool clearApPassword = false;
  String staPasswordInput = "";
  String apPasswordInput = "";

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
