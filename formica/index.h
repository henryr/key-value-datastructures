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

#include "circular-log.h"

namespace formica {

class Index {
 public:
  Index(space_t size);

  // For compatibility
  Index(space_t size, int16_t dummy) : Index(size) { }

  void Insert(const Entry& entry);
  bool Update(const Entry& entry);
  void Delete(const std::string& key);
  bool Read(const std::string& key, keyhash_t hash, std::string* value);

 private:
  CircularLog log_;
  std::unordered_map<tag_t, std::pair<keyhash_t, offset_t>> idx_;
};

class LossyHash {
 public:
  LossyHash(int16_t num_buckets);

  offset_t Lookup(keyhash_t hash);
  void Insert(keyhash_t hash, offset_t offset, offset_t log_tail);

  // TODO: Does deletion require a full read from the log to confirm we're deleting the right thing?
  void Delete(keyhash_t hash);

 private:
  struct Entry {
    tag_t tag = 0;
    offset_t offset = -1;
  };
  struct Bucket {
    // sizeof(Entry) == 9 right now, so to copy the paper and fit into two cache lines we have 128 /
    // 9 == 14 entries per bucket. The paper says they have 15 entries per bucket, so their entry
    // must be smaller. Perhaps they are using 56-bit offsets and packing everything into a 64-bit
    // Entry? That would fit directly into 128 bytes, but they need to save some space for the
    // version field.
    static constexpr int8_t NUM_ENTRIES = 14;
    Entry entries[NUM_ENTRIES];
    int16_t padding;
  };

  int16_t num_buckets_ = 0;
  Bucket* buckets_;
};

class LossyIndex {
 public:
  LossyIndex(space_t size, int16_t num_buckets);

  void Insert(const Entry& entry);
  bool Update(const Entry& entry);
  void Delete(const std::string& key);
  bool Read(const std::string& key, keyhash_t hash, std::string* value);

 private:
  LossyHash idx_;
  CircularLog log_;
};

class ChainedLossyHashIndex {
 public:
  ChainedLossyHashIndex(int num_buckets);
  ChainedLossyHashIndex(space_t dummy, int num_buckets) : ChainedLossyHashIndex(num_buckets) { }

  void Insert(const Entry& entry);
  bool Read(const std::string& key, keyhash_t hash, std::string* value);

  ~ChainedLossyHashIndex();

 private:
  struct Node {
    Node* next = nullptr;
    Node* prev = nullptr;
    tag_t log_tag = 0;
    std::string key;
    std::string value;
  };

  struct Bucket {
    Node* first = nullptr;
    Node* last = nullptr;
    int chain_len = 0;
  };

  int16_t num_buckets_ = 0;

  Bucket* buckets_ = nullptr;

};

}
