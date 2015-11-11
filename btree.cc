#include <assert.h>
#include <vector>
#include <iostream>
#include <chrono>

using namespace std;
using namespace std::chrono;

const int MAX_KEYS = 100; //64 * 1024 / 8;
const int HALF_MAX_KEYS = MAX_KEYS / 2;
class BTree;

class IntVector {
 public:
  int* values_;
  int size_;
  int capacity_;

  IntVector(int capacity) : size_(0), capacity_(capacity) {
    values_ = new int[capacity];    
  }

  void PushBack(int val) {
    //    assert(size_ < capacity_);
    values_[size_++] = val;
  }

  void Resize(int size) {
    // assert(size < capacity_);
    size_ = size;
  }

  void Insert(int idx, int val) {
    // assert(size_ < capacity_);
    memmove(&values_[idx + 1], &values_[idx], sizeof(int) * (size_ - idx));
    ++size_;
    values_[idx] = val;
    // assert_in_order();
  }

  void assert_in_order() {
    if (size_ <= 1) return;
    for (int i = 1; i < size_; ++i) {
      assert(values_[i-1] <= values_[i]);
    }
  }

  int size() const { return size_; }
  int capacity() const { return capacity_; }
  int* values() const { return values_; }
  int back() const { return values_[size_ - 1]; }
};

class Node {
 public:
  IntVector keys_;
  //vector<int> keys_;
  vector<Node*> children_;
  vector<int> values_;
  Node* parent_;
  BTree* btree_;
  int height_;

  void Check() {
    return;
    if (is_leaf_) {
      assert(children_.size() == 0);
      assert(values_.size() == keys_.size());
    } else {
      assert(values_.size() == 0);
      assert(children_.size() == keys_.size());
    }
  }

  void SetIsLeaf(bool leaf) {
    is_leaf_ = leaf;
    //    assert((is_leaf_ && children_.size() == 0) || values_.size() == 0);
  }

  inline bool is_leaf() const { return is_leaf_; }

  Node(BTree* btree) : keys_(MAX_KEYS), parent_(NULL), btree_(btree), is_leaf_(false), height_(0) {    
    values_.reserve(MAX_KEYS);
  }
  void Split();
  Node* MakeSplittedNode();

 private:
  //  friend class BTree;
  bool is_leaf_;
};

class BTree {
 public:
  Node* root_;
  int num_nodes_;
  duration<double> total_time_find_leaf_;

  int Find(int key);
  void Insert(int key, int value);
  void Delete(int key);
  int height() const { return root_->height_; }
  BTree();

 private:
  Node* FindLeaf(int key, int* idx);
};

BTree::BTree() {
  root_ = new Node(this);
  root_->SetIsLeaf(true);
  num_nodes_ = 0;
}

int BTree::Find(int key) {
  int idx;
  Node* leaf = FindLeaf(key, &idx);
  if (leaf->keys_.values()[idx] == key) return leaf->values_[idx];
  return -1;  
}

Node* BTree::FindLeaf(int key, int* idx) {
  Node* cur = root_;
  //  cout << "---------------- " << key << endl;
  while (true) {
    int size = cur->keys_.size();
    // if (key == 46) {
    //   cout << "46!" << endl;
    // }
    if (cur->keys_.back() < key) {
      // value > the largest value in the tree. We can add it to the rightmost leaf, and
      // rewrite the index.
      if (cur->is_leaf()) {
        *idx = size;
        return cur;
      }
      cur->keys_.values()[cur->keys_.size() - 1] = numeric_limits<int>::max();
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

      if (t - b < 64) {
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
  //high_resolution_clock::time_point t1 = high_resolution_clock::now();
  int idx;
  Node* node = FindLeaf(key, &idx);
  //high_resolution_clock::time_point t2 = high_resolution_clock::now();
  //total_time_find_leaf_ += duration_cast<duration<double>>(t2 - t1);

  // vector<int>::iterator keys_it = node->keys_.begin();
  vector<int>::iterator val_it = node->values_.begin();

  if (idx != node->keys_.size()) {
    //cout << "idx: " << idx << endl;
    //    node->keys_.PushBack(key);
    //cout << "size: " << node->keys_.size() << endl;
    // int* keys = &node->keys_[0];
    // memmove(&keys[idx + 1], &keys[idx], sizeof(int) * (node->keys_.size() - idx));
    // keys[idx] = key;
    node->keys_.Insert(idx, key);
    node->values_.insert(val_it + idx, value);
  } else {
    // node->keys_.assert_in_order();
    node->keys_.PushBack(key);
    // node->keys_.assert_in_order();
    node->values_.push_back(value);
  }

  node->Split();
}

void Node::Split() {
  if (keys_.size() < MAX_KEYS) return;
  Node* new_node = MakeSplittedNode();
  ++btree_->num_nodes_;
  if (parent_ == NULL) {
    Node* root = new Node(btree_);
    root->SetIsLeaf(false);
    root->height_ = height_ + 1;
    root->keys_.PushBack(keys_.back());
    root->children_.push_back(this);
    root->keys_.PushBack(new_node->keys_.back());
    root->children_.push_back(new_node);

    new_node->parent_ = root;
    parent_ = root;

    btree_->root_ = root;

    // root->Check();
    // new_node->Check();
    // this->Check();
    return;
  }

  // Otherwise parent gets both
  // push_back into parent keys / values
  //vector<int>::iterator keys_it = parent_->keys_.begin();
  vector<Node*>::iterator val_it = parent_->children_.begin();
  int key = new_node->keys_.back();
  bool found = false;
  for (int i = 0; i < parent_->keys_.size(); ++i, ++val_it) {
    if (*val_it == this) {
      parent_->keys_.values()[i] = keys_.back();
      // parent_->keys_.assert_in_order();
    } else if (parent_->keys_.values()[i] >= key) {
      parent_->keys_.Insert(i, key);
      parent_->children_.insert(val_it, new_node);
      found = true;
      break;
    }
  }

  // for (; keys_it != parent_->keys_.end(); ++keys_it, ++val_it) {
  //   if (*val_it == this) {
  //     // Rewrite the original key to point to the new max key for this node
  //     *keys_it = keys_.back();
  //   } else if (*keys_it >= key) {
  //     parent_->keys_.insert(keys_it, key);
  //     parent_->children_.insert(val_it, new_node);
  //     break;
  //   }
  // }

  if (!found) { // keys_it == parent_->keys_.end()) {
    parent_->keys_.PushBack(key);
    parent_->children_.push_back(new_node);
  }

  parent_->Split();
}

Node* Node::MakeSplittedNode() {
  Check();
  Node* new_node = new Node(btree_);
  new_node->parent_ = parent_;
  new_node->keys_.Resize(HALF_MAX_KEYS);
  if (is_leaf_) {
    new_node->values_.resize(HALF_MAX_KEYS);
    memcpy(&new_node->keys_.values()[0], &keys_.values()[HALF_MAX_KEYS], sizeof(int) * HALF_MAX_KEYS);
    memcpy(&new_node->values_[0], &values_[HALF_MAX_KEYS], sizeof(int) * HALF_MAX_KEYS);
  } else {
    new_node->children_.resize(HALF_MAX_KEYS);
    memcpy(&new_node->keys_.values()[0], &keys_.values()[HALF_MAX_KEYS], sizeof(int) * HALF_MAX_KEYS);
    for (int i = (HALF_MAX_KEYS); i < MAX_KEYS; ++i) {
      children_[i]->parent_ = new_node;
    }
    memcpy(&new_node->children_[0], &children_[HALF_MAX_KEYS], sizeof(Node*) * HALF_MAX_KEYS);
  }
  
  keys_.Resize(HALF_MAX_KEYS);
  if (is_leaf_) {
    values_.resize(HALF_MAX_KEYS);
    new_node->SetIsLeaf(true);
  } else {
    children_.resize(HALF_MAX_KEYS);
  }

  // Check();
  // new_node->Check();

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
  for (int i = 0; i < NUM_ENTRIES; i += 1) {
    // cout << "Inserting: " << entries[i] << endl;
    btree.Insert(entries[i], entries[i]);
    // assert(btree.Find(entries[i]) != -1);
  }

  high_resolution_clock::time_point t2 = high_resolution_clock::now();
  duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
  
  cout << "Inserting " << NUM_ENTRIES << " elements took " << time_span.count() << "s, at rate of "
       << (NUM_ENTRIES / time_span.count()) << " elements/s" << endl;
  cout << "BTree height is: " << btree.height() << endl;
  cout << "BTree num nodes is: " << btree.num_nodes_ << endl;
  double ratio = 100.0 * btree.total_time_find_leaf_.count() / time_span.count();
  cout << "Time spent in FindLeaf() is: " << btree.total_time_find_leaf_.count() << "s" << endl;
  cout << "FindLeaf() took " << ratio << "% of execution time" << endl;

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
