#include <vector>
#include "gtest/gtest.h"

#ifndef BTREE_H
#define BTREE_H

// A vector class with a fast-ish insert thanks to knowing its (fixed) capacity.
// TODO: No reason std::vector doesn't do this, so compare performance and remove if needed.
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

// A node in a BTree. May be either a leaf node or an interior node; the former contains links to
// other nodes, the latter contains as many values as keys.
//
// The number of allowable keys is controlled by BTree::MAX_KEYS. If the number of keys reaches the
// max, the node must be split via Split().
//
// Interior nodes have one more link than key, which corresponds to all those keys which are larger
// than the rightmost key value.
class Node {
 public:
  Node(BTree* btree, bool is_leaf);

  // For testing
  Node(BTree* btree, const IntVector& keys, const IntVector& values);
  Node(BTree* btree, const IntVector& keys, const NodeVector& links);


  Node* parent() const { return parent_; }
  bool is_leaf() const { return is_leaf_; }
  int height() const { return height_; }

  // If num_keys() == MAX_KEYS, splits this node into two along the middle key, and updates parent_
  // to point to both this node and its new right-hand sibling. Recursively splits the parent, if
  // needed. If this is the root node, a new root is created with one key and two links.
  //
  // Returns the number of new nodes added to the tree.
  int Split();

  // Inserts a key into a leaf at index 'idx'
  void InsertKeyValue(int idx, int key, int value);

  // Insert a pointer into an interior node at index 'idx'. If 'after' is true (which it usually
  // is), 'ptr' is inserted after index, i.e. it is taken to be the pointer to the nodecontaining
  // the keys that immediately follow 'key'.
  void InsertKeyPointer(int idx, int key, Node* ptr, bool after);

  // Returns the index of 'key' in this node, whether it indexes a value (for a leaf) or a
  // link (for an interior node).
  int FindKeyIdx(int key);

  int key_at(int idx) { return keys_[idx]; }
  int value_at(int idx) { return values_[idx]; }
  Node* child_at(int idx) { assert(idx < children_.size()); return children_[idx]; }

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

  // Returns a new node containing all keys and values / links that fall *after* 'median_key' (which
  // is the middle key in this node). This node is resized to remove all the keys and values / links
  // that are moved to the returned node.
  Node* MakeSplittedNode(int* median_key);
};

class BTree {
 public:
  BTree(int max_keys);

  // Returns the value associated with 'key' in the tree, or -1 if the key does not exist.
  int Find(int key);

  // Inserts a new (key, value) pair into the tree.
  void Insert(int key, int value);

  // Not implemented
  void Delete(int key);

  // Returns the height of the tree. Only set correctly for the root node.
  int height() const { return root_->height(); }

  // Used only for testing - replace the root node with a new root.
  void SetRoot(Node* root) { root_ = root; }
  void CheckSelf();

  Node* root() { return root_; }

  int num_nodes() const { return num_nodes_; }

  // When a node contains this many keys, it must be split.
  // Leaf nodes contain MAX_KEYS values. Interior nodes contain MAX_KEYS + 1 links.
  const int MAX_KEYS;

 private:
  // Returns the leaf node which may contain 'key', and the index of the closest key to it.
  Node* FindLeaf(int key, int* idx);

  FRIEND_TEST(BTree, Split);
  Node* root_ = nullptr;
  int num_nodes_ = 0;
};

#endif
