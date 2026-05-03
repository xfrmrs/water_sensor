const char *currentNetworkModeName() {
  return currentNetworkMode == NETWORK_MODE_STA ? "Local Wi-Fi" : "Setup AP";
}

const String &currentNetworkName() {
  return currentNetworkMode == NETWORK_MODE_STA ? config.wifiStaSsid : config.wifiApSsid;
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

void servicePendingIo() {
  server.handleClient();
  webSocket.loop();
  applyPendingNetworkChange();
  applyPendingRestart();
}

void serviceRuntime(unsigned long durationMs) {
  unsigned long startedAt = millis();
  while ((millis() - startedAt) < durationMs) {
    servicePendingIo();

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

void writeRestartFieldsJsonArray(JsonOutput &output) {
  bool first = true;
  jsonWrite(output, "[");

  if (bootConfig.trigPin != config.trigPin) writeJsonArrayStringValue(output, first, "trigPin");
  if (bootConfig.echoPin != config.echoPin) writeJsonArrayStringValue(output, first, "echoPin");
  if (bootConfig.waterPin != config.waterPin) writeJsonArrayStringValue(output, first, "waterPin");
  if (bootConfig.errLedPin != config.errLedPin) writeJsonArrayStringValue(output, first, "errLedPin");
  if (bootConfig.serialBaud != config.serialBaud) writeJsonArrayStringValue(output, first, "serialBaud");

  jsonWrite(output, "]");
}

void writeStatusJsonObject(JsonOutput &output) {
  bool first = true;
  jsonWrite(output, "{");
  writeJsonStringField(output, first, "message", uiStatusMessage);
  writeJsonStringField(output, first, "reconnectHint", reconnectHint);
  writeJsonStringField(output, first, "networkMode", currentNetworkModeName());
  writeJsonStringField(output, first, "networkName", currentNetworkName());
  writeJsonStringField(output, first, "networkIp", currentNetworkIp);
  writeJsonBoolField(output, first, "networkReady", networkReady);
  writeJsonBoolField(output, first, "restartRequired", restartRequired());
  writeJsonBoolField(output, first, "restartPending", restartPending);

  writeJsonFieldPrefix(output, first, "restartFields");
  writeRestartFieldsJsonArray(output);

  writeJsonFieldPrefix(output, first, "activeRuntime");
  jsonWrite(output, "{");
  bool activeFirst = true;
  writeJsonULongField(output, activeFirst, "trigPin", activeTrigPin);
  writeJsonULongField(output, activeFirst, "echoPin", activeEchoPin);
  writeJsonULongField(output, activeFirst, "waterPin", activeWaterPin);
  writeJsonULongField(output, activeFirst, "errLedPin", activeErrLedPin);
  writeJsonULongField(output, activeFirst, "serialBaud", activeSerialBaud);
  jsonWrite(output, "}");

  writeJsonFieldPrefix(output, first, "latestMeasurement");
  writeMeasurementJsonObject(output, latestMeasurement);

  writeJsonULongField(output, first, "historyCount", measurementHistoryCount);
  jsonWrite(output, "}");
}

String buildStatusJson() {
  String json;
  json.reserve(1200);
  JsonOutput output = makeStringJsonOutput(json);
  writeStatusJsonObject(output);
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
  String measurementJson;
  measurementJson.reserve(320);
  JsonOutput output = makeStringJsonOutput(measurementJson);
  writeMeasurementJsonObject(output, snapshot);
  String payload = buildWebSocketMessage("telemetry", measurementJson);
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

static void beginStreamingJsonResponse(int statusCode) {
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.sendHeader("Cache-Control", "no-store");
  server.send(statusCode, "application/json", "");
}

static void sendJsonErrorResponse(int statusCode, const String &message) {
  beginStreamingJsonResponse(statusCode);
  JsonOutput output = makeHttpJsonOutput();
  bool first = true;
  jsonWrite(output, "{");
  writeJsonBoolField(output, first, "ok", false);
  writeJsonStringField(output, first, "message", message);
  jsonWrite(output, "}");
  server.sendContent("");
}

void handleRoot() {
  if (!sendStaticFile(INDEX_FILE_PATH, "text/html")) {
    server.send(500, "text/plain", "Dashboard assets are unavailable.");
  }
}

void handleBootstrap() {
  beginStreamingJsonResponse(200);
  JsonOutput output = makeHttpJsonOutput();
  bool first = true;
  jsonWrite(output, "{");

  writeJsonFieldPrefix(output, first, "config");
  writeConfigJsonObject(output, config, false, true);

  writeJsonFieldPrefix(output, first, "status");
  writeStatusJsonObject(output);

  writeJsonFieldPrefix(output, first, "history");
  jsonWrite(output, "[");
  for (uint8_t i = 0; i < measurementHistoryCount; ++i) {
    const size_t index = (measurementHistoryHead + i) % HISTORY_CAPACITY;
    if (i > 0) {
      jsonWrite(output, ",");
    }
    writeHistoryPointJsonObject(output, measurementHistory[index]);
  }
  jsonWrite(output, "]");

  writeJsonFieldPrefix(output, first, "options");
  jsonWrite(output, "{");
  bool optionsFirst = true;
  writeJsonFieldPrefix(output, optionsFirst, "safePins");
  jsonWrite(output, "[");
  bool firstPin = true;
  for (size_t i = 0; i < (sizeof(SAFE_GPIO_VALUES) / sizeof(SAFE_GPIO_VALUES[0])); ++i) {
    writeJsonArrayULongValue(output, firstPin, SAFE_GPIO_VALUES[i]);
  }
  jsonWrite(output, "]");
  writeJsonFieldPrefix(output, optionsFirst, "supportedBauds");
  jsonWrite(output, "[");
  bool firstBaud = true;
  for (size_t i = 0; i < (sizeof(SUPPORTED_SERIAL_BAUDS) / sizeof(SUPPORTED_SERIAL_BAUDS[0])); ++i) {
    writeJsonArrayULongValue(output, firstBaud, SUPPORTED_SERIAL_BAUDS[i]);
  }
  jsonWrite(output, "]");
  writeJsonULongField(output, optionsFirst, "historyCapacity", HISTORY_CAPACITY);
  writeJsonULongField(output, optionsFirst, "maxConfigurablePings", MAX_CONFIGURABLE_PINGS);
  jsonWrite(output, "}");

  jsonWrite(output, "}");
  server.sendContent("");
}

void handleConfigSave() {
  Config previousConfig = config;
  Config candidate = config;
  String errorMessage;

  if (!parseConfigFromRequestBody(candidate, errorMessage)) {
    sendJsonErrorResponse(400, errorMessage);
    return;
  }

  if (!validateConfig(candidate, errorMessage)) {
    sendJsonErrorResponse(400, errorMessage);
    return;
  }

  bool networkChanged = previousConfig.wifiStaSsid != candidate.wifiStaSsid ||
                        previousConfig.wifiStaPassword != candidate.wifiStaPassword ||
                        previousConfig.wifiApSsid != candidate.wifiApSsid ||
                        previousConfig.wifiApPassword != candidate.wifiApPassword ||
                        previousConfig.wifiStaIp != candidate.wifiStaIp ||
                        previousConfig.wifiStaGateway != candidate.wifiStaGateway ||
                        previousConfig.wifiStaSubnet != candidate.wifiStaSubnet ||
                        previousConfig.wifiApIp != candidate.wifiApIp ||
                        previousConfig.wifiApGateway != candidate.wifiApGateway ||
                        previousConfig.wifiApSubnet != candidate.wifiApSubnet ||
                        previousConfig.wifiStaConnectTimeoutMs != candidate.wifiStaConnectTimeoutMs;
  config = candidate;
  rebuildKalmanFilter();
  resetMeasurementState();

  if (!saveConfigToFs()) {
    config = previousConfig;
    rebuildKalmanFilter();
    resetMeasurementState();
    sendJsonErrorResponse(500, F("Settings could not be saved to LittleFS."));
    return;
  }

  if (networkChanged) {
    reconnectHint = String(F("Reconnect to ")) + config.wifiStaIp +
                    F(" if the station join succeeds, or to setup AP ") +
                    config.wifiApSsid + F(" at ") + config.wifiApIp +
                    F(" if it falls back.");
  } else {
    reconnectHint = "";
  }

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

  beginStreamingJsonResponse(200);
  JsonOutput output = makeHttpJsonOutput();
  bool first = true;
  jsonWrite(output, "{");
  writeJsonBoolField(output, first, "ok", true);
  writeJsonBoolField(output, first, "restartRequired", restartRequired());
  writeJsonFieldPrefix(output, first, "restartFields");
  writeRestartFieldsJsonArray(output);
  writeJsonStringField(output, first, "reconnectHint", reconnectHint);
  writeJsonFieldPrefix(output, first, "config");
  writeConfigJsonObject(output, config, false, true);
  writeJsonFieldPrefix(output, first, "status");
  writeStatusJsonObject(output);
  jsonWrite(output, "}");
  server.sendContent("");
  broadcastStatus();
}

void handleRestart() {
  uiStatusMessage = F("Restart requested. The device is rebooting.");
  restartPending = true;
  restartAfterMs = millis() + 750UL;
  broadcastStatus();
  beginStreamingJsonResponse(200);
  JsonOutput output = makeHttpJsonOutput();
  bool first = true;
  jsonWrite(output, "{");
  writeJsonBoolField(output, first, "ok", true);
  writeJsonStringField(output, first, "message", uiStatusMessage);
  writeJsonFieldPrefix(output, first, "status");
  writeStatusJsonObject(output);
  jsonWrite(output, "}");
  server.sendContent("");
}

void handleNotFound() {
  String path = server.uri();
  String contentType = detectContentType(path);
  if (sendStaticFile(path.c_str(), contentType.c_str())) {
    return;
  }

  sendJsonErrorResponse(404, F("Not found"));
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
