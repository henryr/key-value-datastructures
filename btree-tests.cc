#include "gtest/gtest.h"
#include "btree.h"

using namespace std;

TEST(BTree, MakeSplittedNode) {
  {
    vector<int> keys = {1,2,3};
    Node* node = new Node(keys, keys);

    int median = -1;
    Node* new_node = node->MakeSplittedNode(&median);
    ASSERT_EQ(2, median);
    ASSERT_EQ(new_node->num_keys(), 1);
    ASSERT_EQ(node->num_keys(), 1);
  }

  {
    vector<int> keys = {1,2,3,4};
    Node node(keys, keys);

    int median = -1;
    Node* new_node = node.MakeSplittedNode(&median);
    ASSERT_EQ(3, median);
    ASSERT_EQ(new_node->num_keys(), 1);
    ASSERT_EQ(node.num_keys(), 2);

    ASSERT_EQ(1, node.key_at(0));
    ASSERT_EQ(2, node.key_at(1));
    ASSERT_EQ(4, new_node->key_at(0));
  }
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
