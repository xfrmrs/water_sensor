bool jsonVarToUnsignedLong(const JSONVar &value, unsigned long &parsedValue) {
  String rendered = JSON.stringify(value);
  char *endPtr = nullptr;
  unsigned long parsed = strtoul(rendered.c_str(), &endPtr, 10);

  if (endPtr == rendered.c_str() || *endPtr != '\0') {
    return false;
  }

  parsedValue = parsed;
  return true;
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
  String rendered = JSON.stringify(value);
  char *endPtr = nullptr;
  float parsed = strtof(rendered.c_str(), &endPtr);

  if (endPtr == rendered.c_str() || *endPtr != '\0') {
    return false;
  }

  parsedValue = parsed;
  return true;
}

bool requireUnsignedLongField(const JSONVar &json, const char *name, unsigned long &target) {
  return json.hasOwnProperty(name) && jsonVarToUnsignedLong(json[name], target);
}

bool requireUint8Field(const JSONVar &json, const char *name, uint8_t &target) {
  return json.hasOwnProperty(name) && jsonVarToUint8(json[name], target);
}

bool requireStringField(const JSONVar &json, const char *name, String &target) {
  return json.hasOwnProperty(name) && jsonVarToString(json[name], target);
}

bool requireBoolField(const JSONVar &json, const char *name, bool &target) {
  return json.hasOwnProperty(name) && jsonVarToBool(json[name], target);
}

bool requireFloatField(const JSONVar &json, const char *name, float &target) {
  return json.hasOwnProperty(name) && jsonVarToFloat(json[name], target);
}

bool optionalUnsignedLongField(const JSONVar &json, const char *name, unsigned long &target) {
  return !json.hasOwnProperty(name) || jsonVarToUnsignedLong(json[name], target);
}

bool optionalUint8Field(const JSONVar &json, const char *name, uint8_t &target) {
  return !json.hasOwnProperty(name) || jsonVarToUint8(json[name], target);
}

bool optionalStringField(const JSONVar &json, const char *name, String &target) {
  return !json.hasOwnProperty(name) || jsonVarToString(json[name], target);
}

bool optionalBoolField(const JSONVar &json, const char *name, bool &target) {
  return !json.hasOwnProperty(name) || jsonVarToBool(json[name], target);
}

bool optionalFloatField(const JSONVar &json, const char *name, float &target) {
  return !json.hasOwnProperty(name) || jsonVarToFloat(json[name], target);
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
  server.sendContent(value);
}

static void writeJsonToHttpString(void *context, const String &value) {
  (void)context;
  server.sendContent(value);
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

void writeEscapedJsonCString(JsonOutput &output, const char *value) {
  char buffer[24];
  uint8_t used = 0;

  while (*value != '\0') {
    const char current = *value++;
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
  writeEscapedJsonCString(output, value.c_str());
  jsonWrite(output, "\"");
}

void writeJsonBoolField(JsonOutput &output, bool &first, const char *key, bool value) {
  writeJsonFieldPrefix(output, first, key);
  jsonWrite(output, value ? "true" : "false");
}

void writeJsonULongField(JsonOutput &output, bool &first, const char *key, unsigned long value) {
  char buffer[16];
  ultoa(value, buffer, 10);
  writeJsonFieldPrefix(output, first, key);
  jsonWrite(output, buffer);
}

void writeJsonFloatField(JsonOutput &output, bool &first, const char *key, float value, uint8_t decimals) {
  char buffer[24];
  dtostrf((double)value, 1, decimals, buffer);
  writeJsonFieldPrefix(output, first, key);
  jsonWrite(output, buffer);
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
