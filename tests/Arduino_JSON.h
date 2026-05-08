#pragma once

#include <map>
#include <sstream>
#include <string>

#include "Arduino.h"

class JSONVar {
public:
  enum Type {
    TYPE_UNDEFINED,
    TYPE_STRING,
    TYPE_NUMBER,
    TYPE_BOOLEAN,
    TYPE_OBJECT
  };

  JSONVar() : type(TYPE_UNDEFINED), numberValue(0.0), boolValue(false) {}
  JSONVar(const char *text) : type(TYPE_STRING), stringValue(text ? text : ""), numberValue(0.0), boolValue(false) {}
  JSONVar(const String &text) : type(TYPE_STRING), stringValue(text.c_str()), numberValue(0.0), boolValue(false) {}
  JSONVar(int number) : type(TYPE_NUMBER), numberValue((double)number), boolValue(false) {}
  JSONVar(unsigned long number) : type(TYPE_NUMBER), numberValue((double)number), boolValue(false) {}
  JSONVar(double number) : type(TYPE_NUMBER), numberValue(number), boolValue(false) {}
  JSONVar(bool booleanValue) : type(TYPE_BOOLEAN), numberValue(0.0), boolValue(booleanValue) {}

  bool hasOwnProperty(const char *name) const {
    return type == TYPE_OBJECT && properties.find(name ? name : "") != properties.end();
  }

  const JSONVar &operator[](const char *name) const {
    static JSONVar undefinedValue;
    if (type != TYPE_OBJECT) {
      return undefinedValue;
    }
    auto it = properties.find(name ? name : "");
    return it == properties.end() ? undefinedValue : it->second;
  }

  JSONVar &operator[](const char *name) {
    if (type != TYPE_OBJECT) {
      type = TYPE_OBJECT;
      properties.clear();
    }
    return properties[name ? name : ""];
  }

  explicit operator const char *() const {
    if (type == TYPE_STRING) {
      return stringValue.c_str();
    }
    cachedString = render();
    return cachedString.c_str();
  }

  explicit operator unsigned long() const {
    return (unsigned long)numberValue;
  }

  explicit operator double() const {
    return numberValue;
  }

  explicit operator bool() const {
    return boolValue;
  }

  Type type;
  std::string stringValue;
  double numberValue;
  bool boolValue;
  std::map<std::string, JSONVar> properties;

private:
  std::string render() const {
    switch (type) {
      case TYPE_STRING:
        return stringValue;
      case TYPE_NUMBER: {
        std::ostringstream stream;
        stream << numberValue;
        return stream.str();
      }
      case TYPE_BOOLEAN:
        return boolValue ? "true" : "false";
      case TYPE_OBJECT:
        return "[object]";
      case TYPE_UNDEFINED:
      default:
        return "";
    }
  }

  mutable std::string cachedString;
};

class JSONClass {
public:
  String stringify(const JSONVar &value) const {
    switch (value.type) {
      case JSONVar::TYPE_BOOLEAN:
        return value.boolValue ? "true" : "false";
      case JSONVar::TYPE_NUMBER: {
        std::ostringstream stream;
        stream << value.numberValue;
        return String(stream.str());
      }
      case JSONVar::TYPE_STRING:
        return String(value.stringValue);
      case JSONVar::TYPE_OBJECT:
        return String("[object]");
      case JSONVar::TYPE_UNDEFINED:
      default:
        return String();
    }
  }

  String typeof_(const JSONVar &value) const {
    switch (value.type) {
      case JSONVar::TYPE_BOOLEAN:
        return "boolean";
      case JSONVar::TYPE_NUMBER:
        return "number";
      case JSONVar::TYPE_STRING:
        return "string";
      case JSONVar::TYPE_OBJECT:
        return "object";
      case JSONVar::TYPE_UNDEFINED:
      default:
        return "undefined";
    }
  }
};

#define typeof typeof_

extern JSONClass JSON;
