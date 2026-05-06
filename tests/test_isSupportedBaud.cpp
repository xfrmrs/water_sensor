#include <iostream>
#include <cassert>
#include <string>
#include <cstdint>
#include <cstddef>
#include <vector>

#define F(x) x
typedef const char __FlashStringHelper;

class String {
public:
    std::string str;
    String() {}
    String(const char* s) : str(s) {}
    String(const String& s) : str(s.str) {}
    String& operator=(const char* s) { str = s; return *this; }
    String& operator=(const String& s) { str = s.str; return *this; }
    String& operator+=(const char* s) { str += s; return *this; }
    String& operator+=(const String& s) { str += s.str; return *this; }
    bool operator==(const String& s) const { return str == s.str; }
    bool operator!=(const String& s) const { return str != s.str; }
    size_t length() const { return str.length(); }
    void trim() {}
    void reserve(size_t) {}
};

class IPAddress {
public:
    bool fromString(const String& s) { return true; }
};

class JSONVar {
public:
    bool hasOwnProperty(const char*) const { return false; }
    JSONVar operator[](const char*) const { return JSONVar(); }
};

class JSONClass {
public:
    JSONVar parse(const String&) { return JSONVar(); }
    String typeof_(const JSONVar&) { return "object"; }
} JSON;

#define typeof typeof_

class File {
public:
    operator bool() const { return false; }
    String readString() { return ""; }
    void close() {}
    size_t print(const String&) { return 0; }
};

class LittleFSClass {
public:
    bool begin() { return true; }
    bool exists(const char*) { return false; }
    File open(const char*, const char*) { return File(); }
    bool remove(const char*) { return true; }
    bool rename(const char*, const char*) { return true; }
} LittleFS;

class ServerMock {
public:
    String arg(const char*) { return ""; }
} server_mock;
ServerMock* server = &server_mock;

class SerialMock {
public:
    void print(const char*) {}
    void print(const String&) {}
    void print(int) {}
    void print(char) {}
    void print(unsigned long) {}
    void print(uint8_t) {}
    void println(const char*) {}
    void println(const String&) {}
    void println(int) {}
    void println(unsigned long) {}
    void println(uint8_t) {}
} Serial;

#define WATER_SENSOR_COMMON_H

#undef offsetof
#define offsetof(type, member) ((size_t)&(((type*)0)->member))

static const char *CONFIG_FILE_PATH = "/config.json";
static const char *CONFIG_TEMP_PATH = "/config.tmp";
static const uint8_t MAX_PING_BUFFER_CAPACITY = 12;
static const size_t MAX_HISTORY_BUFFER_CAPACITY = 120;
static const uint8_t SAFE_GPIO_VALUES[] = {4, 5, 12, 13, 14, 16};
static const uint32_t SAFE_GPIO_MASK = (1UL << 4) | (1UL << 5) | (1UL << 12) | (1UL << 13) | (1UL << 14) | (1UL << 16);
static const unsigned long SUPPORTED_SERIAL_BAUDS[] = {
  9600UL,
  19200UL,
  38400UL,
  57600UL,
  74880UL,
  115200UL,
  230400UL
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
  bool enableStationDhcp;
  String wifiApSsid;
  String wifiApPassword;
  String wifiStaIp;
  String wifiStaGateway;
  String wifiStaSubnet;
  String wifiApIp;
  String wifiApGateway;
  String wifiApSubnet;
  String adminPassword;
  unsigned long wifiStaConnectTimeoutMs;
  unsigned long httpPort;
  unsigned long websocketPort;
  unsigned long historyCapacity;
  unsigned long maxConfigurablePings;
};

struct JsonOutput {};

Config config;
Config bootConfig;
bool fileSystemReady = false;

void printConfigSummary(const Config &source, const __FlashStringHelper *label);

bool requireBoolField(const JSONVar &, const char *, bool &) { return true; }
bool optionalBoolField(const JSONVar &, const char *, bool &) { return true; }
bool requireUint8Field(const JSONVar &, const char *, uint8_t &) { return true; }
bool optionalUint8Field(const JSONVar &, const char *, uint8_t &) { return true; }
bool requireUnsignedLongField(const JSONVar &, const char *, unsigned long &) { return true; }
bool optionalUnsignedLongField(const JSONVar &, const char *, unsigned long &) { return true; }
bool requireFloatField(const JSONVar &, const char *, float &) { return true; }
bool optionalFloatField(const JSONVar &, const char *, float &) { return true; }
bool requireStringField(const JSONVar &, const char *, String &) { return true; }
bool optionalStringField(const JSONVar &, const char *, String &) { return true; }

void writeJsonBoolField(JsonOutput &, bool &, const char *, bool) {}
void writeJsonUIntField(JsonOutput &, bool &, const char *, uint8_t) {}
void writeJsonULongField(JsonOutput &, bool &, const char *, unsigned long) {}
void writeJsonFloatField(JsonOutput &, bool &, const char *, float, uint8_t) {}
void writeJsonStringField(JsonOutput &, bool &, const char *, const String &) {}
void jsonWrite(JsonOutput &, const char *) {}
JsonOutput makeStringJsonOutput(String &) { return JsonOutput(); }

bool jsonVarToBool(const JSONVar &, bool &) { return true; }
bool jsonVarToString(const JSONVar &, String &) { return true; }

#include "../config.ino"

void test_isSupportedBaud() {
    assert(isSupportedBaud(9600UL) == true);
    assert(isSupportedBaud(19200UL) == true);
    assert(isSupportedBaud(38400UL) == true);
    assert(isSupportedBaud(57600UL) == true);
    assert(isSupportedBaud(74880UL) == true);
    assert(isSupportedBaud(115200UL) == true);
    assert(isSupportedBaud(230400UL) == true);

    assert(isSupportedBaud(0UL) == false);
    assert(isSupportedBaud(9601UL) == false);
    assert(isSupportedBaud(115201UL) == false);
    assert(isSupportedBaud(250000UL) == false);
}

int main() {
    test_isSupportedBaud();
    std::cout << "All isSupportedBaud tests passed!" << std::endl;
    return 0;
}
