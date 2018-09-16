#include "mica-log.h"

using std::string;

struct EntryHeader {
  // Size, including this.
  int64_t size;
};

CircularLog::CircularLog(int64_t size) : size_(size) {
  assert(size_ > 0);

  buffer_.reset(new int8_t[size_]);
  bufptr_ = buffer_.get();
}

int64_t CircularLog::Insert(const string& key, const string& value) {
  return Update(-1, key, value);
}

int64_t CircularLog::Update(int64_t offset, const string& key, const string& value) {
  if (offset > -1) {
    // TODO: Check to see if space used at offset is less than required by key + value
  }

  offset = tail_;

  // Do we need to evict?
  int64_t diff = tail_ - head_;
  if (diff < 0) diff += size_;

  int64_t required = key.size() + value.size() + sizeof(EntryHeader);

  if (size_ - diff < required) {
    // TODO: Evict
  }

  // TODO Check required < size_
  // TODO Pad to a multiple of EntryHeader so there's always room to insert the header at least.
  EntryHeader* header = reinterpret_cast<EntryHeader*>(bufptr_ + tail_);
  header->size = required;
  tail_ += sizeof(EntryHeader); tail_ = tail_ % size_;

  PutString(key);
  PutString(value);

  return tail_;
}

void CircularLog::PutString(const string& s) {
  if (size_ - tail_ > s.size()) {
    memcpy(reinterpret_cast<void*>(bufptr_ + tail_), s.data(), s.size());
    tail_ += s.size();
    return;
  }

  // Otherwise split the write
  int64_t to_write = size_ - tail_;
  memcpy(reinterpret_cast<void*>(bufptr_ + tail_), s.data(), to_write);
  memcpy(reinterpret_cast<void*>(bufptr_), s.data() + to_write, s.size() - to_write);

  tail_ = s.size() - to_write;
}
