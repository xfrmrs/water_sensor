#include "common.h"

bool jsonVarToUnsignedLong(const JSONVar &value, unsigned long &parsedValue) {
  String type = JSON.typeof(value);
  if (type == "number") {
    // Check if there's a problem first by catching invalid format strings
    // when using testing stubs
    #ifndef ARDUINO
    try {
      parsedValue = (unsigned long)value;
    } catch (...) {
      return false;
    }
    #else
    parsedValue = (unsigned long)value;
    #endif
    return true;
  }

  if (type == "string") {
    const char *strValue = (const char *)value;
    char *endPtr = nullptr;
    unsigned long parsed = strtoul(strValue, &endPtr, 10);
    if (endPtr == strValue || *endPtr != '\0') {
      return false;
    }
    parsedValue = parsed;
    return true;
  }

  return false;
}

bool jsonVarToUint8(const JSONVar &value, uint8_t &parsedValue) {
  unsigned long parsed = 0;
  if (!jsonVarToUnsignedLong(value, parsed) || parsed > 255UL) {
    return false;
  }

  parsedValue = (uint8_t)parsed;
  return true;
}

bool jsonVarToString(const JSONVar &value, String &parsedValue) {
  if (JSON.typeof(value) != "string") {
    return false;
  }

  parsedValue = String((const char *)value);
  return true;
}

bool jsonVarToBool(const JSONVar &value, bool &parsedValue) {
  String type = JSON.typeof(value);
  if (type == "boolean") {
    parsedValue = JSON.stringify(value) == "true";
    return true;
  }

  if (type == "number") {
    unsigned long parsed = 0;
    if (!jsonVarToUnsignedLong(value, parsed)) {
      return false;
    }

    parsedValue = parsed != 0;
    return true;
  }

  return false;
}

bool jsonVarToFloat(const JSONVar &value, float &parsedValue) {
  String type = JSON.typeof(value);
  if (type == "number") {
    parsedValue = (double)value;
    return true;
  }

  if (type == "string") {
    const char *strValue = (const char *)value;
    char *endPtr = nullptr;
    float parsed = strtof(strValue, &endPtr);
    if (endPtr == strValue || *endPtr != '\0') {
      return false;
    }
    parsedValue = parsed;
    return true;
  }

  return false;
}

bool requireUnsignedLongField(const JSONVar &json, const char *name, unsigned long &target) {
  if (!json.hasOwnProperty(name)) {
    return false;
  }
  return jsonVarToUnsignedLong(json[name], target);
}

bool requireUint8Field(const JSONVar &json, const char *name, uint8_t &target) {
  if (!json.hasOwnProperty(name)) {
    return false;
  }
  return jsonVarToUint8(json[name], target);
}

bool requireStringField(const JSONVar &json, const char *name, String &target) {
  if (!json.hasOwnProperty(name)) {
    return false;
  }
  return jsonVarToString(json[name], target);
}

bool requireBoolField(const JSONVar &json, const char *name, bool &target) {
  if (!json.hasOwnProperty(name)) {
    return false;
  }
  return jsonVarToBool(json[name], target);
}

bool requireFloatField(const JSONVar &json, const char *name, float &target) {
  if (!json.hasOwnProperty(name)) {
    return false;
  }
  return jsonVarToFloat(json[name], target);
}

bool optionalUnsignedLongField(const JSONVar &json, const char *name, unsigned long &target) {
  if (!json.hasOwnProperty(name)) {
    return true;
  }
  return jsonVarToUnsignedLong(json[name], target);
}

bool optionalUint8Field(const JSONVar &json, const char *name, uint8_t &target) {
  if (!json.hasOwnProperty(name)) {
    return true;
  }
  return jsonVarToUint8(json[name], target);
}

bool optionalStringField(const JSONVar &json, const char *name, String &target) {
  if (!json.hasOwnProperty(name)) {
    return true;
  }
  return jsonVarToString(json[name], target);
}

bool optionalBoolField(const JSONVar &json, const char *name, bool &target) {
  if (!json.hasOwnProperty(name)) {
    return true;
  }
  return jsonVarToBool(json[name], target);
}

bool optionalFloatField(const JSONVar &json, const char *name, float &target) {
  if (!json.hasOwnProperty(name)) {
    return true;
  }
  return jsonVarToFloat(json[name], target);
}

static void writeJsonToStringCString(void *context, const char *value) {
  String *target = (String *)context;
  (*target) += value;
}

static void writeJsonToStringString(void *context, const String &value) {
  String *target = (String *)context;
  (*target) += value;
}

static void writeJsonToHttpCString(void *context, const char *value) {
  (void)context;
  if (server) {
    server->sendContent(value);
  }
}

static void writeJsonToHttpString(void *context, const String &value) {
  (void)context;
  if (server) {
    server->sendContent(value);
  }
}

JsonOutput makeStringJsonOutput(String &target) {
  JsonOutput output = {
    &target,
    writeJsonToStringCString,
    writeJsonToStringString
  };
  return output;
}

JsonOutput makeHttpJsonOutput() {
  JsonOutput output = {
    nullptr,
    writeJsonToHttpCString,
    writeJsonToHttpString
  };
  return output;
}

void writeJsonValue(JsonOutput &output, JsonFieldType type, const void *value, uint8_t decimals = 0) {
  switch (type) {
    case JSON_FIELD_BOOL:
      jsonWrite(output, *(const bool *)value ? "true" : "false");
      break;
    case JSON_FIELD_UINT:
      jsonWrite(output, String(*(const unsigned int *)value));
      break;
    case JSON_FIELD_ULONG:
      jsonWrite(output, String(*(const unsigned long *)value));
      break;
    case JSON_FIELD_FLOAT:
      jsonWrite(output, String(*(const float *)value, decimals));
      break;
    case JSON_FIELD_STRING:
      jsonWrite(output, "\"");
      writeEscapedJsonString(output, *(const String *)value);
      jsonWrite(output, "\"");
      break;
  }
}

inline void jsonWrite(JsonOutput &output, const char *value) {
  output.writeCString(output.context, value);
}

inline void jsonWrite(JsonOutput &output, const String &value) {
  output.writeString(output.context, value);
}

static void flushEscapedJsonBuffer(JsonOutput &output, char *buffer, uint8_t &used) {
  if (used == 0) {
    return;
  }

  buffer[used] = '\0';
  jsonWrite(output, buffer);
  used = 0;
}

static void writeEscapedJsonInternal(JsonOutput &output, const char *value, size_t length) {
  char buffer[24];
  uint8_t used = 0;

  for (size_t i = 0; i < length; ++i) {
    const char current = value[i];
    const char *escapeSequence = nullptr;

    switch (current) {
      case '\\':
        escapeSequence = "\\\\";
        break;
      case '"':
        escapeSequence = "\\\"";
        break;
      case '\n':
        escapeSequence = "\\n";
        break;
      case '\r':
        escapeSequence = "\\r";
        break;
      case '\t':
        escapeSequence = "\\t";
        break;
      default:
        break;
    }

    if (escapeSequence != nullptr) {
      flushEscapedJsonBuffer(output, buffer, used);
      jsonWrite(output, escapeSequence);
      continue;
    }

    buffer[used++] = current;
    if (used >= (sizeof(buffer) - 1)) {
      flushEscapedJsonBuffer(output, buffer, used);
    }
  }

  flushEscapedJsonBuffer(output, buffer, used);
}

void writeEscapedJsonString(JsonOutput &output, const String &value) {
  writeEscapedJsonInternal(output, value.c_str(), value.length());
}

void writeEscapedJsonCString(JsonOutput &output, const char *value) {
  writeEscapedJsonInternal(output, value, strlen(value));
}

void writeJsonFieldPrefix(JsonOutput &output, bool &first, const char *key) {
  if (!first) {
    jsonWrite(output, ",");
  }
  first = false;
  jsonWrite(output, "\"");
  jsonWrite(output, key);
  jsonWrite(output, "\":");
}

void writeJsonStringField(JsonOutput &output, bool &first, const char *key, const char *value) {
  writeJsonFieldPrefix(output, first, key);
  jsonWrite(output, "\"");
  writeEscapedJsonCString(output, value);
  jsonWrite(output, "\"");
}

void writeJsonStringField(JsonOutput &output, bool &first, const char *key, const String &value) {
  writeJsonFieldPrefix(output, first, key);
  jsonWrite(output, "\"");
  writeEscapedJsonString(output, value);
  jsonWrite(output, "\"");
}

void writeJsonBoolField(JsonOutput &output, bool &first, const char *key, bool value) {
  writeJsonFieldPrefix(output, first, key);
  writeJsonValue(output, JSON_FIELD_BOOL, &value);
}

void writeJsonULongField(JsonOutput &output, bool &first, const char *key, unsigned long value) {
  writeJsonFieldPrefix(output, first, key);
  writeJsonValue(output, JSON_FIELD_ULONG, &value);
}

void writeJsonUIntField(JsonOutput &output, bool &first, const char *key, unsigned int value) {
  writeJsonFieldPrefix(output, first, key);
  writeJsonValue(output, JSON_FIELD_UINT, &value);
}

void writeJsonFloatField(JsonOutput &output, bool &first, const char *key, float value, uint8_t decimals) {
  writeJsonFieldPrefix(output, first, key);
  writeJsonValue(output, JSON_FIELD_FLOAT, &value, decimals);
}

void writeJsonArrayStringValue(JsonOutput &output, bool &first, const char *value) {
  if (!first) {
    jsonWrite(output, ",");
  }
  first = false;
  jsonWrite(output, "\"");
  writeEscapedJsonCString(output, value);
  jsonWrite(output, "\"");
}

void writeJsonArrayULongValue(JsonOutput &output, bool &first, unsigned long value) {
  char buffer[16];
  ultoa(value, buffer, 10);

  if (!first) {
    jsonWrite(output, ",");
  }
  first = false;
  jsonWrite(output, buffer);
}
