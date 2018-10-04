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
#include <unordered_map>

namespace formica {

typedef int64_t offset_t;
typedef int32_t entrysize_t;
typedef int64_t space_t;
typedef size_t keyhash_t;
typedef uint32_t tag_t;

struct Entry {
 public:
  const std::string key;
  const std::string value;
  const keyhash_t hash;

  Entry(const std::string& key, const std::string& value) :
      key(key), value(value), hash(std::hash<std::string>{}(key)) {
  }
};

inline tag_t ExtractLogTag(keyhash_t hash) {
  return static_cast<tag_t>(0x00000000FFFFFFFF & hash);
}

inline tag_t ExtractHashTag(keyhash_t hash) {
  return static_cast<tag_t>(0x00000000FFFFFFFF & (hash >> 32));
}

}
