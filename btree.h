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

// TODO:
// 1. Memory is allocated into raw pointers and never deleted.
// 2. DONE - Figure out whether we need FastVector
// 3. Implement Delete()
// 4. Experiment with top-down splitting.
// 5. DONE - Clean up MakeSplittedNode()
// 6. Add support for strings as keys
// 7. DONE - Fix up InsertKeyPointer(before / after) mess

#include <vector>
#include "gtest/gtest.h"
#include "fast-vector.h"

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

  // Insert a pointer into an interior node at index 'idx'. 'ptr' is inserted after index, i.e. it
  // is taken to be the pointer to the node containing the keys that immediately follow 'key'.
  void InsertKeyPointer(int idx, int key, Node* ptr);

  // Returns the index of 'key' in this node, whether it indexes a value (for a leaf) or a
  // link (for an interior node).
  int FindKeyIdx(int key);

  int key_at(int idx) { return keys_[idx]; }
  int value_at(int idx) { return values_[idx]; }
  Node* child_at(int idx) { return children_[idx]; }

  int num_keys() const { return keys_.size(); }
  int num_children() const { return children_.size(); }
  int num_values() const { return values_.size(); }

  void CheckSelf();

 private:
  int height_ = 0;
  Node* parent_ = nullptr;
  IntVector keys_;

  // Only used if !is_leaf().
  NodeVector children_;

  // Only used if is_leaf().
  IntVector values_;

  BTree* btree_;
  const bool is_leaf_;

  FRIEND_TEST(BTree, MakeSplittedNode);
  FRIEND_TEST(BTree, Split);
  FRIEND_TEST(BTree, SplitUpTree);

  // Returns a new node containing all keys and values / links that fall *after* 'pivot_key' (which
  // is the middle key in this node). This node is resized to remove all the keys and values / links
  // that are moved to the returned node.
  Node* MakeSplittedNode(int* pivot_key);
};

// A B+-Tree of Nodes. All values are stored in leaves.
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
  int height() const { return height_; }

  // Used only for testing - replace the root node with a new root.
  void SetRoot(Node* root) { root_ = root; }
  void CheckSelf();

  Node* root() { return root_; }

  int num_nodes() const { return num_nodes_; }

  // When a node contains this many keys, it must be split.
  // Leaf nodes contain MAX_KEYS values. Interior nodes contain MAX_KEYS + 1 links.
  const int MAX_KEYS;

 private:
  friend class Node;

  // Returns the leaf node which may contain 'key', and the index of the closest key to it.
  Node* FindLeaf(int key, int* idx);

  FRIEND_TEST(BTree, Split);
  Node* root_ = nullptr;
  int num_nodes_ = 0;
  int height_ = 0;
};
