#pragma once

#include "Arduino.h"
#include "Arduino_JSON.h"
#include "DNSServer.h"
#include "ESP8266WebServer.h"
#include "ESP8266WiFi.h"
#include "Hash.h"
#include "LittleFS.h"
#include "SimpleKalmanFilter.h"
#include "WebSocketsServer.h"

#include "../src/common.h"

#ifdef TEST_SUPPORT_IMPLEMENTATION
HardwareSerial Serial;
JSONClass JSON;
LittleFSClass LittleFS;
WiFiClass WiFi;

Config config = {};
Config bootConfig = {};
ESP8266WebServer *server = nullptr;
WebSocketsServer *webSocket = nullptr;
DNSServer *dnsServer = nullptr;
SimpleKalmanFilter *kalmanFilter = nullptr;
MeasurementSnapshot latestMeasurement = {};
HistorySample measurementHistory[MAX_HISTORY_BUFFER_CAPACITY] = {};
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
#endif
