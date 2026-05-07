#include <iostream>
#include <string>
#include <cassert>

// Mock String class for testing Arduino code
class String {
public:
    std::string str;
    String() {}
    String(const char* s) : str(s) {}
    String(const std::string& s) : str(s) {}
    String& operator+=(const String& other) {
        str += other.str;
        return *this;
    }
    String& operator+=(const char* other) {
        str += other;
        return *this;
    }
    bool operator==(const String& other) const {
        return str == other.str;
    }
    bool operator==(const char* other) const {
        return str == std::string(other);
    }
};

std::ostream& operator<<(std::ostream& os, const String& s) {
    os << s.str;
    return os;
}

#define F(x) x

struct Config {
    String wifiStaIp;
    String wifiApSsid;
    String wifiApIp;
};

// Target function to test
String buildReconnectHint(const Config &targetConfig) {
  String hint = F("Reconnect to ");
  hint += targetConfig.wifiStaIp;
  hint += F(" if the station join succeeds, or to setup AP ");
  hint += targetConfig.wifiApSsid;
  hint += F(" at ");
  hint += targetConfig.wifiApIp;
  hint += F(" if it falls back.");
  return hint;
}

// Simple test framework
int tests_run = 0;
int tests_passed = 0;

void run_test(const std::string& name, void (*test_func)()) {
    std::cout << "Running " << name << "... ";
    try {
        test_func();
        std::cout << "PASSED" << std::endl;
        tests_passed++;
    } catch (const std::exception& e) {
        std::cout << "FAILED: " << e.what() << std::endl;
    } catch (...) {
        std::cout << "FAILED: Unknown exception" << std::endl;
    }
    tests_run++;
}

#define ASSERT_EQUAL(expected, actual) \
    if (!((expected) == (actual))) { \
        std::cerr << "\n  Expected: '" << (expected) << "'\n  Actual:   '" << (actual) << "'\n"; \
        throw std::runtime_error("Assertion failed"); \
    }

void test_buildReconnectHint_StandardValues() {
    Config config;
    config.wifiStaIp = "192.168.1.100";
    config.wifiApSsid = "WaterSensorSetup";
    config.wifiApIp = "10.0.0.47";

    String result = buildReconnectHint(config);
    String expected = "Reconnect to 192.168.1.100 if the station join succeeds, or to setup AP WaterSensorSetup at 10.0.0.47 if it falls back.";

    ASSERT_EQUAL(expected, result);
}

void test_buildReconnectHint_EmptyValues() {
    Config config;
    config.wifiStaIp = "";
    config.wifiApSsid = "";
    config.wifiApIp = "";

    String result = buildReconnectHint(config);
    String expected = "Reconnect to  if the station join succeeds, or to setup AP  at  if it falls back.";

    ASSERT_EQUAL(expected, result);
}

void test_buildReconnectHint_SpecialCharacters() {
    Config config;
    config.wifiStaIp = "10.0.0.1";
    config.wifiApSsid = "My-Awesome_AP!@#";
    config.wifiApIp = "172.16.0.1";

    String result = buildReconnectHint(config);
    String expected = "Reconnect to 10.0.0.1 if the station join succeeds, or to setup AP My-Awesome_AP!@# at 172.16.0.1 if it falls back.";

    ASSERT_EQUAL(expected, result);
}

int main() {
    std::cout << "--- Testing buildReconnectHint ---" << std::endl;

    run_test("test_buildReconnectHint_StandardValues", test_buildReconnectHint_StandardValues);
    run_test("test_buildReconnectHint_EmptyValues", test_buildReconnectHint_EmptyValues);
    run_test("test_buildReconnectHint_SpecialCharacters", test_buildReconnectHint_SpecialCharacters);

    std::cout << "----------------------------------" << std::endl;
    std::cout << "Tests run: " << tests_run << ", Passed: " << tests_passed << std::endl;

    return tests_passed == tests_run ? 0 : 1;
}
