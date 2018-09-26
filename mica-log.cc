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

#include "mica-log.h"

#include <iostream>
#include <sys/mman.h>
#include <mach/mach_vm.h>

namespace mica {

using std::string;
using std::cout;
using std::endl;

struct EntryHeader {
  char delimiter = '!';
  // Size, including this.
  entrysize_t size;
  entrysize_t keylen;
  entrysize_t valuelen;
  tag_t tag;
};

namespace {

int16_t ExtractLogTag(keyhash_t hash) {
  return static_cast<int16_t>(0x000000000000FFFF & hash);
}

int16_t ExtractHashTag(keyhash_t hash) {
  return static_cast<int16_t>(0x000000000000FFFF & (hash >> 16));
}

}

CircularLog::CircularLog(space_t size) : size_(size) {
  assert(size_ > 0);
  void* map = mmap(0, size_, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
  if (map == MAP_FAILED) {
    cout << "ERRORNO: " << errno << " (enomem: " << ENOMEM << ")" << endl;
    assert(false);  // TODO
  }
  bufptr_ = reinterpret_cast<int8_t*>(map);
}

offset_t CircularLog::Insert(const string& key, const string& value, keyhash_t hash) {
  return Update(-1, key, value, hash);
}

offset_t CircularLog::Update(offset_t offset, const string& key, const string& value, keyhash_t hash) {
  assert(tail_ < size_);

  space_t required = key.size() + value.size() + sizeof(EntryHeader);
  if (required >= size_) {
    return -1;
  }

  bool is_append = (offset == -1);
  if (offset > -1) {
    EntryHeader* header = reinterpret_cast<EntryHeader*>(bufptr_ + offset);
    is_append = header->delimiter != '!' ||
        (header->keylen + header->valuelen < key.size() + value.size());
  }

  if (is_append) offset = tail_;

  offset_t cursor = offset;
  EntryHeader* header = reinterpret_cast<EntryHeader*>(bufptr_ + cursor);
  header->delimiter = '!';
  header->size = required;
  header->keylen = key.size();
  header->valuelen = value.size();
  header->tag = ExtractLogTag(hash);
  cursor += sizeof(EntryHeader);
  cursor %= size_;
  cursor = PutString(cursor, key);
  cursor = PutString(cursor, value);

  if (is_append) {
    tail_ = cursor;
    if (size_ - tail_ < sizeof(EntryHeader)) tail_ = 0L;
  }

  return offset;
}

offset_t CircularLog::PutString(offset_t offset, const string& s) {
  if (size_ - offset > s.size()) {
    memcpy(reinterpret_cast<void*>(bufptr_ + offset), s.data(), s.size());
    return offset + s.size();
  }

  // Otherwise split the write
  space_t to_write = size_ - offset;
  memcpy(reinterpret_cast<void*>(bufptr_ + offset), s.data(), to_write);
  memcpy(reinterpret_cast<void*>(bufptr_), s.data() + to_write, s.size() - to_write);

  return s.size() - to_write;
}

void CircularLog::ReadString(offset_t offset, entrysize_t len, string* s) {
  offset %= size_;
  if (offset + len < size_) {
    *s = string(reinterpret_cast<char*>(bufptr_ + offset), len);
    return;
  }

  // Otherwise this is a wrapped read.
  s->reserve(len);

  // Read from offset to size_
  int8_t* start = bufptr_ + offset;
  s->append(reinterpret_cast<char*>(start), size_ - offset);

  space_t remaining = len - (size_ - offset);
  s->append(reinterpret_cast<char*>(bufptr_), remaining);
}

bool CircularLog::ReadFrom(offset_t offset, keyhash_t expected, string* key, string* value) {
  assert(offset < size_);
  EntryHeader* header = reinterpret_cast<EntryHeader*>(bufptr_ + offset);
  if (header->tag != ExtractLogTag(expected)) return false;
  offset_t keystart = offset + sizeof(EntryHeader);
  ReadString(keystart, header->keylen, key);
  ReadString(keystart + header->keylen, header->valuelen, value);
  return true;
}

void CircularLog::DebugDump() {
  for (offset_t i = 0; i < size_; ++i) {
    cout << i << ":" << *(reinterpret_cast<char*>(bufptr_ + i));
    if (i == tail_) cout << " <-- TAIL ";
    cout << endl;
  }
  cout << endl;
}

CircularLog::~CircularLog() {
  if (bufptr_ != nullptr) munmap(bufptr_, size_);
}


Index::Index(space_t size) : log_(size) { }

void Index::Insert(const Entry& entry) {
  // TODO: Could check if entry exists and use Update directly.
  offset_t offset = log_.Insert(entry.key, entry.value, entry.hash);
  idx_[entry.key] = {entry.hash, offset};
}

bool Index::Update(const Entry& entry) {
  auto it = idx_.find(entry.key);
  if (it == idx_.end()) return false;

  offset_t offset = log_.Update(it->second.second, entry.key, entry.value, entry.hash);
  if (offset == -1) return false;
  it->second.second = offset;
  return true;
}

void Index::Delete(const string& key) {
  idx_.erase(key);
}

bool Index::Read(const std::string& key, keyhash_t hash, std::string* value) {
  auto it = idx_.find(key);
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

}
