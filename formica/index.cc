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

#include "index.h"

#include <iostream>

namespace formica {

using std::string;
using std::cout;
using std::endl;

Index::Index(space_t size) : log_(size) { }

void Index::Insert(const Entry& entry) {
  // TODO: Could check if entry exists and use Update directly.
  offset_t offset = log_.Insert(entry.key, entry.value, entry.hash);
  tag_t bucket_tag = ExtractHashTag(entry.hash);
  idx_[bucket_tag] = {entry.hash, offset};
}

bool Index::Update(const Entry& entry) {
  auto it = idx_.find(ExtractHashTag(entry.hash));
  if (it == idx_.end()) return false;

  offset_t offset = log_.Update(it->second.second, entry.key, entry.value, entry.hash);
  if (offset == -1) return false;
  it->second.second = offset;
  return true;
}

void Index::Delete(const string& key) {
  // idx_.erase(ExtractHashTag(key);
}

bool Index::Read(const std::string& key, keyhash_t hash, std::string* value) {
  auto it = idx_.find(ExtractHashTag(hash));
  if (it == idx_.end()) return false;

  string stored_key;
  if (!log_.ReadFrom(it->second.second, hash, &stored_key, value)) return false;

  // This is the only time that the key is completely compared to the requested one.
  return key == stored_key;
}

LossyHash::LossyHash(int16_t num_buckets) : num_buckets_(num_buckets) {
  buckets_ = new Bucket[num_buckets];
}

offset_t LossyHash::Lookup(keyhash_t hash) {
  tag_t bucket_number = ExtractHashTag(hash);
  Bucket* bucket = buckets_ + (bucket_number % num_buckets_);

  tag_t log_tag = ExtractLogTag(hash);
  for (int i = 0; i < Bucket::NUM_ENTRIES; ++i) {
    if (bucket->entries[i].tag == log_tag) return bucket->entries[i].offset;
  }

  return -1;
}

void LossyHash::Insert(keyhash_t hash, offset_t offset, offset_t log_tail) {
  tag_t bucket_number = ExtractHashTag(hash);
  Bucket* bucket = buckets_ + (bucket_number % num_buckets_);

  tag_t log_tag = ExtractLogTag(hash);

  // TODO: Use log_tail to find the 'oldest' entry and delete that.
  // For now, use some of the hash to pick an entry at random.
  int entry_idx = (0xFFFF & hash) % Bucket::NUM_ENTRIES;
  for (int i = 0; i < Bucket::NUM_ENTRIES; ++i) {
    LossyHash::Entry* entry = &(bucket->entries[i]);
    if (entry->offset == -1 || entry->tag == log_tag) {
      // Either there's a spare entry, or this is a duplicate tag which we must overwrite to avoid
      // false negatives on read.
      entry_idx = i;
      break;
    }
  }

  // No space, and no duplicate to overwrite.
  bucket->entries[entry_idx] = {log_tag, offset};
}

LossyIndex::LossyIndex(space_t size, int16_t num_buckets) : idx_(num_buckets), log_(size) { }

void LossyIndex::Insert(const Entry& entry) {
  offset_t offset = log_.Insert(entry.key, entry.value, entry.hash);
  idx_.Insert(entry.hash, offset, -1);
}

bool LossyIndex::Update(const Entry& entry) {
  // TODO - need to refactor because we want to avoid double lookup in the hash to find the offset
  // and then rewrite it.
  return false;
}

bool LossyIndex::Read(const std::string& key, keyhash_t hash, std::string* value) {
  offset_t offset = idx_.Lookup(hash);
  if (offset == -1) return false;

  string stored_key;
  if (!log_.ReadFrom(offset, hash, &stored_key, value)) return false;

  return (stored_key == key);
}

void ChainedLossyHashIndex::Insert(const Entry& entry) {
  tag_t hash_tag = ExtractHashTag(entry.hash);
  int16_t bucket_num = hash_tag % num_buckets_;

  Node* node = new Node();
  node->log_tag = ExtractLogTag(entry.hash);
  node->key = entry.key;
  node->value = entry.value;

  Node* old = buckets_[bucket_num];
  buckets_[bucket_num] = node;
  node->next = old;
  if (old) {
    node->chain_len = std::min(14, old->chain_len + 1);
    if (node->chain_len == 14) {
      Node* cur = node;
      while (cur->next->next) cur = cur->next;
      delete cur->next;
      cur->next = nullptr;
    }
  }
}

bool ChainedLossyHashIndex::Read(const string& key, keyhash_t hash, string* value) {
  tag_t hash_tag = ExtractHashTag(hash);
  tag_t log_tag = ExtractLogTag(hash);

  Node* node = buckets_[hash_tag % num_buckets_];
  while (node) {
    if (node->log_tag == log_tag && node->key == key) {
      *value = node->value;
      return true;
    }
    node = node->next;
  }
  return false;
}

ChainedLossyHashIndex::ChainedLossyHashIndex(int num_buckets) : num_buckets_(num_buckets) {
  buckets_ = new Node*[num_buckets_];
  for (int i = 0; i < num_buckets_; ++i) buckets_[i] = nullptr;
}

ChainedLossyHashIndex::~ChainedLossyHashIndex() {
  if (buckets_ == nullptr) return;
  for (int i = 0; i < num_buckets_; ++i) {
    Node* node = buckets_[i];
    Node* next = nullptr;
    while (node) {
      next = node->next;
      delete node;
      node = next;
    }
  }
  delete buckets_;
}

}