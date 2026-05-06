#ifndef LITTLEFS_H
#define LITTLEFS_H
class LittleFS_Mock {
public:
    bool begin() { return true; }
};
extern LittleFS_Mock LittleFS;
#endif
