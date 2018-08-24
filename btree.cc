#include <assert.h>
#include <vector>
#include <iostream>
#include <chrono>

using namespace std;

const int MAX_KEYS = 100; //64 * 1024 / 8;
const int HALF_MAX_KEYS = MAX_KEYS / 2;

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
  IntVector keys_;
  NodeVector children_;
  IntVector values_;
  BTree* btree_;

  Node* parent() const { return parent_; }


  inline bool is_leaf() const { return is_leaf_; }
  int height() const { return height_; }

  Node(BTree* btree, bool is_leaf) : keys_(MAX_KEYS),
                                     children_(is_leaf ? 0 : MAX_KEYS),
                                     values_(is_leaf ? MAX_KEYS : 0),
                                     parent_(NULL),
                                     btree_(btree),
                                     is_leaf_(is_leaf),
                                     height_(0) {
  }

  void Split();
 private:
  bool is_leaf_;
  int height_;
  Node* parent_;
  Node* MakeSplittedNode();
};

class BTree {
 public:
  int Find(int key);
  void Insert(int key, int value);
  void Delete(int key);
  int height() const { return root_->height(); }
  BTree();

  int num_nodes_;
  Node* root_;

 private:
  Node* FindLeaf(int key, int* idx);
};

BTree::BTree() {
  root_ = new Node(this, true);
  num_nodes_ = 0;
}

int BTree::Find(int key) {
  int idx;
  Node* leaf = FindLeaf(key, &idx);
  if (leaf->keys_[idx] == key) return leaf->values_[idx];
  return -1;
}

Node* BTree::FindLeaf(int key, int* idx) {
  Node* cur = root_;
  while (true) {
    int size = cur->keys_.size();
    if (cur->keys_.back() < key) {
      // value > the largest value in the tree. We can add it to the rightmost leaf, and
      // rewrite the index.
      if (cur->is_leaf()) {
        *idx = size;
        return cur;
      }
      cur->keys_[cur->keys_.size() - 1] = key + 1; //numeric_limits<int>::max();
      cur = cur->children_.back();
      continue;
    }

    // Now search until we find smallest element >= than key
    int b = 0; int t = size - 1;
    int* keys = cur->keys_.values();
    while (true) {
      if (b >= t) {
        if (cur->is_leaf()) {
          *idx = b;
          return cur;
        }
        cur = cur->children_[b];
        break;
      }

      if (t - b < 16) {
        for (int i = b; i <= t; ++i) {
          if (keys[i] >= key) {
            if (cur->is_leaf()) {
              *idx = i;
              return cur;
            }
            cur = cur->children_[i];
            break;
          }
        }
        break;
      }

      if (keys[b] == key) {
        if (cur->is_leaf()) {
          *idx = b;
          return cur;
        }
        cur = cur->children_[b];
        break;
      }

      if (keys[t] == key) {
        if (cur->is_leaf()) {
          *idx = t;
          return cur;
        }
        cur = cur->children_[t];
        break;
      }

      int mid = (b + t) / 2;
      if (keys[mid] == key) {
        if (cur->is_leaf()) {
          *idx = mid;
          return cur;
        }
        cur = cur->children_[mid];
        break;
      }

      if (keys[mid] < key) {
        b = mid + 1;
      } else {
        t = mid;
      }
    }
  }
}

void BTree::Insert(int key, int value) {
  int idx;
  Node* node = FindLeaf(key, &idx);

  if (idx != node->keys_.size()) {
    node->keys_.Insert(idx, key);
    node->values_.Insert(idx, value);
  } else {
    node->keys_.PushBack(key);
    node->values_.PushBack(value);
  }

  node->Split();
}

void Node::Split() {
  if (keys_.size() < MAX_KEYS) return;
  Node* new_node = MakeSplittedNode();
  ++btree_->num_nodes_;
  if (parent_ == NULL) {
    Node* root = new Node(btree_, false);
    root->height_ = height_ + 1;
    root->keys_.PushBack(keys_.back());
    root->children_.PushBack(this);
    root->keys_.PushBack(new_node->keys_.back());
    root->children_.PushBack(new_node);

    new_node->parent_ = root;
    parent_ = root;

    btree_->root_ = root;
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

Node* Node::MakeSplittedNode() {
  Node* new_node = new Node(btree_, is_leaf_);
  new_node->parent_ = parent_;
  new_node->keys_.Resize(HALF_MAX_KEYS);
  if (is_leaf_) {
    new_node->values_.Resize(HALF_MAX_KEYS);
    memcpy(&new_node->keys_[0], &keys_[HALF_MAX_KEYS], sizeof(int) * HALF_MAX_KEYS);
    memcpy(&new_node->values_[0], &values_[HALF_MAX_KEYS], sizeof(int) * HALF_MAX_KEYS);
  } else {
    new_node->children_.Resize(HALF_MAX_KEYS);
    memcpy(&new_node->keys_[0], &keys_[HALF_MAX_KEYS], sizeof(int) * HALF_MAX_KEYS);
    for (int i = (HALF_MAX_KEYS); i < MAX_KEYS; ++i) {
      children_[i]->parent_ = new_node;
    }
    memcpy(&new_node->children_[0], &children_[HALF_MAX_KEYS], sizeof(Node*) * HALF_MAX_KEYS);
  }

  keys_.Resize(HALF_MAX_KEYS);
  if (is_leaf_) {
    values_.Resize(HALF_MAX_KEYS);
  } else {
    children_.Resize(HALF_MAX_KEYS);
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
