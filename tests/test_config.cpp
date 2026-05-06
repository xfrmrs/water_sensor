#include <iostream>
#include <string>
#include <cassert>
#include <cstdint>

// Mocking Arduino structures
using String = std::string;

struct IPAddress {
    uint8_t bytes[4];
    bool isSet = false;

    IPAddress() {
        bytes[0] = 0; bytes[1] = 0; bytes[2] = 0; bytes[3] = 0;
    }

    bool fromString(const String& s) {
        // Mock simple implementation
        int parsedBytes[4] = {0};
        int parsedCount = 0;
        std::string currentNum = "";

        for (char c : s) {
            if (c == '.') {
                if (currentNum.empty() || parsedCount >= 3) return false;
                try { parsedBytes[parsedCount] = std::stoi(currentNum); } catch (...) { return false; }
                if (parsedBytes[parsedCount] < 0 || parsedBytes[parsedCount] > 255) return false;
                currentNum = "";
                parsedCount++;
            } else if (isdigit(c)) {
                currentNum += c;
            } else {
                return false;
            }
        }

        if (currentNum.empty() || parsedCount != 3) return false;
        try { parsedBytes[parsedCount] = std::stoi(currentNum); } catch (...) { return false; }
        if (parsedBytes[parsedCount] < 0 || parsedBytes[parsedCount] > 255) return false;

        bytes[0] = parsedBytes[0];
        bytes[1] = parsedBytes[1];
        bytes[2] = parsedBytes[2];
        bytes[3] = parsedBytes[3];
        isSet = true;
        return true;
    }
};

// Mock Arduino.h to prevent ip_utils.h from failing
// No need to include anything else

#include "../ip_utils.h"

// Provide the implementation to test directly since we mocked the dependencies
#include "../ip_utils.cpp"

// Tests
void test_parseIpAddressString_ValidIP() {
    IPAddress parsed;
    parsed.bytes[0] = 99; parsed.bytes[1] = 99; parsed.bytes[2] = 99; parsed.bytes[3] = 99;

    bool result = parseIpAddressString("192.168.1.1", parsed);
    assert(result == true);
    assert(parsed.bytes[0] == 192);
    assert(parsed.bytes[1] == 168);
    assert(parsed.bytes[2] == 1);
    assert(parsed.bytes[3] == 1);
    assert(parsed.isSet == true);
}

void test_parseIpAddressString_ValidIP_EdgeCases() {
    IPAddress parsed;

    bool result = parseIpAddressString("255.255.255.255", parsed);
    assert(result == true);
    assert(parsed.bytes[0] == 255 && parsed.bytes[1] == 255 && parsed.bytes[2] == 255 && parsed.bytes[3] == 255);

    result = parseIpAddressString("0.0.0.0", parsed);
    assert(result == true);
    assert(parsed.bytes[0] == 0 && parsed.bytes[1] == 0 && parsed.bytes[2] == 0 && parsed.bytes[3] == 0);
}

void test_parseIpAddressString_InvalidIP() {
    IPAddress parsed;
    parsed.bytes[0] = 99; parsed.bytes[1] = 99; parsed.bytes[2] = 99; parsed.bytes[3] = 99;

    // Test various invalid strings
    const char* invalid_strings[] = {
        "invalid-ip",
        "256.1.1.1",
        "192.168.1",
        "192.168.1.1.1",
        "192.168..1",
        "192.168.1.a",
        "",
        " "
    };

    for (const char* invalid_str : invalid_strings) {
        IPAddress p = parsed;
        bool result = parseIpAddressString(invalid_str, p);
        assert(result == false);
        // Values should remain unchanged if parsing fails
        assert(p.bytes[0] == 99);
        assert(p.bytes[1] == 99);
        assert(p.bytes[2] == 99);
        assert(p.bytes[3] == 99);
        assert(p.isSet == false);
    }
}

int main() {
    std::cout << "Running tests for parseIpAddressString...\n";

    test_parseIpAddressString_ValidIP();
    test_parseIpAddressString_ValidIP_EdgeCases();
    test_parseIpAddressString_InvalidIP();

    std::cout << "All tests passed successfully.\n";
    return 0;
}
