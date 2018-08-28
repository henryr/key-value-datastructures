#include <vector>
#include "gtest/gtest.h"

#ifndef BTREE_H
#define BTREE_H

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
  const T& back() const { return values_[size_ - 1]; }

  T& operator[](int idx) {
    return values_[idx];
  }

  T operator[](int idx) const { return values_[idx]; }

 private:
  T* values_;
  int size_ = 0;
  int capacity_ = 0;
};

class Node;
typedef FastVector<int> IntVector;
typedef FastVector<Node*> NodeVector;

class BTree;

class Node {
 public:
  Node* parent() const { return parent_; }


  bool is_leaf() const { return is_leaf_; }
  int height() const { return height_; }


  Node(BTree* btree, bool is_leaf);

  // For testing
  Node(BTree* btree, const IntVector& keys, const IntVector& values);
  Node(BTree* btree, const IntVector& keys, const NodeVector& links);

  void Split();

  void InsertKeyValue(int idx, int key, int value);
  void InsertKeyPointer(int idx, int key, Node* ptr);

  // Returns the index of 'key' in this node, whether it indexes a value (for a leaf) or a
  // link (for an interior node)
  int FindKeyIdx(int key);

  int key_at(int idx) { return keys_[idx]; }
  int value_at(int idx) { return values_[idx]; }
  Node* child_at(int idx) { assert(idx < children_.size()); return children_[idx]; }

  void update_key(int idx, int value) { keys_[idx] = value; }

  Node* rightmost_child() { return children_.back(); }

  int num_keys() const { return keys_.size(); }
  int num_children() const { return children_.size(); }
  int num_values() const { return values_.size(); }

  void CheckSelf();

 private:

  int height_ = 0;
  Node* parent_ = nullptr;
  IntVector keys_;
  NodeVector children_;
  IntVector values_;
  BTree* btree_;
  const bool is_leaf_;

  FRIEND_TEST(BTree, MakeSplittedNode);
  FRIEND_TEST(BTree, Split);
  FRIEND_TEST(BTree, SplitUpTree);

  Node* MakeSplittedNode(int* median_key);
};

class BTree {
 public:
  int Find(int key);
  void Insert(int key, int value);
  void Delete(int key);
  int height() const { return root_->height(); }
  BTree(int max_leaf_keys, int max_interior_keys);

  void SetRoot(Node* root) { root_ = root; }
  void CheckSelf();
  int num_nodes_ = 0;

  const int MAX_LEAF_KEYS;
  const int MAX_INTERIOR_KEYS;

 private:
  // Returns the leaf node which may contain 'key', and the index of the closest key to it.
  Node* FindLeaf(int key, int* idx);

  FRIEND_TEST(BTree, Split);
  Node* root_ = nullptr;
};

#endif
