#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using __FlashStringHelper = char;

#define F(x) x
#define PROGMEM
#define ICACHE_FLASH_ATTR
#define CONTENT_LENGTH_UNKNOWN -1
#define HTTP_GET 0
#define HTTP_POST 1

class String {
public:
  std::string value;

  String() = default;
  String(const char *text) : value(text ? text : "") {}
  String(const std::string &text) : value(text) {}
  String(char ch) : value(1, ch) {}
  String(unsigned int number) : value(std::to_string(number)) {}
  String(unsigned long number) : value(std::to_string(number)) {}
  String(int number) : value(std::to_string(number)) {}
  String(long number) : value(std::to_string(number)) {}
  String(float number, unsigned char decimals = 2) { assignFloat(number, decimals); }
  String(double number, unsigned char decimals = 2) { assignFloat(number, decimals); }

  String &operator=(const char *text) {
    value = text ? text : "";
    return *this;
  }

  String &operator=(const String &) = default;

  String &operator+=(const char *text) {
    value += text ? text : "";
    return *this;
  }

  String &operator+=(const String &other) {
    value += other.value;
    return *this;
  }

  String &operator+=(char ch) {
    value.push_back(ch);
    return *this;
  }

  bool operator==(const char *text) const { return value == (text ? text : ""); }
  bool operator!=(const char *text) const { return !(*this == text); }
  bool operator==(const String &other) const { return value == other.value; }
  bool operator!=(const String &other) const { return value != other.value; }

  size_t length() const { return value.length(); }
  const char *c_str() const { return value.c_str(); }
  void reserve(size_t) {}

  void trim() {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
      value.clear();
      return;
    }
    const auto last = value.find_last_not_of(" \t\r\n");
    value = value.substr(first, last - first + 1);
  }

  char charAt(size_t index) const {
    return index < value.length() ? value[index] : '\0';
  }

  String substring(size_t start, size_t end) const {
    if (start >= value.length() || end <= start) {
      return String();
    }
    return String(value.substr(start, end - start));
  }

  bool endsWith(const String &suffix) const {
    if (suffix.length() > length()) {
      return false;
    }
    return value.compare(length() - suffix.length(), suffix.length(), suffix.value) == 0;
  }

private:
  void assignFloat(double number, unsigned char decimals) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(decimals) << number;
    value = stream.str();
  }
};

inline String operator+(const String &left, const String &right) {
  return String(left.value + right.value);
}

inline String operator+(const String &left, const char *right) {
  return String(left.value + std::string(right ? right : ""));
}

inline String operator+(const char *left, const String &right) {
  return String(std::string(left ? left : "") + right.value);
}

class Print {
public:
  virtual ~Print() = default;
  virtual size_t print(const char *) { return 0; }
  virtual size_t print(const String &) { return 0; }
  virtual size_t print(unsigned long) { return 0; }
  virtual size_t print(unsigned int) { return 0; }
  virtual size_t print(long) { return 0; }
  virtual size_t print(int) { return 0; }
  virtual size_t print(char) { return 0; }
  virtual size_t print(bool) { return 0; }
  virtual size_t println() { return 0; }
  virtual size_t println(const char *) { return 0; }
  virtual size_t println(const String &) { return 0; }
  virtual size_t println(unsigned long) { return 0; }
  virtual size_t println(unsigned int) { return 0; }
  virtual size_t println(long) { return 0; }
  virtual size_t println(int) { return 0; }
  virtual size_t println(bool) { return 0; }
};

class HardwareSerial : public Print {
public:
  void begin(unsigned long) {}
  void flush() {}
};

extern HardwareSerial Serial;

inline unsigned long millis() { return 0; }
inline unsigned long micros() { return 0; }
inline void delay(unsigned long) {}
inline void delayMicroseconds(unsigned int) {}
inline void yield() {}

inline char *ultoa(unsigned long value, char *buffer, int radix) {
  if (radix == 10) {
    std::snprintf(buffer, 32, "%lu", value);
  } else {
    buffer[0] = '\0';
  }
  return buffer;
}
