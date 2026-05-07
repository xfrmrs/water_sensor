#pragma once
#include "Arduino.h"

struct LittleFSMockState {
    bool fileSystemReady = false;
    bool existsReturn = false;
    bool openReturn = true;
    bool removeReturn = true;
    bool renameReturn = true;
    size_t writeReturnBytes = 0;
    bool returnExactWriteLength = true;

    String lastOpenedPath;
    String lastOpenedMode;
    String lastRemovedPath;
    String lastRenameFrom;
    String lastRenameTo;
    bool closeCalled = false;
    String writtenData;

    void reset() {
        fileSystemReady = true;
        existsReturn = false;
        openReturn = true;
        removeReturn = true;
        renameReturn = true;
        writeReturnBytes = 0;
        returnExactWriteLength = true;

        lastOpenedPath = "";
        lastOpenedMode = "";
        lastRemovedPath = "";
        lastRenameFrom = "";
        lastRenameTo = "";
        closeCalled = false;
        writtenData = "";
    }
};

extern LittleFSMockState littleFsMockState;

class File {
public:
    operator bool() const { return littleFsMockState.openReturn; }
    String readString() { return String(""); }
    void close() { littleFsMockState.closeCalled = true; }
    size_t print(const String& s) {
        littleFsMockState.writtenData += s;
        if (littleFsMockState.returnExactWriteLength) {
            return s.length();
        }
        return littleFsMockState.writeReturnBytes;
    }
};

class LittleFSMock {
public:
    bool begin() { return littleFsMockState.fileSystemReady; }
    bool exists(const char*) { return littleFsMockState.existsReturn; }
    File open(const char* path, const char* mode) {
        littleFsMockState.lastOpenedPath = path;
        littleFsMockState.lastOpenedMode = mode;
        return File();
    }
    bool remove(const char* path) {
        littleFsMockState.lastRemovedPath = path;
        return littleFsMockState.removeReturn;
    }
    bool rename(const char* from, const char* to) {
        littleFsMockState.lastRenameFrom = from;
        littleFsMockState.lastRenameTo = to;
        return littleFsMockState.renameReturn;
    }
};

extern LittleFSMock LittleFS;
