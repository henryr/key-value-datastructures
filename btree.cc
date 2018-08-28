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

void Node::InsertKeyPointer(int idx, int key, Node* ptr, bool after) {
  assert(!is_leaf());
  keys_.Insert(idx, key);
  children_.Insert(idx + (after ? 1 : 0), ptr);
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
  int median;
  Node* new_node = MakeSplittedNode(&median);
  if (!parent_) {
    Node* root = new Node(btree_, false);
    ++btree_->height_;
    root->keys_.PushBack(median);
    root->children_.PushBack(this);
    root->children_.PushBack(new_node);

    new_node->parent_ = root;
    parent_ = root;
    btree_->root_ = root;
    return 2;
  }

  int idx = parent_->FindKeyIdx(median);
  parent_->InsertKeyPointer(idx, median, new_node, true);
  return 1 + parent_->Split();
}

Node* Node::MakeSplittedNode(int* median_key) {
  // TODO: these are slightly outdated.
  // Steps for splitting an interior node:
  // 1. pick a median. This value will be inserted into the parent.
  // 2. create a new node with floor(keys.size() / 2) keys.
  // 3. copy all keys from keys_[median_idx + 1] .. keys_.size() into new node
  // 4. copy all children pointers from children_[median_idx + 1] .. childen_.size() into new node
  // 5. Resize keys_ to keys_.size() / 2. Resize children_ to 1 + children_.size() / 2
  //
  // For a leaf node:
  // 1. pick a median. This value will be inserted into the parent.
  // 2. Create a new node with floor(keys.size() / 2) keys and children.
  // 3. Copy all keys from keys_[median_idx + 1] .. keys_.size() into new node
  // 4. copy all values from keys_[median_idx + 1] .. children.size() into new node
  // 5. Resize keys_ to median_idx. Resize children_ to median_idx.

  Node* new_node = new Node(btree_, is_leaf_);
  new_node->parent_ = parent_;

  if (!is_leaf()) {
    // If:
    // a) keys_.size() is odd, say 9, then: median_idx = 4 (0,1,2,3 and 5,6,7,8 on lhs and ths)
    //    num_keys_rhs should be 4
    //    the rhs child should have one more link than key
    //    num_keys_rhs should also be 4
    //
    // b) keys_.size() is even, say 8, then:
    //    median_idx should be 4 (0,1,2,3 on lhs, 5,6,7 on rhs)
    //    num_keys_rhs is *3*.
    //    num_keys_lhs is 4
    //
    //    so:
    //    median_idx = keys_.size() >> 1
    //    num_keys_rhs = keys_.size() - (median_idx + 1) // 8 -> 3, 9 -> 4, 10 -> 4, etc
    int median_idx = keys_.size() >> 1;
    int num_keys_rhs = keys_.size() - (median_idx + 1);
    *median_key = keys_[median_idx];
    new_node->children_.Resize(num_keys_rhs + 1); // One more link than key
    new_node->keys_.Resize(num_keys_rhs);

    memcpy(&new_node->keys_[0], &keys_[median_idx + 1], sizeof(int) * new_node->keys_.size());
    memcpy(&new_node->children_[0], &children_[median_idx + 1], sizeof(Node*) * new_node->children_.size());

    for (int i = 0; i < new_node->children_.size(); ++i) {
      if (!new_node->children_[i]) continue;
      new_node->children_[i]->parent_ = new_node;
    }
    keys_.Resize(median_idx);
    children_.Resize(median_idx + 1);
  } else {
    // If:
    // a) keys_.size() is odd, say 9, then:
    //    median_idx = 4
    //    num_keys_rhs = median_idx (4)
    //    num_keys_lhs = median_idx + 1 (5)
    //
    // b) keys_.size() is even, say 8, then:
    //    median_idx = is 3
    //    num_keys_rhs = 4 (4,5,6,7)
    //    num_keys_lhs = 4 (0,1,2,3)
    //
    //  so num_keys_rhs = median_idx + (keys_.size() & 1)
    int median_idx = (keys_.size() >> 1) - !(keys_.size() & 1);
    *median_key = keys_[median_idx];

    //
    int num_keys_rhs = keys_.size() >> 1;
    new_node->values_.Resize(num_keys_rhs);
    new_node->keys_.Resize(num_keys_rhs);

    int copy_from_idx = median_idx + 1;
    memcpy(&new_node->keys_[0], &keys_[copy_from_idx], sizeof(int) * new_node->keys_.size());
    memcpy(&new_node->values_[0], &values_[copy_from_idx], sizeof(int) * new_node->values_.size());

    keys_.Resize(median_idx + 1);
    values_.Resize(keys_.size());

    assert(keys_.size() == values_.size());
    assert(new_node->keys_.size() == new_node->values_.size());
  }

  return new_node;
}
