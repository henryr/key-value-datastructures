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

#include "store.h"

#include <iostream>

namespace formica {

using std::string;
using std::cout;
using std::endl;

StdMapStore::StdMapStore(space_t size) : log_(size) { }

void StdMapStore::Insert(const Entry& entry) {
  // TODO: Could check if entry exists and use Update directly.
  offset_t offset = log_.Insert(entry.key, entry.value, entry.hash);
  idx_[entry.key] = {entry.hash, offset};
}

bool StdMapStore::Update(const Entry& entry) {
  auto it = idx_.find(entry.key);
  if (it == idx_.end()) return false;

  offset_t offset = log_.Update(it->second.second, entry.key, entry.value, entry.hash);
  if (offset == -1) return false;
  it->second.second = offset;
  return true;
}

void StdMapStore::Delete(const string& key) {
  idx_.erase(key);
}

bool StdMapStore::Read(const std::string& key, keyhash_t hash, std::string* value) {
  auto it = idx_.find(key);
  if (it == idx_.end()) {
    ++index_misses_;
    return false;
  }

  string stored_key;
  if (!log_.ReadFrom(it->second.second, hash, &stored_key, value)) {
    ++log_overwritten_;
    return false;
  }

  // This is the only time that the key is completely compared to the requested one.
  if (key != stored_key) {
    ++log_other_key_;
    return false;
  }
  return true;
}

LossyHash::LossyHash(bucket_count_t num_buckets) : num_buckets_(num_buckets) {
  buckets_ = new Bucket[num_buckets];
}

offset_t LossyHash::Lookup(keyhash_t hash) {
  tag_t bucket_number = ExtractHashTag(hash);
  Bucket* bucket = &(buckets_[bucket_number % num_buckets_]);

  tag_t log_tag = ExtractLogTag(hash);
  for (int i = 0; i < Bucket::NUM_ENTRIES; ++i) {
    if (bucket->entries[i].tag == log_tag) return bucket->entries[i].offset;
  }

  return -1;
}

void LossyHash::Insert(keyhash_t hash, offset_t offset, offset_t log_tail) {
  tag_t bucket_number = ExtractHashTag(hash);
  Bucket* bucket = &(buckets_[bucket_number % num_buckets_]);

  tag_t log_tag = ExtractLogTag(hash);

  // TODO: Use log_tail to find the 'oldest' entry and delete that, per the paper.
  // For now, use some of the hash to pick an entry at random.
  int entry_idx = (0xF0F0F0F0 & hash) % Bucket::NUM_ENTRIES;
  for (int i = 0; i < Bucket::NUM_ENTRIES; ++i) {
    LossyHash::Entry* entry = &(bucket->entries[i]);
    if (entry->offset == -1 || entry->tag == log_tag) {
      // Either there's a spare entry, or this is a duplicate tag which we must overwrite to avoid
      // false negatives on read.
      entry_idx = i;
      break;
    }
  }

  bucket->entries[entry_idx] = {log_tag, offset};
}

FormicaStore::FormicaStore(space_t size, bucket_count_t num_buckets)
    : idx_(num_buckets), log_(size) { }

void FormicaStore::Insert(const Entry& entry) {
  offset_t offset = log_.Insert(entry.key, entry.value, entry.hash);
  idx_.Insert(entry.hash, offset, -1);
}

bool FormicaStore::Update(const Entry& entry) {
  // TODO - need to refactor because we want to avoid double lookup in the hash to find the offset
  // and then rewrite it, i.e. need hash to return an `iterator` that we can reuse.
  return false;
}

bool FormicaStore::Read(const std::string& key, keyhash_t hash, std::string* value) {
  offset_t offset = idx_.Lookup(hash);
  if (offset == -1) {
    ++index_misses_;
    return false;
  }

  string stored_key;
  if (!log_.ReadFrom(offset, hash, &stored_key, value)) {
    ++log_overwritten_;
    return false;
  }

  if (stored_key != key) {
    ++log_other_key_;
    return false;
  }
  return true;
}

void ChainedLossyHashStore::Insert(const Entry& entry) {
  tag_t hash_tag = ExtractHashTag(entry.hash);
  bucket_count_t bucket_num = hash_tag % num_buckets_;

  Bucket* bucket = &(buckets_[bucket_num]);
  Node* old = bucket->first;

  // Only allocate if we're still expanding the chain, otherwise recycle the node we're evicting.
  Node* node = bucket->chain_len == MAX_CHAIN_LENGTH ? bucket->last : new Node();
  node->log_tag = ExtractLogTag(entry.hash);
  node->key = entry.key;
  node->value = entry.value;

  if (bucket->chain_len == MAX_CHAIN_LENGTH) {
    node->prev->next = nullptr;
    bucket->last = node->prev;
    node->prev = nullptr;
  } else {
    ++bucket->chain_len;
  }

  bucket->first = node;
  node->next = old;

  if (old) {
    old->prev = node;
  } else {
    bucket->last = node;
  }
}

bool ChainedLossyHashStore::Read(const string& key, keyhash_t hash, string* value) {
  tag_t hash_tag = ExtractHashTag(hash);
  tag_t log_tag = ExtractLogTag(hash);

  Node* node = buckets_[hash_tag % num_buckets_].first;
  while (node) {
    if (node->log_tag == log_tag && node->key == key) {
      *value = node->value;
      return true;
    }
    node = node->next;
  }

  ++index_misses_;
  return false;
}

ChainedLossyHashStore::ChainedLossyHashStore(int num_buckets) : num_buckets_(num_buckets) {
  buckets_ = new Bucket[num_buckets_];
}

ChainedLossyHashStore::~ChainedLossyHashStore() {
  if (buckets_ == nullptr) return;
  for (int i = 0; i < num_buckets_; ++i) {
    Node* node = buckets_[i].first;
    Node* next = nullptr;
    while (node) {
      next = node->next;
      delete node;
      node = next;
    }
  }
  delete[] buckets_;
}

}
