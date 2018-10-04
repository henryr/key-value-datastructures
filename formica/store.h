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

// Controls how many buckets a hash table can have. Should be no wider than tag_t, which is used to
// index the hash tables, otherwise there are going to be lots of unused buckets.
typedef int32_t bucket_count_t;

// The StdMapStore uses a std::unordered_map to index offsets into a CircularLog. The offsets are
// indexed by the full key, not the log-tag, to avoid having high-impact collisions.
class StdMapStore {
 public:
  StdMapStore(space_t size);

  // So that we can use indexes as template parameters with identical c'tor signatures.
  StdMapStore(space_t size, bucket_count_t dummy) : StdMapStore(size) { }

  void Insert(const Entry& entry);
  bool Update(const Entry& entry);
  void Delete(const std::string& key);
  bool Read(const std::string& key, keyhash_t hash, std::string* value);

  void DebugDump();

  int index_misses() { return index_misses_; }
  int log_overwritten() { return log_overwritten_; }
  int log_other_key() { return log_other_key_; }

 private:
  CircularLog log_;

  // Index (hash, offset) pairs by the full key. It's easy to change this to use a tag_t for better
  // performance, but a lot more collisions.
  std::unordered_map<std::string, std::pair<keyhash_t, offset_t>> idx_;

  int index_misses_ = 0;
  int log_overwritten_ = 0;
  int log_other_key_ = 0;
};

// This is the Formica version of MICA's lossy hash table.
class LossyHash {
 public:
  LossyHash(bucket_count_t num_buckets);

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
    // sizeof(Entry) == 10 right now, so to copy the paper and fit into two cache lines we have 128 /
    // 9 == 14 entries per bucket. The paper says they have 15 entries per bucket, so their entry
    // must be smaller. Perhaps they are using 56-bit offsets and packing everything into a 64-bit
    // Entry? That would fit directly into 128 bytes, but they need to save some space for the
    // version field.
    static constexpr int8_t NUM_ENTRIES = 25;
    Entry entries[NUM_ENTRIES];
    int32_t padding;
    int16_t padding2;
  };

  bucket_count_t num_buckets_ = 0;
  Bucket* buckets_;
};

class FormicaStore {
 public:
  FormicaStore(space_t size, bucket_count_t num_buckets);

  void Insert(const Entry& entry);
  bool Update(const Entry& entry);
  void Delete(const std::string& key);
  bool Read(const std::string& key, keyhash_t hash, std::string* value);

  void DebugDump();

  int index_misses() { return index_misses_; }
  int log_overwritten() { return log_overwritten_; }
  int log_other_key() { return log_other_key_; }

 private:
  LossyHash idx_;
  CircularLog log_;
  int index_misses_ = 0;
  int log_overwritten_ = 0;
  int log_other_key_ = 0;
};

class ChainedLossyHashStore {
 public:
  ChainedLossyHashStore(int num_buckets);
  ChainedLossyHashStore(space_t dummy, int num_buckets) : ChainedLossyHashStore(num_buckets) { }

  void Insert(const Entry& entry);
  bool Read(const std::string& key, keyhash_t hash, std::string* value);

  void DebugDump();

  int index_misses() { return index_misses_; }
  int log_overwritten() { return log_overwritten_; }
  int log_other_key() { return log_other_key_; }


  ~ChainedLossyHashStore();

 private:
  int index_misses_ = 0;
  int log_overwritten_ = 0;
  int log_other_key_ = 0;

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

  bucket_count_t num_buckets_ = 0;

  Bucket* buckets_ = nullptr;

};

}
