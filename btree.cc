#include <assert.h>
#include <vector>
#include <iostream>
#include <chrono>

using namespace std;

const int MAX_KEYS = 100;
const int HALF_MAX_KEYS = MAX_KEYS / 2;
class BTree;

class Node {
 public:
  vector<int> keys_;
  vector<Node*> children_;
  vector<int> values_;
  Node* parent_;
  BTree* btree_;

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

  Node(BTree* btree) : parent_(NULL), btree_(btree), is_leaf_(false) { }
  void Split();
  Node* MakeSplittedNode();

 private:
  //  friend class BTree;
  bool is_leaf_;
};

class BTree {
 public:
  Node* root_;

  int Find(int key);
  void Insert(int key, int value);
  void Delete(int key);

  BTree();

 private:
  Node* FindLeaf(int key, bool print_height = false);
};

BTree::BTree() {
  root_ = new Node(this);
  root_->SetIsLeaf(true);
}


int BTree::Find(int key) {
  Node* leaf = FindLeaf(key, true);
  for (int i = 0; i < leaf->keys_.size(); ++i) {
    if (leaf->keys_[i] == key) return leaf->values_[i];
    if (leaf->keys_[i] > key) return -1;
  }

  return -1;
}

Node* BTree::FindLeaf(int key, bool print_height) {
  int height = 0;
  Node* cur = root_;
  while (true) {
    if (cur->is_leaf()) return cur;
    bool found = false;
    for (int i = 0; i < cur->keys_.size(); ++i) {
      if (cur->keys_[i] >= key) {
        cur = cur->children_[i];
        ++height;
        found = true;
        break;
      }
    }

    if (!found) {
      // value > the largest value in the tree. We can add it to the rightmost leaf, and
      // rewrite the index.
      cur->keys_.back() = numeric_limits<int>::max();
      cur = cur->children_.back();
    }
  }
}

void BTree::Insert(int key, int value) {
  Node* node = FindLeaf(key);

  vector<int>::iterator keys_it = node->keys_.begin();
  vector<int>::iterator val_it = node->values_.begin();

  for (; keys_it != node->keys_.end(); ++keys_it, ++val_it) {
    if (*keys_it >= key) {
      node->keys_.insert(keys_it, key);
      node->values_.insert(val_it, value);
      break;
    }
  }

  if (keys_it == node->keys_.end()) {
    node->keys_.push_back(key);
    node->values_.push_back(value);
  }

  node->Split();
}

void Node::Split() {
  if (keys_.size() < MAX_KEYS) return;
  // cout << "Splitting node from " << keys_[0] << " -> " << keys_.back() << endl;
  Node* new_node = MakeSplittedNode();
  // cout << "After split, left node ends at " << keys_.back()
  //      << ", right node begins at " << new_node->keys_[0] << ", and ends at "
  //      << new_node->keys_.back() << endl;
  // cout << endl;
  if (parent_ == NULL) {
    // DCHECK(btree_->root_ == this)
    Node* root = new Node(btree_);
    root->SetIsLeaf(false);
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
  for (int i = 0; i < NUM_ENTRIES; ++i) {
    btree.Insert(i, i);
  }

  high_resolution_clock::time_point t2 = high_resolution_clock::now();
  duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
  cout << "Inserting " << NUM_ENTRIES << " elements took " << time_span.count() << "s, at rate of "
       << (NUM_ENTRIES / time_span.count()) << " elements/s" << endl;

  t1 = high_resolution_clock::now();
  for (int i = 0; i < NUM_ENTRIES; ++i) {
    int found = btree.Find(i);
    if (found == -1) cout << "Val: " << i << ", found: " << found << endl;
  }
  t2 = high_resolution_clock::now();
  time_span = duration_cast<duration<double>>(t2 - t1);
  cout << "Searching for " << NUM_ENTRIES << " elements took " << time_span.count()
       << "s, at rate of " << (NUM_ENTRIES / time_span.count()) << " elements/s" << endl;
}
