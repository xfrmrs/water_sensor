#ifndef IP_UTILS_H
#define IP_UTILS_H

#ifdef ARDUINO
#include <Arduino.h>
#include <ESP8266WiFi.h> // ESP8266 core header that provides IPAddress
#else
#include <string>
#include <cstdint>
#ifndef WATER_SENSOR_COMMON_H
using String = std::string;
#endif
struct IPAddress;
#endif

bool parseIpAddressString(const String &value, IPAddress &parsedValue);

#endif // IP_UTILS_H
