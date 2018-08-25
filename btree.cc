#include <assert.h>
#include <vector>
#include <iostream>
#include <chrono>

#include "btree.h"

using namespace std;

BTree::BTree() {
  root_ = new Node(this, true);
}

int BTree::Find(int key) {
  int idx;
  Node* leaf = FindLeaf(key, &idx);
  if (idx == -1 || leaf->key_at(idx) != key) return -1;
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

    cur = cur->child_at(next_idx);
  }
}

int Node::FindKeyIdx(int key) {
  // For now, linear search
  for (int i = 0; i < keys_.size(); ++i) {
    if (keys_[i] >= key) return i;
  }

  // Return the index of the last link, which is one more than the index of the last key.
  return is_leaf() ? keys_.size() : -1;
}

  // while (true) {
//     int size = cur->keys_.size();
//     if (cur->key_at(size - 1) < key) {
//       // value > the largest value in the tree. We can add it to the rightmost leaf, and
//       // rewrite the index.
//       if (cur->is_leaf()) {
//         *idx = size;
//         return cur;
//       }
//       cur->update_key(size - 1, key + 1);
//       cur = cur->rightmost_child();
//       continue;
//     }

//     // Now search until we find smallest element >= than key
//     int b = 0; int t = size - 1;
//     // int* keys = cur->keys_.values();
//     while (true) {
//       if (b >= t) {
//         if (cur->is_leaf()) {
//           *idx = b;
//           return cur;
//         }
//         cur = cur->children_[b];
//         break;
//       }

//       if (t - b < 16) {
//         for (int i = b; i <= t; ++i) {
//           if (cur->key_at(i) >= key) {
//             if (cur->is_leaf()) {
//               *idx = i;
//               return cur;
//             }
//             cur = cur->children_[i];
//             break;
//           }
//         }
//         break;
//       }

//       if (cur->key_at(b) == key) {
//         if (cur->is_leaf()) {
//           *idx = b;
//           return cur;
//         }
//         cur = cur->children_[b];
//         break;
//       }

//       if (cur->key_at(t) == key) {
//         if (cur->is_leaf()) {
//           *idx = t;
//           return cur;
//         }
//         cur = cur->children_[t];
//         break;
//       }

//       int mid = (b + t) / 2;
//       if (cur->key_at(mid) == key) {
//         if (cur->is_leaf()) {
//           *idx = mid;
//           return cur;
//         }
//         cur = cur->children_[mid];
//         break;
//       }

//       if (cur->key_at(mid) < key) {
//         b = mid + 1;
//       } else {
//         t = mid;
//       }
//     }
//   }
// }

void BTree::Insert(int key, int value) {
  int idx;
  Node* node = FindLeaf(key, &idx);
  node->InsertKeyValue(idx, key, value);
  node->Split();
}

void Node::Split() {
  // TODO: Move some logic to BTree
  if (keys_.size() < (is_leaf() ? MAX_LEAF_KEYS : MAX_INTERIOR_KEYS)) return;
  int median;
  Node* new_node = MakeSplittedNode(&median);
  ++btree_->num_nodes_;
  if (!parent_) {
    Node* root = new Node(btree_, false);
    root->height_ = height_ + 1;
    root->keys_.PushBack(keys_.back());
    root->children_.PushBack(this);
    root->keys_.PushBack(new_node->keys_.back());
    root->children_.PushBack(new_node);

    new_node->parent_ = root;
    parent_ = root;

    btree_->SetRoot(root);
    return;
  }

  // Otherwise parent gets both
  // push_back into parent keys / values
  int key = new_node->keys_.back();
  bool found = false;
  for (int i = 0; i < parent_->keys_.size(); ++i) {
    if (parent_->children_[i] == this) {
      parent_->keys_[i] = keys_.back();
    } else if (parent_->keys_[i] >= key) {
      parent_->keys_.Insert(i, key);
      parent_->children_.Insert(i, new_node);
      found = true;
      break;
    }
  }

  if (!found) {
    parent_->keys_.PushBack(key);
    parent_->children_.PushBack(new_node);
  }

  parent_->Split();
}

Node* Node::MakeSplittedNode(int* median_key) {
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
    int half_keys = keys_.size() >> 1;
    int median_idx = half_keys;
    int num_keys_rhs = keys_.size() - (median_idx + 1);
    *median_key = keys_[median_idx];
    new_node->children_.Resize(num_keys_rhs + 1); // One more link than key
    //assert(half_keys == keys_.size() - (median_idx + 1));
    new_node->keys_.Resize(num_keys_rhs);

    memcpy(&new_node->keys_[0], &keys_[median_idx + 1], sizeof(int) * new_node->keys_.size());
    memcpy(&new_node->children_[0], &children_[median_idx], sizeof(Node*) * new_node->children_.size());

    for (int i = 0; i < new_node->children_.size(); ++i) {
      if (!new_node->children_[i]) continue;
      new_node->children_[i]->parent_ = new_node;
    }
    keys_.Resize(half_keys);
  } else {
    // If:
    // a) keys_.size() is odd, say 9, then:
    //    median_idx = 4
    //    num_keys_rhs = median_idx (4)
    //    num_keys_lhs = median_idx + 1 (5)
    //
    // b) keys_.size() is even, say 8, then:
    //    median_idx = is 4
    //    num_keys_rhs = 4 (4,5,6,7)
    //    num_keys_lhs = 4 (0,1,2,3)
    //
    //  so num_keys_rhs = median_idx + (keys_.size() & 1)
    int median_idx = keys_.size() >> 1;
    *median_key = keys_[median_idx];

    //
    int num_keys_rhs = median_idx;
    new_node->values_.Resize(num_keys_rhs);
    new_node->keys_.Resize(num_keys_rhs);

    int copy_from_idx = median_idx + (keys_.size() & 1);
    memcpy(&new_node->keys_[0], &keys_[copy_from_idx], sizeof(int) * new_node->keys_.size());
    memcpy(&new_node->values_[0], &values_[copy_from_idx], sizeof(int) * new_node->values_.size());

    keys_.Resize(median_idx + (keys_.size() & 1));
    values_.Resize(keys_.size());

    assert(keys_.size() == values_.size());
    assert(new_node->keys_.size() == new_node->values_.size());
  }

  return new_node;
}


// int main(int argv, char** argc) {
//   using namespace std::chrono;
//   BTree btree;
//   int NUM_ENTRIES = 10000000;
//   vector<int> entries(NUM_ENTRIES);
//   for (int i = 0; i < NUM_ENTRIES; ++i) {
//     entries[i] = i;
//   }
//   random_shuffle(entries.begin(), entries.end());
//   high_resolution_clock::time_point t1 = high_resolution_clock::now();
//   for (int i = 0; i < NUM_ENTRIES; ++i) {
//     btree.Insert(entries[i], entries[i]);
//   }

//   high_resolution_clock::time_point t2 = high_resolution_clock::now();
//   duration<double> time_span = duration_cast<duration<double>>(t2 - t1);

//   cout << "Inserting " << NUM_ENTRIES << " elements took " << time_span.count() << "s, at rate of "
//        << (NUM_ENTRIES / time_span.count()) << " elements/s" << endl;
//   cout << "BTree height is: " << btree.height() << endl;
//   cout << "BTree num nodes is: " << btree.num_nodes_ << endl;

//   t1 = high_resolution_clock::now();
//   for (int i = 0; i < NUM_ENTRIES; i += 1) {
//     int found = btree.Find(entries[i]);
//     if (found == -1) cout << "Val: " << entries[i] << ", found: " << found << endl;
//   }
//   t2 = high_resolution_clock::now();
//   time_span = duration_cast<duration<double>>(t2 - t1);
//   cout << "Searching for " << NUM_ENTRIES << " elements took " << time_span.count()
//        << "s, at rate of " << (NUM_ENTRIES / time_span.count()) << " elements/s" << endl;
// }
