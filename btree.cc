#include <assert.h>
#include <vector>
#include <iostream>
#include <chrono>

using namespace std;
using namespace std::chrono;

const int MAX_KEYS = 1000; //64 * 1024 / 8;
const int HALF_MAX_KEYS = MAX_KEYS / 2;
class BTree;

class Node {
 public:
  vector<int> keys_;
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

  bool is_leaf() const { return is_leaf_; }

  Node(BTree* btree) : parent_(NULL), btree_(btree), is_leaf_(false), height_(0) {
    keys_.reserve(MAX_KEYS);
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
  int num_find_loops_;
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
  if (leaf->keys_[idx] == key) return leaf->values_[idx];
  return -1;  
}

Node* BTree::FindLeaf(int key, int* idx) {
  Node* cur = root_;
  //  cout << "---------------- " << key << endl;
  while (true) {
    int size = cur->keys_.size();    
    if (cur->keys_.back() < key) {
      // value > the largest value in the tree. We can add it to the rightmost leaf, and
      // rewrite the index.
      if (cur->is_leaf()) {
        *idx = size;
        return cur;
      }
      cur->keys_.back() = numeric_limits<int>::max();
      cur = cur->children_.back();
      continue;
    }
            
    // Now search until we find smallest element >= than key          
    int b = 0; int t = size - 1;
    int* keys = &cur->keys_[0];
    while (true) {
      //      cout << "b: " << b << ", t: " << t << endl;
      if (b >= t) {
        if (cur->is_leaf()) {
          *idx = b;
          return cur;
        }
        cur = cur->children_[b];
        break;
      }

      int l = keys[b];
      if (l == key) {
        if (cur->is_leaf()) {
          *idx = b;
          return cur;
        }
        cur = cur->children_[b];
        break;
      }        
        
      int h = keys[t];
      if (h == key) {
        if (cur->is_leaf()) {
          *idx = t;
          return cur;
        }
        cur = cur->children_[t];
        break;
      }

      int mid = (b + t) / 2;
      int mk = keys[mid];
      // if (mk == key) return (*leaf_values)[mid];

      if (mk < key) {
        b = mid + 1;
      } else {
        t = mid;
      }
    }
  }    
}

void BTree::Insert(int key, int value) {  
  high_resolution_clock::time_point t1 = high_resolution_clock::now();
  int idx;
  Node* node = FindLeaf(key, &idx);
  high_resolution_clock::time_point t2 = high_resolution_clock::now();
  total_time_find_leaf_ += duration_cast<duration<double>>(t2 - t1);

  vector<int>::iterator keys_it = node->keys_.begin();
  vector<int>::iterator val_it = node->values_.begin();

  if (idx != node->keys_.size()) {
    node->keys_.insert(keys_it + idx, key);
    node->values_.insert(val_it + idx, value);
  } else {
    node->keys_.push_back(key);
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
    root->keys_.push_back(keys_.back());
    root->children_.push_back(this);
    root->keys_.push_back(new_node->keys_.back());
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
  vector<int>::iterator keys_it = parent_->keys_.begin();
  vector<Node*>::iterator val_it = parent_->children_.begin();
  int key = new_node->keys_.back();

  for (; keys_it != parent_->keys_.end(); ++keys_it, ++val_it) {
    if (*val_it == this) {
      // Rewrite the original key to point to the new max key for this node
      *keys_it = keys_.back();
    } else if (*keys_it >= key) {
      parent_->keys_.insert(keys_it, key);
      parent_->children_.insert(val_it, new_node);
      break;
    }
  }

  if (keys_it == parent_->keys_.end()) {
    parent_->keys_.push_back(key);
    parent_->children_.push_back(new_node);
  }

  parent_->Split();
}

Node* Node::MakeSplittedNode() {
  Check();
  Node* new_node = new Node(btree_);
  new_node->parent_ = parent_;
  new_node->keys_.resize(HALF_MAX_KEYS);
  if (is_leaf_) {
    new_node->values_.resize(HALF_MAX_KEYS);
  } else {
    new_node->children_.resize(HALF_MAX_KEYS);
  }
  for (int i = (HALF_MAX_KEYS); i < MAX_KEYS; ++i) {
    new_node->keys_[i - HALF_MAX_KEYS] = keys_[i];
    if (is_leaf_) {
      new_node->values_[i - HALF_MAX_KEYS] = values_[i];
    } else {
      children_[i]->parent_ = new_node;
      new_node->children_[i -HALF_MAX_KEYS] = children_[i];
    }
  }

  keys_.resize(HALF_MAX_KEYS);
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
  high_resolution_clock::time_point t1 = high_resolution_clock::now();
  for (int i = 0; i < NUM_ENTRIES; i += 1) {
    btree.Insert(i, i);
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
    int found = btree.Find(i);
    if (found == -1) cout << "Val: " << i << ", found: " << found << endl;
  }
  t2 = high_resolution_clock::now();
  time_span = duration_cast<duration<double>>(t2 - t1);
  cout << "Searching for " << NUM_ENTRIES << " elements took " << time_span.count()
       << "s, at rate of " << (NUM_ENTRIES / time_span.count()) << " elements/s" << endl;
  cout << "Num find loops: " << btree.num_find_loops_ << endl;
}
