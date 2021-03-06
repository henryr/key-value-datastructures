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

#include "gtest/gtest.h"
#include "btree.h"

using namespace std;

TEST(BTree, MakeSplittedNode) {
  {
    BTree btree(4);
    vector<int> keys = {1,2,3};
    Node node(&btree, keys, keys);

    int median = -1;
    unique_ptr<Node> new_node(node.MakeSplittedNode(&median));
    ASSERT_EQ(2, median);
    ASSERT_EQ(new_node->num_keys(), 1);
    ASSERT_EQ(node.num_keys(), 2);

    ASSERT_EQ(1, node.key_at(0));
    ASSERT_EQ(2, node.key_at(1));

    ASSERT_EQ(3, new_node->key_at(0));

    ASSERT_EQ(1, node.value_at(0));
    ASSERT_EQ(3, new_node->value_at(0));
  }

  {
    BTree btree(4);
    vector<int> keys = {1,2,3,4};
    Node node(&btree, keys, keys);

    int median = -1;
    unique_ptr<Node> new_node(node.MakeSplittedNode(&median));
    ASSERT_EQ(2, median);
    ASSERT_EQ(2, new_node->num_keys());
    ASSERT_EQ(2, node.num_keys());

    ASSERT_EQ(1, node.key_at(0));
    ASSERT_EQ(2, node.key_at(1));

    ASSERT_EQ(3, new_node->key_at(0));
    ASSERT_EQ(4, new_node->key_at(1));
  }

  {
    BTree btree(4);
    vector<int> keys = {1,2,3,4};
    vector<Node*> children = {nullptr, nullptr, nullptr, nullptr, nullptr};
    Node node(&btree, keys, children);

    int median = -1;
    unique_ptr<Node> new_node(node.MakeSplittedNode(&median));
    ASSERT_EQ(2, median);

    ASSERT_EQ(1, node.key_at(0));

    ASSERT_EQ(3, new_node->key_at(0));
    ASSERT_EQ(4, new_node->key_at(1));
  }
}

void CheckLeaf(Node* n, const vector<int>& keys, const vector<int>& values) {
  ASSERT_EQ(true, n->is_leaf());
  ASSERT_EQ(values.size(), n->num_values());
  ASSERT_EQ(keys.size(), n->num_keys());
  for (int i = 0; i < keys.size(); ++i) {
    ASSERT_EQ(keys[i], n->key_at(i));
    ASSERT_EQ(values[i], n->value_at(i));
  }
  ASSERT_EQ(n->num_keys(), n->num_values());
}

void CheckLeaf(Node* n, const vector<int>& keys) {
  CheckLeaf(n, keys, keys);
}

TEST(BTree, Split) {
  BTree btree(4);

  vector<int> keys = {1,2,3,4};
  Node node(&btree, keys, keys);

  btree.SetRoot(&node);

  node.Split();
  ASSERT_NE(btree.root_, &node);
  ASSERT_EQ(1, btree.height());

  ASSERT_EQ(false, btree.root_->is_leaf());
  ASSERT_EQ(2, btree.root_->num_children());
  ASSERT_EQ(1, btree.root_->num_keys());
  ASSERT_EQ(2, btree.root_->key_at(0));

  Node* root = btree.root_;
  Node* child0 = root->child_at(0);
  CheckLeaf(child0, {1, 2});

  Node* child1 = root->child_at(1);
  CheckLeaf(child1, {3, 4});
}

TEST(BTree, SplitUpTree) {
  BTree btree(5);

  Node root(&btree, false);
  for (int i = 0; i < 4; ++i) {
    int s = i * 10;
    vector<int> keys = { s + 1, s + 2, s + 3, s + 4 };
    Node* n = new Node(&btree, keys, keys);

    root.keys_.Insert(i, (i + 1) * 10);
    root.children_.Insert(i, n);
    n->parent_ = &root;
  }
  vector<int> final_keys = { 50, 55, 60 };
  root.children_.PushBack(new Node(&btree, final_keys, final_keys));
  btree.SetRoot(&root);

  btree.Insert(7, 7);

  // Expect root just to have one key because it is newly created after a split.
  ASSERT_EQ(1, btree.root()->num_keys());
  ASSERT_EQ(20, btree.root()->key_at(0));
  Node* child = btree.root()->child_at(0);
  ASSERT_EQ(2, child->num_keys());
  ASSERT_EQ(3, child->key_at(0));

  ASSERT_EQ(10, child->key_at(1));
  ASSERT_EQ(3, child->num_children());

  Node* leaf = child->child_at(0);
  CheckLeaf(leaf, {1, 2, 3});
}

TEST(BTree, Insert) {
  BTree btree(4);
  vector<int> keys;
  for (int i = 0; i < 200; ++i) keys.push_back(i);
  random_shuffle(keys.begin(), keys.end());

  for (int i = 0; i < 200; ++i) {
    btree.Insert(keys[i], keys[i]);
    for (int j = 0; j <= i; ++j) {
      ASSERT_NE(-1, btree.Find(keys[j])) << "Failed to find " << keys[j] << " in btree";
    }
  }
}

void DoBenchmark() {
  using namespace std::chrono;
  BTree btree(100);
  int NUM_ENTRIES = 10000000;
  vector<int> entries(NUM_ENTRIES);
  for (int i = 0; i < NUM_ENTRIES; ++i) {
    entries[i] = i;
  }
  random_shuffle(entries.begin(), entries.end());
  high_resolution_clock::time_point t1 = high_resolution_clock::now();
  for (int i: entries) btree.Insert(i, i);

  high_resolution_clock::time_point t2 = high_resolution_clock::now();
  duration<double> time_span = duration_cast<duration<double>>(t2 - t1);

  cout << "Inserting " << NUM_ENTRIES << " elements took " << time_span.count() << "s, at rate of "
       << (NUM_ENTRIES / time_span.count()) << " elements/s" << endl;
  cout << "BTree height is: " << btree.height() << endl;
  cout << "BTree num nodes is: " << btree.num_nodes() << endl;

  t1 = high_resolution_clock::now();
  for (int k: entries) {
    int found = btree.Find(k);
    if (found == -1) cout << "Val: " << k << ", found: " << found << endl;
  }
  t2 = high_resolution_clock::now();
  time_span = duration_cast<duration<double>>(t2 - t1);
  cout << "Searching for " << NUM_ENTRIES << " elements took " << time_span.count()
       << "s, at rate of " << (NUM_ENTRIES / time_span.count()) << " elements/s" << endl;

}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
