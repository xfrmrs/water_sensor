#pragma once
#include "Arduino.h"

// Define function pointer types for mock injections
typedef bool (*LittleFSMockExistsFn)(const char*);
typedef bool (*LittleFSMockRemoveFn)(const char*);
typedef bool (*LittleFSMockRenameFn)(const char*, const char*);

class File {
public:
    bool isOpen = false;
    size_t writeLength = 0;

    operator bool() const { return isOpen; }
    String readString() { return String(""); }
    void close() { isOpen = false; }
    size_t print(const String& str) {
        writeLength = str.length();
        return writeLength;
    }
};

class LittleFSMock {
public:
    bool beginReturnValue = true;
    LittleFSMockExistsFn existsFn = nullptr;
    LittleFSMockRemoveFn removeFn = nullptr;
    LittleFSMockRenameFn renameFn = nullptr;
    bool openReturnValue = true;

    bool begin() { return beginReturnValue; }

    bool exists(const char* path) {
        if (existsFn) return existsFn(path);
        return false;
    }

    File open(const char*, const char*) {
        File f;
        f.isOpen = openReturnValue;
        return f;
    }

    bool remove(const char* path) {
        if (removeFn) return removeFn(path);
        return true;
    }

    bool rename(const char* oldPath, const char* newPath) {
        if (renameFn) return renameFn(oldPath, newPath);
        return true;
    }
};

extern LittleFSMock LittleFS;
