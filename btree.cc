#include <assert.h>
#include <vector>
#include <iostream>
#include <chrono>

using namespace std;

const int MAX_LEAF_KEYS = 100;
const int MAX_INTERIOR_KEYS = 100;



template <typename T>
class FastVector {
 public:
  FastVector(int capacity) : size_(0), capacity_(capacity) {
    values_ = new T[capacity];
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
  int size_;
  int capacity_;
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

 private:

  bool is_leaf_;
  int height_ = 0;
  Node* parent_ = nullptr;
  IntVector keys_;
  NodeVector children_;
  IntVector values_;
  BTree* btree_;

  Node* MakeSplittedNode();
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
  if (keys_.size() < MAX_KEYS) return;
  Node* new_node = MakeSplittedNode();
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
    int half_keys = keys_.size() >> 1;
    int median_idx = 1 + half_keys;
    new_node->children_.Resize(half_keys + 1); // One more link than key
    new_node->keys_.Resize(keys_.size() - (median_idx + 1));

    memcpy(&new_node->keys_[0], &keys_[median_idx + 1], sizeof(int) * new_node->keys_.size());
    memcpy(&new_node->children_[0], &children_[median_idx], sizeof(Node*) * new_node->children_.size());

    for (int i = 0; i < new_node->children_.size(); ++i) new_node->children[i]->parent_ = new_node;
  } else {
    // Leaf node
    int half_keys = keys_.size() >> 1;
    int median_idx = 1 + half_keys;
    new_node->values_.Resize(
  }


  int half_keys = is_leaf() ? MAX_LEAF_KEYS / 2 : MAX_INTERIOR_KEYS / 2;
  assert(half_keys * 2 == (is_leaf() ? MAX_LEAF_KEYS : MAX_INTERIOR_KEYS));

  new_node->keys_.Resize(half_keys);
  if (is_leaf_) {
    new_node->values_.Resize(half_keys);
    memcpy(&new_node->keys_[0], &keys_[half_keys], sizeof(int) * half_keys);
    memcpy(&new_node->values_[0], &values_[half_keys], sizeof(int) * half_keys);
  } else {
    int half_keys = keys_.size() >> 1;
    new_node->children_.Resize(half_keys);
    memcpy(&new_node->keys_[0], &keys_[half_keys + 1], sizeof(int) * half_keys);
    // Reassign the children to the new node
    for (int i = half_keys ; i < MAX_KEYS; ++i) {
      children_[i]->parent_ = new_node;
    }
    memcpy(&new_node->children_[0], &children_[half_keys], sizeof(Node*) * half_keys);
  }

  keys_.Resize(half_keys);
  if (is_leaf_) {
    values_.Resize(half_keys);
  } else {
    children_.Resize(half_keys);
  }

  return new_node;
}


int main(int argv, char** argc) {
  using namespace std::chrono;
  BTree btree;
  int NUM_ENTRIES = 10000000;
  vector<int> entries(NUM_ENTRIES);
  for (int i = 0; i < NUM_ENTRIES; ++i) {
    entries[i] = i;
  }
  random_shuffle(entries.begin(), entries.end());
  high_resolution_clock::time_point t1 = high_resolution_clock::now();
  for (int i = 0; i < NUM_ENTRIES; ++i) {
    btree.Insert(entries[i], entries[i]);
  }

  high_resolution_clock::time_point t2 = high_resolution_clock::now();
  duration<double> time_span = duration_cast<duration<double>>(t2 - t1);

  cout << "Inserting " << NUM_ENTRIES << " elements took " << time_span.count() << "s, at rate of "
       << (NUM_ENTRIES / time_span.count()) << " elements/s" << endl;
  cout << "BTree height is: " << btree.height() << endl;
  cout << "BTree num nodes is: " << btree.num_nodes_ << endl;

  t1 = high_resolution_clock::now();
  for (int i = 0; i < NUM_ENTRIES; i += 1) {
    int found = btree.Find(entries[i]);
    if (found == -1) cout << "Val: " << entries[i] << ", found: " << found << endl;
  }
  t2 = high_resolution_clock::now();
  time_span = duration_cast<duration<double>>(t2 - t1);
  cout << "Searching for " << NUM_ENTRIES << " elements took " << time_span.count()
       << "s, at rate of " << (NUM_ENTRIES / time_span.count()) << " elements/s" << endl;
}
