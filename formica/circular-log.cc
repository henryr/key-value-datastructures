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

#include "circular-log.h"

#include <iostream>
#include <sys/mman.h>

using std::string;
using std::cout;
using std::endl;

namespace formica {

struct EntryHeader {
  char delimiter = '!';
  // Size, including this.
  entrysize_t size;
  entrysize_t keylen;
  entrysize_t valuelen;
  tag_t tag;
};

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
  if (header->tag != ExtractLogTag(expected)) {
    cout << "Expected tag: " << ExtractLogTag(expected) << ", but got: " << header->tag << endl;
    return false;
  }
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

}
