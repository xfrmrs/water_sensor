#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <cstring>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdexcept>

// Test macro for simple C++ test framework
#define TEST(name) void name(); int name##_register = (tests.push_back({#name, name}), 0); void name()
#define ASSERT_TRUE(condition) if (!(condition)) throw std::runtime_error("Assertion failed: " #condition)

struct TestCase {
    const char* name;
    void (*func)();
};
std::vector<TestCase> tests;

// Mocks for Arduino and Arduino_JSON classes to compile locally
class String {
public:
    std::string str;
    String() : str("") {}
    String(const char* s) : str(s) {}
    String(const std::string& s) : str(s) {}
    String(unsigned int i) : str(std::to_string(i)) {}
    String(unsigned long i) : str(std::to_string(i)) {}
    String(float f, uint8_t decimals = 0) : str(std::to_string(f)) { (void)decimals; }
    bool operator==(const char* other) const { return str == other; }
    bool operator!=(const char* other) const { return str != other; }
    const char* c_str() const { return str.c_str(); }
    size_t length() const { return str.length(); }
    String& operator+=(const char* s) { str += s; return *this; }
    String& operator+=(const String& s) { str += s.str; return *this; }
};

class JSONVar {
public:
    std::string type_val;
    std::string str_val;
    JSONVar(const char* type, const char* str) : type_val(type), str_val(str) {}
    bool hasOwnProperty(const char* name) const { (void)name; return false; }
    JSONVar operator[](const char* name) const { (void)name; return JSONVar("undefined", "undefined"); }
    explicit operator const char*() const { return str_val.c_str(); }
};

class JSON_Class {
public:
    // Workaround because typeof is a GCC keyword
    String typeof_(const JSONVar& value) {
        return String(value.type_val);
    }
    String stringify(const JSONVar& value) {
        return String(value.str_val);
    }
};

JSON_Class JSON;

#define typeof typeof_

class ESP8266WebServer {
public:
    void sendContent(const char* content) { (void)content; }
    void sendContent(const String& content) { (void)content; }
};
ESP8266WebServer* server = nullptr;

// utoa replacement for linux
char* ultoa(unsigned long value, char* str, int base) {
    if (base == 10) {
        sprintf(str, "%lu", value);
    }
    return str;
}

// Ensure SimpleKalmanFilter exists
class SimpleKalmanFilter {
public:
    SimpleKalmanFilter(float, float, float) {}
};
SimpleKalmanFilter* kalmanFilter = nullptr;

#include "common.h"

// Forward declare these before including json_helpers.ino
inline void jsonWrite(JsonOutput &output, const char *value);
inline void jsonWrite(JsonOutput &output, const String &value);
void writeEscapedJsonString(JsonOutput &output, const String &value);

#include "json_helpers.ino"

// --- Tests ---

TEST(test_boolean_true) {
    bool parsed = false;
    bool success = jsonVarToBool(JSONVar("boolean", "true"), parsed);
    ASSERT_TRUE(success == true);
    ASSERT_TRUE(parsed == true);
}

TEST(test_boolean_false) {
    bool parsed = true;
    bool success = jsonVarToBool(JSONVar("boolean", "false"), parsed);
    ASSERT_TRUE(success == true);
    ASSERT_TRUE(parsed == false);
}

TEST(test_number_one) {
    bool parsed = false;
    bool success = jsonVarToBool(JSONVar("number", "1"), parsed);
    ASSERT_TRUE(success == true);
    ASSERT_TRUE(parsed == true);
}

TEST(test_number_zero) {
    bool parsed = true;
    bool success = jsonVarToBool(JSONVar("number", "0"), parsed);
    ASSERT_TRUE(success == true);
    ASSERT_TRUE(parsed == false);
}

TEST(test_number_invalid_format) {
    bool parsed = true;
    bool success = jsonVarToBool(JSONVar("number", "1.5"), parsed);
    ASSERT_TRUE(success == false);
    // When parsing fails, parsedValue shouldn't change
    ASSERT_TRUE(parsed == true);
}

TEST(test_string_type) {
    bool parsed = true;
    bool success = jsonVarToBool(JSONVar("string", "true"), parsed);
    ASSERT_TRUE(success == false);
    ASSERT_TRUE(parsed == true);
}

TEST(test_undefined_type) {
    bool parsed = true;
    bool success = jsonVarToBool(JSONVar("undefined", "undefined"), parsed);
    ASSERT_TRUE(success == false);
    ASSERT_TRUE(parsed == true);
}

int main() {
    int failed = 0;
    for (const auto& test : tests) {
        std::cout << "Running " << test.name << "... ";
        try {
            test.func();
            std::cout << "PASSED\n";
        } catch (const std::exception& e) {
            std::cout << "FAILED: " << e.what() << "\n";
            failed++;
        } catch (...) {
            std::cout << "FAILED: Unknown exception\n";
            failed++;
        }
    }
    if (failed == 0) {
        std::cout << "All tests passed!\n";
    } else {
        std::cout << failed << " tests failed!\n";
    }
    return failed;
}
