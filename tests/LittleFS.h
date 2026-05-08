#pragma once

#include <map>
#include <string>
#include <vector>

#include "Arduino.h"

class File {
public:
  File() : storage(nullptr), writable(false) {}
  File(std::string *target, bool allowWrite) : storage(target), writable(allowWrite) {}

  explicit operator bool() const { return storage != nullptr; }
  size_t size() const { return storage ? storage->size() : 0; }

  String readString() const {
    return storage ? String(*storage) : String();
  }

  size_t print(const String &text) {
    if (!storage || !writable) {
      return 0;
    }
    storage->append(text.value);
    return text.length();
  }

  size_t print(const char *text) {
    return print(String(text));
  }

  void close() {}

private:
  std::string *storage;
  bool writable;
};

class Dir {
public:
  Dir() : files(nullptr), index(0) {}
  explicit Dir(std::map<std::string, std::string> *entries) : files(entries), index(0) {
    if (files) {
      for (const auto &entry : *files) {
        names.push_back(entry.first);
      }
    }
  }

  bool next() {
    if (index >= names.size()) {
      return false;
    }
    ++index;
    return true;
  }

  String fileName() const {
    if (index == 0 || index > names.size()) {
      return String();
    }
    return String(names[index - 1]);
  }

  File openFile(const char *) {
    if (!files || index == 0 || index > names.size()) {
      return File();
    }
    return File(&(*files)[names[index - 1]], false);
  }

private:
  std::map<std::string, std::string> *files;
  std::vector<std::string> names;
  size_t index;
};

class LittleFSClass {
public:
  bool begin() { return true; }

  bool exists(const char *path) const {
    return files.find(path ? path : "") != files.end();
  }

  File open(const char *path, const char *mode) {
    const std::string key = path ? path : "";
    const bool writeMode = mode && std::strchr(mode, 'w');
    if (writeMode) {
      files[key].clear();
      return File(&files[key], true);
    }
    auto it = files.find(key);
    if (it == files.end()) {
      return File();
    }
    return File(&it->second, false);
  }

  bool remove(const char *path) {
    return files.erase(path ? path : "") > 0;
  }

  bool rename(const char *from, const char *to) {
    const std::string source = from ? from : "";
    const std::string target = to ? to : "";
    auto it = files.find(source);
    if (it == files.end()) {
      return false;
    }
    files[target] = it->second;
    files.erase(it);
    return true;
  }

  Dir openDir(const char *) {
    return Dir(&files);
  }

private:
  std::map<std::string, std::string> files;
};

extern LittleFSClass LittleFS;
