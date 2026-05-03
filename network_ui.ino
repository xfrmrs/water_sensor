#include "common.h"
extern "C" {
#include "user_interface.h"
}


static const char RECOVERY_DASHBOARD_HTML[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Water Station Recovery</title>
<style>
body{margin:0;padding:24px;background:#081417;color:#eff8f4;font-family:Arial,sans-serif}
main{max-width:760px;margin:0 auto;background:#102126;border:1px solid rgba(255,255,255,.12);border-radius:20px;padding:22px}
h1{margin:0 0 10px;font-family:Georgia,serif}.muted{color:#94b8b1}.status{margin:18px 0;padding:14px;border-radius:14px;background:rgba(255,255,255,.06);white-space:pre-wrap}
button{border:0;border-radius:14px;padding:14px 18px;font-size:1rem;font-weight:700;cursor:pointer}.danger{background:#ff7a67;color:#220704}.ghost{background:rgba(255,255,255,.08);color:#eff8f4;margin-left:10px}
</style>
</head>
<body>
<main>
<h1>Water Station Recovery Dashboard</h1>
<p class="muted">This page is served by firmware when the LittleFS dashboard assets are unavailable.</p>
<div id="status" class="status">Loading device status...</div>
<button id="stop" class="danger" type="button">Emergency Shutoff</button>
<button id="refresh" class="ghost" type="button">Refresh Status</button>
</main>
<script>
const statusEl=document.getElementById('status');
async function refresh(){
  try{
    const r=await fetch('/api/status',{cache:'no-store'});
    const s=await r.json();
    statusEl.textContent=`Mode: ${s.networkMode}\nNetwork: ${s.networkName}\nIP: ${s.networkIp}\nReady: ${s.networkReady}\nEmergency shutoff: ${s.emergencyStopActive}\nMessage: ${s.message||''}`;
  }catch(e){statusEl.textContent=e.message||'Status request failed.';}
}
async function stopNow(){
  if(!confirm('Activate emergency shutoff and force the water relay off?')) return;
  try{
    const r=await fetch('/api/emergency-stop',{method:'POST'});
    const d=await r.json();
    statusEl.textContent=d.status?`Emergency shutoff active.\nIP: ${d.status.networkIp}\nMessage: ${d.status.message}`:'Emergency shutoff active.';
  }catch(e){statusEl.textContent=e.message||'Emergency shutoff request failed.';}
}
document.getElementById('refresh').addEventListener('click',refresh);
document.getElementById('stop').addEventListener('click',stopNow);
refresh();
</script>
</body>
</html>
)rawliteral";


const char *wifiStatusName(wl_status_t status) {
  switch (status) {
    case WL_IDLE_STATUS: return "WL_IDLE_STATUS";
    case WL_NO_SSID_AVAIL: return "WL_NO_SSID_AVAIL";
    case WL_SCAN_COMPLETED: return "WL_SCAN_COMPLETED";
    case WL_CONNECTED: return "WL_CONNECTED";
    case WL_CONNECT_FAILED: return "WL_CONNECT_FAILED";
    case WL_CONNECTION_LOST: return "WL_CONNECTION_LOST";
    case WL_DISCONNECTED: return "WL_DISCONNECTED";
    default: return "WL_UNKNOWN";
  }
}

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
  networkReady = false;
  currentNetworkIp = "";

  if (config.wifiStaSsid.length() == 0) {
    Serial.println(F("Station SSID is empty; starting setup AP."));
    return false;
  }

  IPAddress staIp;
  IPAddress staGateway;
  IPAddress staSubnet;

  WiFi.persistent(false);
  WiFi.setAutoReconnect(false);
  if (dnsServer) {
    dnsServer->stop();
  }
  WiFi.softAPdisconnect(true);
  WiFi.disconnect(false);
  delay(100);
  WiFi.mode(WIFI_STA);

  if (!config.enableStationDhcp) {
    if (!parseIpAddressString(config.wifiStaIp, staIp) ||
        !parseIpAddressString(config.wifiStaGateway, staGateway) ||
        !parseIpAddressString(config.wifiStaSubnet, staSubnet)) {
      Serial.println(F("Station static IP config is invalid; starting setup AP."));
      return false;
    }
    if (!WiFi.config(staIp, staGateway, staSubnet)) {
      Serial.println(F("WiFi.config() rejected the station static IP values; starting setup AP."));
      return false;
    }
    Serial.print(F("Station static IP requested: "));
    Serial.println(config.wifiStaIp);
  } else {
    Serial.println(F("Station DHCP enabled."));
  }

  Serial.print(F("Joining station SSID: "));
  Serial.println(config.wifiStaSsid);
  WiFi.begin(config.wifiStaSsid.c_str(), config.wifiStaPassword.c_str());

  unsigned long startedAt = millis();
  wl_status_t lastStatus = WiFi.status();
  while ((WiFi.status() != WL_CONNECTED) &&
         ((millis() - startedAt) < config.wifiStaConnectTimeoutMs)) {
    wl_status_t currentStatus = WiFi.status();
    if (currentStatus != lastStatus) {
      Serial.print(F("Station status: "));
      Serial.println(wifiStatusName(currentStatus));
      lastStatus = currentStatus;
    }
    delay(250);
    yield();
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.print(F("Station join failed. Final status: "));
    Serial.println(wifiStatusName(WiFi.status()));
    WiFi.disconnect(false);
    networkReady = false;
    currentNetworkIp = "";
    return false;
  }

  networkReady = true;
  currentNetworkMode = NETWORK_MODE_STA;
  currentNetworkIp = WiFi.localIP().toString();
  Serial.print(F("Station joined. IP: "));
  Serial.println(currentNetworkIp);
  return true;
}

void startSoftApMode() {
  networkReady = false;
  currentNetworkMode = NETWORK_MODE_SOFTAP;
  currentNetworkIp = "";

  if (config.wifiApSsid.length() == 0) {
    Serial.println(F("Setup AP SSID is empty; cannot start AP."));
    return;
  }

  IPAddress apIp;
  IPAddress apGateway;
  IPAddress apSubnet;
  if (!parseIpAddressString(config.wifiApIp, apIp) ||
      !parseIpAddressString(config.wifiApGateway, apGateway) ||
      !parseIpAddressString(config.wifiApSubnet, apSubnet)) {
    Serial.println(F("Setup AP IP config is invalid; cannot start AP."));
    return;
  }

  WiFi.persistent(false);
  WiFi.softAPdisconnect(true);
  WiFi.disconnect(false);
  wifi_softap_dhcps_stop();
  delay(100);
  WiFi.mode(WIFI_AP);

  if (!WiFi.softAPConfig(apIp, apGateway, apSubnet)) {
    Serial.println(F("softAPConfig() rejected the AP IP values; cannot start AP."));
    return;
  }

  const char *apPassword = config.wifiApPassword.length() > 0 ? config.wifiApPassword.c_str() : nullptr;
  networkReady = WiFi.softAP(
    config.wifiApSsid.c_str(),
    apPassword,
    config.wifiApChannel,
    config.wifiApHidden ? 1 : 0,
    config.wifiApMaxConnections
  );
  if (networkReady) {
    if (config.enableApDhcp) {
      wifi_softap_dhcps_start();
      Serial.println(F("Setup AP DHCP enabled."));
    } else {
      wifi_softap_dhcps_stop();
      Serial.println(F("Setup AP DHCP disabled. Clients require static IPv4 settings."));
    }
  }
  delay(100);
  currentNetworkIp = WiFi.softAPIP().toString();

  Serial.print(F("Setup AP start result: "));
  Serial.println(networkReady ? F("success") : F("failed"));
  Serial.print(F("Setup AP SSID: "));
  Serial.println(config.wifiApSsid);
  Serial.print(F("Setup AP IP: "));
  Serial.println(currentNetworkIp);
  Serial.print(F("Setup AP dashboard URL: http://"));
  Serial.print(currentNetworkIp);
  Serial.println(F("/"));

  if (networkReady && dnsServer) {
    dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer->start((uint16_t)config.dnsPort, "*", apIp);
  }
}

void applyNetworkMode() {
  if (!connectToStationMode()) {
    Serial.println(F("Starting setup AP."));
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
  if (server) {
    server->stop();
    server->begin();
  }

  if (currentNetworkMode == NETWORK_MODE_STA) {
    uiStatusMessage = String(F("Network settings applied. Reconnect using the device LAN IP: ")) + currentNetworkIp;
  } else {
    uiStatusMessage = String(F("Setup AP is active. Reconnect to '")) +
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
  if (server) {
    server->handleClient();
  }
  if (webSocket) {
    webSocket->loop();
  }
  if (dnsServer && currentNetworkMode == NETWORK_MODE_SOFTAP) {
    dnsServer->processNextRequest();
  }
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
  if (bootConfig.httpPort != config.httpPort) writeJsonArrayStringValue(output, first, "httpPort");
  if (bootConfig.websocketPort != config.websocketPort) writeJsonArrayStringValue(output, first, "websocketPort");

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
  writeJsonBoolField(output, first, "emergencyStopActive", emergencyStopActive);

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

void broadcastJson(const char *type, const String &dataJson) {
  if (!webSocket) {
    return;
  }
  String payload = buildWebSocketMessage(type, dataJson);
  webSocket->broadcastTXT(payload);
}

void broadcastStatus() {
  broadcastJson("status", buildStatusJson());
}

void broadcastTelemetry(const MeasurementSnapshot &snapshot) {
  broadcastJson("telemetry", buildMeasurementJson(snapshot));
}

void handleWebSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  (void)payload;
  (void)length;

  if (type == WStype_CONNECTED) {
    String payload = buildWebSocketMessage("status", buildStatusJson());
    if (webSocket) {
      webSocket->sendTXT(num, payload);
    }
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

  if (server) {
    server->sendHeader("Cache-Control", "no-store");
    server->streamFile(file, contentType);
  }
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
  if (server) {
    server->setContentLength(CONTENT_LENGTH_UNKNOWN);
    server->sendHeader("Cache-Control", "no-store");
    server->send(statusCode, "application/json", "");
  }
}

static void sendJsonErrorResponse(int statusCode, const String &message) {
  beginStreamingJsonResponse(statusCode);
  JsonOutput output = makeHttpJsonOutput();
  bool first = true;
  jsonWrite(output, "{");
  writeJsonBoolField(output, first, "ok", false);
  writeJsonStringField(output, first, "message", message);
  jsonWrite(output, "}");
  if (server) {
    server->sendContent("");
  }
}

void handleRoot() {
  if (!sendStaticFile(INDEX_FILE_PATH, "text/html")) {
    if (server) {
      server->sendHeader("Cache-Control", "no-store");
      server->send_P(200, "text/html", RECOVERY_DASHBOARD_HTML);
    }
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
    const size_t index = (measurementHistoryHead + i) % MAX_HISTORY_BUFFER_CAPACITY;
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
  writeJsonULongField(output, optionsFirst, "historyCapacityMax", MAX_HISTORY_BUFFER_CAPACITY);
  writeJsonULongField(output, optionsFirst, "maxConfigurablePingsMax", MAX_PING_BUFFER_CAPACITY);
  jsonWrite(output, "}");

  jsonWrite(output, "}");
  if (server) {
    server->sendContent("");
  }
}

void handleStatusGet() {
  beginStreamingJsonResponse(200);
  JsonOutput output = makeHttpJsonOutput();
  writeStatusJsonObject(output);
  if (server) {
    server->sendContent("");
  }
}

void handleEmergencyStop() {
  activateEmergencyStop();
  uiStatusMessage = F("Emergency shutoff active. Water output is forced off until restart.");
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
  if (server) {
    server->sendContent("");
  }
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

  bool networkChanged = networkSettingsDiffer(previousConfig, candidate);
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

  reconnectHint = networkChanged ? buildReconnectHint(config) : "";

  if (networkChanged) {
    uiStatusMessage = F("Settings saved. Network settings are pending application.");
    networkReconnectPending = true;
    networkReconnectAfterMs = millis() + 1000UL;
    if (server) {
      server->sendHeader("Connection", "close");
    }
  } else if (restartRequired()) {
    uiStatusMessage = F("Settings saved. Restart required for low-level settings.");
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
  if (server) {
    server->sendContent("");
  }
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
  if (server) {
    server->sendContent("");
  }
}

void handleNotFound() {
  String path = server ? server->uri() : String();
  String contentType = detectContentType(path);
  if (sendStaticFile(path.c_str(), contentType.c_str())) {
    return;
  }

  sendJsonErrorResponse(404, F("Not found"));
}

void configureWebServer() {
  if (!server) {
    return;
  }

  server->on("/", HTTP_GET, handleRoot);
  server->on("/app.js", HTTP_GET, []() {
    if (!sendStaticFile(APP_JS_FILE_PATH, "application/javascript")) {
      server->send(404, "text/plain", "app.js not found");
    }
  });
  server->on("/style.css", HTTP_GET, []() {
    if (!sendStaticFile(STYLE_CSS_FILE_PATH, "text/css")) {
      server->send(404, "text/plain", "style.css not found");
    }
  });
  server->on("/api/bootstrap", HTTP_GET, handleBootstrap);
  server->on("/api/status", HTTP_GET, handleStatusGet);
  server->on("/api/emergency-stop", HTTP_POST, handleEmergencyStop);
  server->on("/api/config", HTTP_POST, handleConfigSave);
  server->on("/api/restart", HTTP_POST, handleRestart);
  server->onNotFound(handleNotFound);
  server->begin();
}

void configureWebSocket() {
  if (!webSocket) {
    return;
  }
  webSocket->begin();
  webSocket->onEvent(handleWebSocketEvent);
}
