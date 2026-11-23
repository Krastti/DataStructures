#include "TreeTestHelpers.h"

#include <initializer_list>
#include <map>
#include <stdexcept>

namespace {

using tree_test::CheckedTree;
using tree_test::RedBlackTag;
using tree_test::throwsExactly;

TEST(RedBlackTreeTests, SingleKeyConstructorCreatesBlackRootWithDefaultValue) {
  RedBlackTree<int, int> tree(17);
  ASSERT_EQ(tree.size(), 1);
  EXPECT_TRUE(tree.containsKey(17));
  EXPECT_EQ(tree.get_root(), 0);
  EXPECT_TRUE(tree.isBlack(17));
  EXPECT_FALSE(tree.isRed(17));
}

TEST(RedBlackTreeTests, ColorLookupDistinguishesEmptyTreeFromMissingKey) {
  RedBlackTree<int, int> tree;
  EXPECT_TRUE(throwsExactly<std::logic_error>([&] { (void)tree.isRed(17); }));
  EXPECT_TRUE(throwsExactly<std::logic_error>([&] { (void)tree.isBlack(17); }));

  tree.insert(17, 170);
  EXPECT_TRUE(throwsExactly<std::out_of_range>([&] { (void)tree.isRed(18); }));
  EXPECT_TRUE(throwsExactly<std::out_of_range>([&] { (void)tree.isBlack(18); }));
}

TEST(RedBlackTreeTests, RebalancesInnerAndOuterInsertionsOnBothSides) {
  const std::initializer_list<int> orders[] = {{3, 2, 1}, {3, 1, 2}, {1, 2, 3}, {1, 3, 2}};
  for (const auto& order : orders) {
    SCOPED_TRACE(testing::PrintToString(order));
    CheckedTree<RedBlackTag> tree;
    std::map<int, int> expected;
    for (int key : order) {
      tree.insert(key, key * 10);
      expected.emplace(key, key * 10);
      tree.verify(expected);
    }
    EXPECT_EQ(tree.rootKey(), 2);
  }
}

TEST(RedBlackTreeTests, RebalancesRedSiblingAndNearAndFarRedChildrenOnBothSides) {
  const std::initializer_list<int> orders[] = {{2, 1, 4, 3, 5, 6}, {2, 1, 4, 3}, {2, 1, 4, 5}};
  for (int sign : {1, -1}) {
    SCOPED_TRACE(sign);
    for (const auto& order : orders) {
      SCOPED_TRACE(testing::PrintToString(order));
      CheckedTree<RedBlackTag> tree;
      std::map<int, int> expected;
      for (int key : order) {
        tree.insert(sign * key, key * 10);
        expected.emplace(sign * key, key * 10);
      }
      ASSERT_TRUE(tree.isBlack(sign));
      tree.remove(sign);
      expected.erase(sign);
      tree.verify(expected);
    }
  }
}

TEST(RedBlackTreeTests, PropagatesBlackHeightLossToRootOnBothSides) {
  for (int sign : {1, -1}) {
    SCOPED_TRACE(sign);
    CheckedTree<RedBlackTag> tree;
    std::map<int, int> expected;
    for (int key = 1; key <= 10; ++key) {
      tree.insert(sign * key, key * 10);
      expected.emplace(sign * key, key * 10);
    }
    for (int key : {8, 9}) {
      tree.remove(sign * key);
      expected.erase(sign * key);
    }
    tree.verify(expected);
    ASSERT_EQ(tree.rootKey(), sign * 4);
    for (int key : {1, 2, 3, 6}) ASSERT_TRUE(tree.isBlack(sign * key));

    tree.remove(sign);
    expected.erase(sign);
    tree.verify(expected);
    EXPECT_EQ(tree.rootKey(), sign * 4);
    EXPECT_TRUE(tree.isRed(sign * 3));
    EXPECT_TRUE(tree.isRed(sign * 6));
  }
}

} // namespace
