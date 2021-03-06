// Copyright 2018 Henry Robinson
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
// in compliance with the License.  You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
// or implied.  See the License for the specific language governing permissions and limitations
// under the License.

#pragma once

#include "common.h"

namespace formica {

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
