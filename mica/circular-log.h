#pragma once

#include "mica-log.h"

namespace mica {

class CircularLog {
 public:
  CircularLog(space_t size);
  ~CircularLog();

  offset_t Insert(const std::string& key, const std::string& value, keyhash_t hash);
  offset_t Update(offset_t offset, const std::string& key, const std::string& value, keyhash_t hash);

  bool ReadFrom(offset_t offset, keyhash_t expected, std::string* key, std::string* value);

  void DebugDump();

 private:
  offset_t PutString(offset_t offset, const std::string& s);
  void ReadString(offset_t offset, entrysize_t len, std::string* s);

  space_t size_ = 0L;
  int8_t* bufptr_ = nullptr;

  offset_t tail_ = 0L;
};

}
