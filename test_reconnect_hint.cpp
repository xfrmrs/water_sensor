#include <iostream>
#include <string>

// Mock String class for Arduino
class String {
public:
    std::string str;
    String() {}
    String(const char* s) : str(s) {}
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

void test_buildReconnectHint() {
    Config config;
    config.wifiStaIp = "192.168.1.100";
    config.wifiApSsid = "WaterSensorSetup";
    config.wifiApIp = "10.0.0.47";

    String result = buildReconnectHint(config);
    String expected = "Reconnect to 192.168.1.100 if the station join succeeds, or to setup AP WaterSensorSetup at 10.0.0.47 if it falls back.";

    if (result == expected) {
        std::cout << "Test passed!" << std::endl;
    } else {
        std::cout << "Test failed!" << std::endl;
        std::cout << "Expected: " << expected << std::endl;
        std::cout << "Got: " << result << std::endl;
    }
}

int main() {
    test_buildReconnectHint();
    return 0;
}
