#include <cassert>
#include <iostream>

#define TEST_SUPPORT_IMPLEMENTATION
#include "test_support.h"

void writeEscapedJsonString(JsonOutput &output, const String &value);

#include "../json_helpers.ino"

static void testJsonVarToUint8() {
  uint8_t parsed = 0;

  assert(jsonVarToUint8(JSONVar("123"), parsed));
  assert(parsed == 123);

  assert(jsonVarToUint8(JSONVar("0"), parsed));
  assert(parsed == 0);

  assert(jsonVarToUint8(JSONVar("255"), parsed));
  assert(parsed == 255);

  assert(!jsonVarToUint8(JSONVar("256"), parsed));
  assert(!jsonVarToUint8(JSONVar("-1"), parsed));
  assert(!jsonVarToUint8(JSONVar("12.3"), parsed));
  assert(!jsonVarToUint8(JSONVar("abc"), parsed));
}

static void testJsonVarToUnsignedLong() {
  unsigned long parsed = 0;

  assert(jsonVarToUnsignedLong(JSONVar(42UL), parsed));
  assert(parsed == 42UL);

  assert(jsonVarToUnsignedLong(JSONVar("9000"), parsed));
  assert(parsed == 9000UL);

  assert(!jsonVarToUnsignedLong(JSONVar("bad"), parsed));
}

static void testJsonVarToString() {
  String parsed;

  assert(jsonVarToString(JSONVar("station"), parsed));
  assert(parsed == "station");

  assert(!jsonVarToString(JSONVar(17), parsed));
  assert(!jsonVarToString(JSONVar(true), parsed));
}

static void testJsonVarToBool() {
  bool parsed = false;

  assert(jsonVarToBool(JSONVar(true), parsed));
  assert(parsed);

  assert(jsonVarToBool(JSONVar(false), parsed));
  assert(!parsed);

  assert(jsonVarToBool(JSONVar(1), parsed));
  assert(parsed);

  assert(jsonVarToBool(JSONVar(0), parsed));
  assert(!parsed);

  assert(!jsonVarToBool(JSONVar("true"), parsed));
}

static void testJsonVarToFloat() {
  float parsed = 0.0f;

  assert(jsonVarToFloat(JSONVar(3.5), parsed));
  assert(parsed > 3.4f && parsed < 3.6f);

  assert(jsonVarToFloat(JSONVar("7.25"), parsed));
  assert(parsed > 7.2f && parsed < 7.3f);

  assert(!jsonVarToFloat(JSONVar("NaNish"), parsed));
}

int main() {
  testJsonVarToUint8();
  testJsonVarToUnsignedLong();
  testJsonVarToString();
  testJsonVarToBool();
  testJsonVarToFloat();

  std::cout << "json_helpers tests passed\n";
  return 0;
}
