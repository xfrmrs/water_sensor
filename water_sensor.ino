#include "common.h"

Config config = {};
Config bootConfig = {};

ESP8266WebServer *server = nullptr;
WebSocketsServer *webSocket = nullptr;
SimpleKalmanFilter *kalmanFilter = nullptr;

MeasurementSnapshot latestMeasurement = {0, 0, 0, 0, 0, 0, 0, MEASUREMENT_STATE_ERROR, 0};
HistorySample measurementHistory[MAX_HISTORY_BUFFER_CAPACITY];
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

uint8_t activeTrigPin = 0;
uint8_t activeEchoPin = 0;
uint8_t activeWaterPin = 0;
uint8_t activeErrLedPin = 0;
unsigned long activeSerialBaud = 0;

unsigned long lastAcceptedUs = 0;
unsigned long pendingShortUs = 0;
uint8_t pendingShortCount = 0;
uint8_t invalidBurstCount = 0;

void setup() {
  bool configLoaded = false;
  String validationError;
  unsigned long startupBaud = BOOT_SERIAL_BAUD;

  beginFileSystem();
  configLoaded = loadConfigFromFs();

  if (configLoaded && !validateConfig(config, validationError)) {
    configLoaded = false;
    Serial.begin(startupBaud);
    delay(10);
    Serial.println(F("Config file loaded but validation failed."));
    Serial.println(validationError);
  } else if (!configLoaded) {
    Serial.begin(startupBaud);
    delay(10);
    Serial.println(F("Config file missing or invalid; no built-in runtime defaults are applied."));
  } else {
    startupBaud = config.serialBaud > 0 ? config.serialBaud : BOOT_SERIAL_BAUD;
    Serial.begin(startupBaud);
    delay(10);
  }

  uint16_t httpPort = configLoaded && config.httpPort > 0 ? config.httpPort : BOOT_HTTP_PORT;
  uint16_t websocketPort = configLoaded && config.websocketPort > 0 ? config.websocketPort : BOOT_WEBSOCKET_PORT;
  server = new ESP8266WebServer(httpPort);
  webSocket = new WebSocketsServer(websocketPort);

  if (fileSystemReady) {
    Serial.println(F("LittleFS mounted"));
  } else {
    Serial.println(F("LittleFS mount failed; runtime config will remain empty."));
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
