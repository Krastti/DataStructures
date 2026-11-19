#include "Trees/BinaryTree.h"
#include "Trees/RedBlackTree.h"
#include "Trees/Set.h"
#include "../Array/ArrayTestHelpers.h"
#include <gtest/gtest.h>

#include <string>
#include <random>
#include <set>
#include <type_traits>
#include <utility>

static_assert(!std::is_convertible_v<RedBlackTree<int, int>*, BinaryTree<int, int>*>);
static_assert(!std::is_convertible_v<RedBlackTree<int, int>&, BinaryTree<int, int>&>);
static_assert(!std::is_assignable_v<RedBlackTree<int, int>&, BinaryTree<int, int>&&>);
static_assert(!std::is_constructible_v<RedBlackTree<int, int>, BinaryTree<int, int>&&>);

TEST(BinaryTreeTests, DuplicateInsertKeepsOneNodeAndOrderedTraversals) {
  BinaryTree<int, std::string> tree;
  tree.insert(4, "four");
  tree.insert(2, "two");
  tree.insert(6, "six");
  tree.insert(4, "duplicate");

  EXPECT_EQ(tree.size(), 3);
  EXPECT_EQ(tree.at(4), "four");
  EXPECT_EQ(toVector(tree.keys()), (std::vector<int>{2, 4, 6}));
  EXPECT_EQ(toVector(tree.range(2, 4)),
            (std::vector<std::pair<int, std::string>>{{2, "two"}, {4, "four"}}));
  EXPECT_FALSE(tree.containsKey(5));
}

TEST(BinaryTreeTests, RemoveAndMovePreserveOwnershipAndSize) {
  BinaryTree<int, int> original;
  for (int key : {4, 2, 6, 1, 3, 5, 7}) original.insert(key, key * 10);
  original.remove(4);
  EXPECT_EQ(original.size(), 6);
  EXPECT_FALSE(original.containsKey(4));

  BinaryTree<int, int> moved(std::move(original));
  EXPECT_EQ(original.size(), 0);
  EXPECT_EQ(moved.size(), 6);
  EXPECT_EQ(toVector(moved.keys()), (std::vector<int>{1, 2, 3, 5, 6, 7}));
}

TEST(MapTests, RedBlackTreeExposesMutableValuesAndMaintainsSize) {
  Map<int, Array<int>> map;
  for (int key = 10; key >= 1; --key) {
    Array<int> values;
    values.push_back(key);
    map.insert(key, std::move(values));
  }
  EXPECT_EQ(map.size(), 10);
  EXPECT_TRUE(map.containsKey(5));
  map.at(5).push_back(50);
  EXPECT_EQ(toVector(map.at(5)), (std::vector<int>{5, 50}));
  EXPECT_EQ(toVector(map.keys()), (std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}));

  map.remove(1);
  map.remove(10);
  EXPECT_EQ(map.size(), 8);
  EXPECT_FALSE(map.containsKey(1));
}

TEST(MapTests, MaintainsOrderingAcrossSequentialInsertionsAndDeletions) {
  Map<int, int> map;
  constexpr int keyCount = 128;
  for (int key = 0; key < keyCount; ++key) map.insert(key, key * 10);
  ASSERT_EQ(map.size(), keyCount);

  for (int key = 0; key < keyCount; key += 2) map.remove(key);
  Array<int> oddKeys;
  for (int key = 1; key < keyCount; key += 2) oddKeys.push_back(key);
  EXPECT_EQ(map.size(), keyCount / 2);
  EXPECT_EQ(toVector(map.keys()), toVector(oddKeys));
  for (int key = 1; key < keyCount; key += 2) map.remove(key);

  EXPECT_EQ(map.size(), 0);
  EXPECT_EQ(map.keys().size(), 0);
}

TEST(SetTests, IgnoresDuplicatesAndRemovesKeys) {
  Set<int> values;
  for (int key : {4, 2, 6, 1, 3, 5, 7}) values.insert(key);
  values.insert(4);
  EXPECT_TRUE(values.contains(4));
  EXPECT_TRUE(values.contains(7));

  values.remove(4);
  EXPECT_FALSE(values.contains(4));
  EXPECT_TRUE(values.contains(5));
  values.remove(1);
  values.remove(7);
  values.remove(2);
  EXPECT_FALSE(values.contains(2));
}

TEST(SetTests, MissingKeysThrowOutOfRangeOnEmptyAndNonemptySet) {
  Set<int> values;
  EXPECT_FALSE(values.contains(1));
  EXPECT_THROW(values.remove(1), std::out_of_range);
  values.insert(2);
  EXPECT_THROW(values.remove(1), std::out_of_range);
  EXPECT_TRUE(values.contains(2));
  values.remove(2);
  EXPECT_THROW(values.remove(2), std::out_of_range);
}

TEST(SetTests, SingleKeyConstructorAndMixedOperationsMatchStdSet) {
  Set<int> values(10);
  std::set<int> expected{10};
  std::mt19937 random(147);
  for (int step = 0; step < 10000; ++step) {
    int key = static_cast<int>(random() % 128);
    if (random() % 2) {
      values.insert(key);
      expected.insert(key);
    } else if (expected.erase(key)) {
      values.remove(key);
    } else {
      EXPECT_THROW(values.remove(key), std::out_of_range);
    }
    for (int candidate = 0; candidate < 128; ++candidate) {
      ASSERT_EQ(values.contains(candidate), expected.contains(candidate));
    }
  }
  for (int key : expected) values.remove(key);
  EXPECT_FALSE(values.contains(10));
  EXPECT_THROW(values.remove(10), std::out_of_range);
}
