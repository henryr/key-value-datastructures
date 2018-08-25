#include <vector>
#include "gtest/gtest.h"

#ifndef BTREE_H
#define BTREE_H

const int MAX_LEAF_KEYS = 100;
const int MAX_INTERIOR_KEYS = 100;

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
    memcpy(&values_[0], &other.values_[0], sizeof(T) * size_);
  }

  FastVector(const std::vector<T>& other) {
    values_ = new T[other.size()];
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


  inline bool is_leaf() const { return is_leaf_; }
  int height() const { return height_; }


  Node(BTree* btree, bool is_leaf) : keys_(is_leaf ? MAX_LEAF_KEYS : MAX_INTERIOR_KEYS),
                                     children_(is_leaf ? 0 : MAX_INTERIOR_KEYS + 1),
                                     values_(is_leaf ? MAX_LEAF_KEYS : 0),
                                     btree_(btree),
                                     is_leaf_(is_leaf) {
    // Interior nodes have N keys and N + 1 links to the next level.
    // Leaves have N keys and N corresponding values.
  }

  // For testing
  Node(const IntVector& keys, const IntVector& values) : is_leaf_(true), keys_(keys), values_(values) { }

  Node(const IntVector& keys, const NodeVector& links) : is_leaf_(false), keys_(keys), children_(links) {
    assert(keys_.size() == children_.size() - 1);
  }

  void Split();

  void InsertKeyValue(int idx, int key, int value) {
    assert(is_leaf());
    if (idx != keys_.size()) {
      keys_.Insert(idx, key);
      values_.Insert(idx, value);
    } else {
      keys_.PushBack(key);
      values_.PushBack(value);
    }
  }

  // Returns the index of 'key' in this node, whether it indexes a value (for a leaf) or a
  // link (for an interior node)
  int FindKeyIdx(int key);

  int key_at(int idx) { return keys_[idx]; }
  int value_at(int idx) { return values_[idx]; }
  Node* child_at(int idx) { return children_[idx]; }

  void update_key(int idx, int value) { keys_[idx] = value; }

  Node* rightmost_child() { return children_.back(); }

  int num_keys() const { return keys_.size(); }
  int num_children() const { return children_.size(); }
  int num_values() const { return values_.size(); }

 private:

  bool is_leaf_;
  int height_ = 0;
  Node* parent_ = nullptr;
  IntVector keys_;
  NodeVector children_;
  IntVector values_;
  BTree* btree_;

  FRIEND_TEST(BTree, MakeSplittedNode);

  Node* MakeSplittedNode(int* median_key);
};

class BTree {
 public:
  int Find(int key);
  void Insert(int key, int value);
  void Delete(int key);
  int height() const { return root_->height(); }
  BTree();

  void SetRoot(Node* root) { root_ = root; }
  int num_nodes_ = 0;

 private:
  // Returns the leaf node which may contain 'key', and the index of the closest key to it.
  Node* FindLeaf(int key, int* idx);

  Node* root_ = nullptr;
};

#endif
