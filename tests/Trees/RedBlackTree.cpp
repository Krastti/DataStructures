#include "../../include/Trees/RedBlackTree.h"
#include <gtest/gtest.h>

#include <cstdlib>

TEST(ColorNodeTest, ColorNodeTest) {
  Map<int, int> tree;

  tree.insert(10, 10);
  tree.insert(11, 10);
  tree.insert(12, 10);
  tree.insert(13, 10);
  tree.insert(14, 10);

  EXPECT_EQ(tree.isBlack(11), 1);
  EXPECT_EQ(tree.isBlack(10), 1);
  EXPECT_EQ(tree.isBlack(13), 1);
  EXPECT_EQ(tree.isRed(12), 1);
  EXPECT_EQ(tree.isRed(14), 1);
}