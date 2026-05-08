#include "src/common.h"

Config config = {};
Config bootConfig = {};

ESP8266WebServer *server = nullptr;
WebSocketsServer *webSocket = nullptr;
DNSServer *dnsServer = nullptr;
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
bool emergencyStopActive = false;
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

static void haltWithoutRuntimeConfig(const String &message) {
  Serial.print(F("Configuration halt: "));
  Serial.println(message);
  Serial.println(F("Runtime configuration is required at /config.json on LittleFS."));
  Serial.println(F("Network service and water-control service are inactive in this state."));
  while (true) {
    delay(1000);
    yield();
  }
}

void setup() {
  bool configLoaded = false;

  Serial.begin(BOOT_SERIAL_BAUD);
  delay(10);
  Serial.println();
  Serial.println(F("Water sensor controller booting"));

  if (!beginFileSystem()) {
    haltWithoutRuntimeConfig(F("LittleFS mount failed."));
  }

  Serial.println(F("LittleFS mounted"));
  printLittleFsInventory();

  String configError;
  configLoaded = loadConfigFromFs(configError);
  if (!configLoaded) {
    haltWithoutRuntimeConfig(configError);
  }
  Serial.println(F("Runtime configuration loaded and validated."));

  if (config.serialBaud != BOOT_SERIAL_BAUD) {
    Serial.flush();
    Serial.begin(config.serialBaud);
    delay(10);
  }

  server = new ESP8266WebServer((uint16_t)config.httpPort);
  webSocket = new WebSocketsServer((uint16_t)config.websocketPort);
  dnsServer = new DNSServer();

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
