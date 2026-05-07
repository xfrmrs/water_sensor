#pragma once
#include "Arduino.h"

struct LittleFSMockState {
    bool openResult = false;
    size_t printResult = 0;
    bool existsResult = false;
    bool removeResult = true;
    bool renameResult = true;

    int openCount = 0;
    int printCount = 0;
    int existsCount = 0;
    int removeCount = 0;
    int renameCount = 0;

    String lastOpenedPath;
    String lastOpenedMode;
    String lastPrintedData;
    String lastRemovedPath;
    String lastRenamedFrom;
    String lastRenamedTo;

    void reset() {
        openResult = false;
        printResult = 0;
        existsResult = false;
        removeResult = true;
        renameResult = true;

        openCount = 0;
        printCount = 0;
        existsCount = 0;
        removeCount = 0;
        renameCount = 0;

        lastOpenedPath = "";
        lastOpenedMode = "";
        lastPrintedData = "";
        lastRemovedPath = "";
        lastRenamedFrom = "";
        lastRenamedTo = "";
    }
};

extern LittleFSMockState littleFsMockState;

class File {
public:
    operator bool() const { return littleFsMockState.openResult; }
    String readString() { return String(""); }
    void close() {}
    size_t print(const String& data) {
        littleFsMockState.printCount++;
        littleFsMockState.lastPrintedData = data;
        if (littleFsMockState.printResult == (size_t)-1) {
            return data.length();
        }
        return littleFsMockState.printResult;
    }
};

class LittleFSMock {
public:
    bool begin() { return true; }
    bool exists(const char* path) {
        littleFsMockState.existsCount++;
        return littleFsMockState.existsResult;
    }
    File open(const char* path, const char* mode) {
        littleFsMockState.openCount++;
        littleFsMockState.lastOpenedPath = path;
        littleFsMockState.lastOpenedMode = mode;
        return File();
    }
    bool remove(const char* path) {
        littleFsMockState.removeCount++;
        littleFsMockState.lastRemovedPath = path;
        return littleFsMockState.removeResult;
    }
    bool rename(const char* from, const char* to) {
        littleFsMockState.renameCount++;
        littleFsMockState.lastRenamedFrom = from;
        littleFsMockState.lastRenamedTo = to;
        return littleFsMockState.renameResult;
    }
};

extern LittleFSMock LittleFS;
