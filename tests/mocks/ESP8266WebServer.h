#pragma once
class ESP8266WebServer {
public:
    String arg(const char*) { return ""; }
    void sendContent(const char* content) {}
    void sendContent(const String& content) {}
};
