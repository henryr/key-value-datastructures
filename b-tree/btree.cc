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

#include <assert.h>
#include <vector>
#include <stack>
#include <iostream>
#include <chrono>

#include "btree.h"

using namespace std;

BTree::BTree(int max_keys) : MAX_KEYS(max_keys), root_(nullptr) {
}

int BTree::Find(int key) {
  int idx;
  Node* leaf = FindLeaf(key, &idx);
  if (leaf->key_at(idx) != key) return -1;
  return leaf->value_at(idx);
}

Node* BTree::FindLeaf(int key, int* idx) {
  Node* cur = root_;

  while (true) {
    int next_idx = cur->FindKeyIdx(key);
    if (cur->is_leaf()) {
      *idx = next_idx;
      return cur;
    }
    assert(!cur->is_leaf());
    Node* nxt = cur->child_at(next_idx);
    cur = nxt;
  }
}

int Node::FindKeyIdx(int key) {
  if (keys_.size() == 0) return 0;
  keys_.Prefetch();

  // If we want to SSE-ize this, let's:
  // 1. create a register with 'key' in all 8 slots
  // 2. 8 keys_ at a time, check to see if any are greater than 'key'
  // 3. If any are, break.

  // For now, linear search
  for (int i = 0; i < keys_.size(); ++i) {
    if (keys_[i] >= key) return i;
  }

  return keys_.size();
}

void BTree::Insert(int key, int value) {
  if (!root_) root_ = new Node(this, true);
  int idx;
  Node* node = FindLeaf(key, &idx);
  node->InsertKeyValue(idx, key, value);
  num_nodes_ += node->Split();
  CheckSelf();
}

void BTree::CheckSelf() {
#ifdef SANITY_CHECK
  if (!root_) return;
  stack<Node*> to_check;
  to_check.push(root_);
  while (!to_check.empty()) {
    Node* cur = to_check.top();
    to_check.pop();

    cur->CheckSelf();
    if (!cur->is_leaf()) {
      for (int i = 0; i < cur->num_children(); ++i) {
        to_check.push(cur->child_at(i));
      }
    }
  }
#endif
}

Node::Node(BTree* btree, const IntVector& keys, const IntVector& values) :
    is_leaf_(true), keys_(btree->MAX_KEYS), values_(btree->MAX_KEYS), btree_(btree) {
  for (int i = 0; i < keys.size(); ++i) keys_.PushBack(keys[i]);
  for (int i = 0; i < values.size(); ++i) values_.PushBack(values[i]);
}

Node::Node(BTree* btree, const IntVector& keys, const NodeVector& links) :
    is_leaf_(false), keys_(btree->MAX_KEYS), children_(btree->MAX_KEYS + 1), btree_(btree) {
  for (int i = 0; i < keys.size(); ++i) keys_.PushBack(keys[i]);
  for (int i = 0; i < links.size(); ++i) children_.PushBack(links[i]);
  assert(keys_.size() == children_.size() - 1);
}

Node::Node(BTree* btree, bool is_leaf) : keys_(btree->MAX_KEYS),
                                         children_(is_leaf ? 0 : btree->MAX_KEYS + 1),
                                         values_(is_leaf ? btree->MAX_KEYS : 0),
                                         btree_(btree),
                                         is_leaf_(is_leaf) {
  // Interior nodes have N keys and N + 1 links to the next level.
  // Leaves have N keys and N corresponding values.
}

void Node::CheckSelf() {
  bool is_root = parent_ == nullptr;
  if (!is_leaf()) {
    if (!is_root) {
      assert(num_keys() >= (btree_->MAX_KEYS / 2) - 1);
      assert(num_keys() < btree_->MAX_KEYS);
    }
    assert(num_children() == num_keys() + 1);

    for (int i = 0; i < num_keys(); ++i) {
      Node* child = child_at(i);
      int key = key_at(i);
      int prev = i == 0 ? -1 : key_at(i - 1);
      for (int j = 0; j < child->num_keys(); ++j) {
        assert(child->key_at(j) <= key);
        assert(child->key_at(j) > prev);
      }
    }
  } else {
    assert(num_keys() == num_values());
  }

  for (int i = 1; i < num_keys(); ++i) {
    assert(key_at(i) > key_at(i - 1));
  }
}

void Node::InsertKeyPointer(int idx, int key, Node* ptr) {
  assert(!is_leaf());
  keys_.Insert(idx, key);
  children_.Insert(idx + 1, ptr);
#ifdef SANITY_CHECK
  for (int i = 1; i < num_keys(); ++i) {
    assert(key_at(i) > key_at(i - 1));
  }
#endif
  ptr->parent_ = this;
}

void Node::InsertKeyValue(int idx, int key, int value) {
  assert(is_leaf());
  keys_.Insert(idx, key);
  values_.Insert(idx, value);
#ifdef SANITY_CHECK
  for (int i = 1; i < num_keys(); ++i) {
    assert(key_at(i) > key_at(i - 1));
  }
#endif
}

int Node::Split() {
  if (keys_.size() < btree_->MAX_KEYS) return 0;
  int pivot;
  Node* new_node = MakeSplittedNode(&pivot);
  if (!parent_) {
    Node* root = new Node(btree_, false);
    ++btree_->height_;
    root->keys_.PushBack(pivot);
    root->children_.PushBack(this);
    root->children_.PushBack(new_node);

    new_node->parent_ = root;
    parent_ = root;
    btree_->root_ = root;
    return 2;
  }

  int idx = parent_->FindKeyIdx(pivot);
  parent_->InsertKeyPointer(idx, pivot, new_node);
  return 1 + parent_->Split();
}

Node* Node::MakeSplittedNode(int* pivot_key) {
  // Split a node by partitioning it into two halves around a 'pivot' key. The pivot key is returned
  // to ultimately be inserted into the parent.  The new node created by splitting is the right-hand
  // successor of this node, and contains all keys larger than the pivot key.
  Node* new_node = new Node(btree_, is_leaf_);
  new_node->parent_ = parent_;

  // We need to be sure that the value for the pivot key belongs to the LHS.
  // So if #keys is odd, the pivot is the 'middle' key (e.g. in 4,5,6,7,8 the pivot is 6)
  //    if #keys is even, the pivot is the rightmost key in the left half. (e.g. in 4,5,6,7 the
  //    pivot is 5)
  //    in the odd case, the LHS gets one more key than the RHS.
  // way to compute this is to take the integer floor of (keys_.size() - 1) / 2.
  // int pivot_idx = (keys_.size() - 1) >> 1;
  int pivot_idx = (keys_.size() - 1) >> 1;
  *pivot_key = keys_[pivot_idx];

  int num_keys_rhs = keys_.size() >> 1;
  new_node->keys_.Resize(num_keys_rhs);
  memcpy(&new_node->keys_[0], &keys_[pivot_idx + 1], sizeof(int) * new_node->keys_.size());

  // If we're a leaf we 'keep' the key being pushed to the parent.
  keys_.Resize(pivot_idx + (is_leaf() ? 1 : 0));

  if (!is_leaf()) {
    new_node->children_.Resize(num_keys_rhs + 1); // One more link than key
    memcpy(&new_node->children_[0], &children_[pivot_idx + 1], sizeof(Node*) * (num_keys_rhs + 1));

    for (int i = 0; i < new_node->children_.size(); ++i) {
      if (!new_node->children_[i]) continue;
      new_node->children_[i]->parent_ = new_node;
    }
    children_.Resize(pivot_idx + 1);
  } else {
    new_node->values_.Resize(num_keys_rhs);
    memcpy(&new_node->values_[0], &values_[pivot_idx + 1], sizeof(int) * num_keys_rhs);
    values_.Resize(keys_.size());
  }

  return new_node;
}
