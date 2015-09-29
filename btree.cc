#include <vector>
#include <iostream>

using namespace std;

const int MAX_KEYS = 10;
class BTree;

class Node {
 public:
  vector<int> keys_;
  vector<Node*> children_;
  bool is_leaf_;
  vector<int> values_;
  Node* parent_;
  BTree* btree_;

 private:
  Node(BTree* btree) : parent_(NULL), btree_(btree) { }
  void Split();
  Node* MakeSplittedNode();
  friend class BTree;
};

class BTree {
 public:
  Node* root_;

  int Find(int key);
  void Insert(int key, int value);
  void Delete(int key);

  BTree();

 private:
  Node* FindLeaf(int key);
};

BTree::BTree() {
  root_ = new Node(this);
  root_->is_leaf_ = true;
}


int BTree::Find(int key) {
  Node* leaf = FindLeaf(key);
  for (int i = 0; i < leaf->keys_.size(); ++i) {
    if (leaf->keys_[i] == key) return leaf->values_[i];
    if (leaf->keys_[i] > key) return -1;
  }

  return -1;
}

Node* BTree::FindLeaf(int key) {
  Node* cur = root_;
  while (true) {
    if (cur->is_leaf_) return cur;
    bool found = false;
    for (int i = 0; i < cur->keys_.size(); ++i) {
      if (cur->keys_[i] > key) {
        cur = cur->children_[i];
        found = true;
        break;
      }
    }

    if (!found) {
      // value > the largest value in the tree. We can add it to the rightmost leaf, and
      // rewrite the index.
      cur->keys_.back() = key + 1;
      cur = cur->children_.back();
    }
  }
}

void BTree::Insert(int key, int value) {
  Node* node = FindLeaf(key);

  vector<int>::iterator keys_it = node->keys_.begin();
  vector<int>::iterator val_it = node->values_.begin();

  for (; keys_it != node->keys_.end(); ++keys_it, ++val_it) {
    if (*keys_it > key) {
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
  Node* new_node = MakeSplittedNode();
  if (parent_ == NULL) {
    // DCHECK(btree_->root_ == this)
    Node* root = new Node(btree_);
    root->is_leaf_ = false;
    root->keys_.push_back(new_node->keys_[0]);
    root->children_.push_back(this);
    root->keys_.push_back(new_node->keys_.back());
    root->children_.push_back(new_node);

    new_node->parent_ = root;
    parent_ = root;

    btree_->root_ = root;
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
      *keys_it = new_node->keys_[0];
    } else if (*keys_it > key) {
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
  Node* new_node = new Node(btree_);
  new_node->parent_ = parent_;
  for (int i = (MAX_KEYS / 2); i < MAX_KEYS; ++i) {
    new_node->keys_.push_back(keys_[i]);
    if (is_leaf_) {
      new_node->values_.push_back(values_[i]);
    } else {
      children_[i]->parent_ = new_node;
      new_node->children_.push_back(children_[i]);
    }
  }

  keys_.resize(MAX_KEYS / 2);
  if (is_leaf_) {
    values_.resize(MAX_KEYS / 2);
    new_node->is_leaf_ = true;
  } else {
    children_.resize(MAX_KEYS / 2);
  }

  return new_node;
}


int main(int argv, char** argc) {
  BTree btree;
  int NUM_ENTRIES = 50;
  for (int i = 0; i < NUM_ENTRIES; ++i) {
    btree.Insert(i, i);
  }

  for (int i = 0; i < NUM_ENTRIES; ++i) {
    int found = btree.Find(i);
    cout << "Val: " << i << ", found: " << found << endl;
  }
}
