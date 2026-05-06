#pragma once
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "WString.h"

#define ICACHE_FLASH_ATTR
#define PROGMEM
#define PGM_P const char *
#define F(string_literal) string_literal

inline unsigned long millis() { return 0; }
inline void delay(unsigned long) {}

inline char* ultoa(unsigned long value, char* str, int base) {
    if (base == 10) sprintf(str, "%lu", value);
    return str;
}
