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

#include <string>

class CircularLog {
 public:
  CircularLog(int64_t size);

  int64_t Insert(const std::string& key, const std::string& value);
  int64_t Update(int64_t offset, const std::string& key, const std::string& value);

  // This needs to evolve to have hashtag matching
  void ReadFrom(int64_t offset, std::string* key, std::string* value);

  void DebugDump();
 private:
  int64_t PutString(int64_t offset, const std::string& s);
  void ReadString(int64_t offset, int64_t len, std::string* s);

  int64_t size_ = 0L;
  std::unique_ptr<int8_t> buffer_;
  int8_t* bufptr_ = nullptr;

  int64_t tail_ = 0L;
};
