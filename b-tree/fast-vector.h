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

// A vector class with a fast-ish insert thanks to knowing its (fixed) capacity.
// std::vector appears to be about 25% slower. More investigation warranted.
template <typename T>
class FastVector {
 public:
  FastVector() : size_(0), capacity_(0) { }

  FastVector(int capacity) : size_(0), capacity_(capacity) {
    values_ = new T[capacity];
  }

  FastVector(const FastVector<T>& other) {
    values_ = new T[other.capacity()];
    size_ = other.size_;
    capacity_ = other.capacity();
    memcpy(&values_[0], &other.values_[0], sizeof(T) * size_);
  }

  FastVector(const std::vector<T>& other) {
    capacity_ = other.capacity();
    values_ = new T[other.capacity()];
    for (const auto& val: other) PushBack(val);
    assert(size_ == other.size());
  }

  void PushBack(const T& val) {
    values_[size_++] = val;
  }

  void Resize(int size) {
    size_ = size;
  }

  void Insert(int idx, const T& val) {
    if (idx == size_) {
      PushBack(val);
      return;
    }
    memmove(&values_[idx + 1], &values_[idx], sizeof(T) * (size_ - idx));
    ++size_;
    values_[idx] = val;
  }

  int size() const { return size_; }
  int capacity() const { return capacity_; }
  T* values() const { return values_; }

  T& operator[](int idx) {
    return values_[idx];
  }

  T operator[](int idx) const { return values_[idx]; }

  ~FastVector() {
    if (values_) delete[] values_;
  }

  void Prefetch() {
    int step = 64 / sizeof(T);
    for (int i = 0; i < size_; i+= step) {
      void* v= reinterpret_cast<void*>(&values_[step]);
      __builtin_prefetch(v);
    }
  }

 private:
  T* values_ = nullptr;
  int size_ = 0;
  int capacity_ = 0;
};
