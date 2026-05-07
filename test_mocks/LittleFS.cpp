#include "LittleFS.h"
LittleFSMockState littleFsMockState;

// Modify File::print in LittleFS.h to return data.length() if littleFsMockState.printResult == (size_t)-1
