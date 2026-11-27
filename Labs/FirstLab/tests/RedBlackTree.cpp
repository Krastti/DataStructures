#include "BinaryTree.h"
#include "RedBlackTree.h"
#include "../../../DataStructures/tests/Array/ArrayTestHelpers.h"
#include "../../../DataStructures/tests/AllocationFailure.h"
#include "TreeTestHelpers.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <new>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <typeinfo>
#include <utility>
#include <vector>

namespace {

using tree_test::CoutRedirect;
using tree_test::Faulty;
using tree_test::ComparisonFailure;
using tree_test::CopyFailure;
using tree_test::PrintFailure;
using tree_test::UnprintableKey;
using tree_test::throwsExactly;


TEST(RedBlackTreeTests, InsertsKeyAndValue) {
  RedBlackTree<int, int> tree;

  tree.insert(4, 40);

  EXPECT_EQ(tree.size(), 1);
  EXPECT_EQ(tree.at(4), 40);
}

TEST(RedBlackTreeTests, FindsValueByKey) {
  RedBlackTree<int, int> tree;
  tree.insert(4, 40);
  tree.insert(2, 20);

  EXPECT_EQ(tree.at(2), 20);
}

TEST(RedBlackTreeTests, RemovesKey) {
  RedBlackTree<int, int> tree;
  tree.insert(4, 40);
  tree.insert(2, 20);

  tree.remove(2);

  EXPECT_FALSE(tree.containsKey(2));
  EXPECT_TRUE(tree.containsKey(4));
  EXPECT_EQ(tree.size(), 1);
}

class CheckedRedBlackTree : public RedBlackTree<int, int> {
  using Node = BinaryTree<int, int>::Node;

public:
  void verify(const std::map<int, int>& expected) const {
    std::size_t count = 0;
    ASSERT_NO_FATAL_FAILURE(tree_test::verifySearchOrder(
        root, static_cast<const Node*>(nullptr), std::numeric_limits<long long>::min(),
        std::numeric_limits<long long>::max(), count));
    EXPECT_EQ(count, size());
    ASSERT_EQ(count, expected.size());
    if (root != nullptr) {
      EXPECT_TRUE(isBlack(root->key));
      verifyBlackHeight(root);
    }
    const auto actual = entries();
    ASSERT_EQ(actual.size(), expected.size());
    std::size_t index = 0;
    for (const auto& [key, value] : expected) {
      EXPECT_EQ(actual[index].first, key);
      EXPECT_EQ(actual[index].second, value);
      ++index;
    }
  }

  std::optional<int> rootKey() const {
    if (root == nullptr) {
      return std::nullopt;
    }
    return root->key;
  }

  void verifyCopyOf(const CheckedRedBlackTree& source) const {
    EXPECT_EQ(size(), source.size());
    verifyCopiedNode(source, source.root, root, nullptr);
  }

private:
  int verifyBlackHeight(const Node* node) const {
    // Отсутствующий потомок считается чёрным листом.
    if (node == nullptr) {
      return 1;
    }
    SCOPED_TRACE(testing::Message() << "node key=" << node->key);
    const bool black = isBlack(node->key);
    if (!black) {
      if (node->left != nullptr) {
        EXPECT_TRUE(isBlack(node->left->key));
      }
      if (node->right != nullptr) {
        EXPECT_TRUE(isBlack(node->right->key));
      }
    }

    const int leftHeight = verifyBlackHeight(node->left);
    const int rightHeight = verifyBlackHeight(node->right);
    EXPECT_EQ(leftHeight, rightHeight);
    return leftHeight + (black ? 1 : 0);
  }

  void verifyCopiedNode(const CheckedRedBlackTree& source, const Node* original,
                        const Node* copy, const Node* parent) const {
    if (original == nullptr) {
      EXPECT_EQ(copy, nullptr);
      return;
    }
    ASSERT_NE(copy, nullptr);
    ASSERT_NE(copy, original);
    ASSERT_EQ(copy->parent, parent);
    ASSERT_EQ(copy->key, original->key);
    EXPECT_EQ(copy->data, original->data);
    EXPECT_EQ(isRed(copy->key), source.isRed(original->key));
    verifyCopiedNode(source, original->left, copy->left, copy);
    verifyCopiedNode(source, original->right, copy->right, copy);
  }
};

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
    CheckedRedBlackTree tree;
    std::map<int, int> expected;
    for (int key : order) {
      tree.insert(key, key * 10);
      expected.emplace(key, key * 10);
      ASSERT_NO_FATAL_FAILURE(tree.verify(expected));
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
      CheckedRedBlackTree tree;
      std::map<int, int> expected;
      for (int key : order) {
        tree.insert(sign * key, key * 10);
        expected.emplace(sign * key, key * 10);
      }
      ASSERT_TRUE(tree.isBlack(sign));
      tree.remove(sign);
      expected.erase(sign);
      ASSERT_NO_FATAL_FAILURE(tree.verify(expected));
    }
  }
}

TEST(RedBlackTreeTests, PropagatesBlackHeightLossToRootOnBothSides) {
  for (int sign : {1, -1}) {
    SCOPED_TRACE(sign);
    CheckedRedBlackTree tree;
    std::map<int, int> expected;
    for (int key = 1; key <= 10; ++key) {
      tree.insert(sign * key, key * 10);
      expected.emplace(sign * key, key * 10);
    }
    for (int key : {8, 9}) {
      tree.remove(sign * key);
      expected.erase(sign * key);
    }
    ASSERT_NO_FATAL_FAILURE(tree.verify(expected));
    ASSERT_EQ(tree.rootKey(), sign * 4);
    for (int key : {1, 2, 3, 6}) ASSERT_TRUE(tree.isBlack(sign * key));

    tree.remove(sign);
    expected.erase(sign);
    ASSERT_NO_FATAL_FAILURE(tree.verify(expected));
    EXPECT_EQ(tree.rootKey(), sign * 4);
    EXPECT_TRUE(tree.isRed(sign * 3));
    EXPECT_TRUE(tree.isRed(sign * 6));
  }
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
  EXPECT_FALSE(map.containsKey(10));
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

class RedBlackTreeSafetyTests : public testing::Test {
protected:
  void TearDown() override {
    Faulty::comparisonsLeft = -1;
    Faulty::copiesLeft = -1;
    EXPECT_EQ(Faulty::alive, 0);
  }
};

TEST_F(RedBlackTreeSafetyTests, LookupAndReplacePreserveContracts) {
  RedBlackTree<int, int> tree;
  const auto& readOnly = tree;
  EXPECT_FALSE(tree.containsKey(1));
  EXPECT_THROW(tree.at(1), std::out_of_range);
  EXPECT_THROW(readOnly.at(1), std::out_of_range);
  EXPECT_TRUE(throwsExactly<std::logic_error>([&] { tree.replace(1, 10); }));
  for (int key : {4, 2, 6, 1, 3, 5, 7}) tree.insert(key, key * 10);
  for (int key : {4, 2, 6, 1, 3, 5, 7}) {
    ASSERT_TRUE(readOnly.containsKey(key));
    EXPECT_EQ(readOnly.at(key), key * 10);
    tree.replace(key, -key);
    EXPECT_EQ(readOnly.at(key), -key);
    tree.at(key) = key;
    EXPECT_EQ(readOnly.at(key), key);
  }
  EXPECT_EQ(tree.size(), 7);
  EXPECT_THROW(tree.at(8), std::out_of_range);
  EXPECT_THROW(readOnly.at(8), std::out_of_range);
  EXPECT_TRUE(throwsExactly<std::out_of_range>([&] { tree.replace(8, 80); }));
  EXPECT_EQ(tree.size(), 7);
}

TEST_F(RedBlackTreeSafetyTests, RootAndExtremaReturnValuesAndThrowOnEmptyTree) {
  RedBlackTree<int, int> tree;
  EXPECT_TRUE(throwsExactly<std::logic_error>([&] { (void)tree.get_root(); }));
  EXPECT_TRUE(throwsExactly<std::logic_error>([&] { (void)tree.min(); }));
  EXPECT_TRUE(throwsExactly<std::logic_error>([&] { (void)tree.max(); }));

  tree.insert(4, 40);
  tree.insert(2, 90);
  tree.insert(6, 10);
  EXPECT_EQ(tree.get_root(), 40);
  EXPECT_EQ(tree.min(), 90);
  EXPECT_EQ(tree.max(), 10);

  tree.remove(2);
  EXPECT_EQ(tree.min(), 40);
  tree.remove(6);
  EXPECT_EQ(tree.max(), 40);
}

TEST_F(RedBlackTreeSafetyTests, RangeHandlesEmptyResultsEqualBoundsAndReversedBounds) {
  RedBlackTree<int, int> tree;
  EXPECT_EQ(tree.range(2, 6).size(), 0);
  EXPECT_TRUE(throwsExactly<std::invalid_argument>([&] { (void)tree.range(6, 2); }));

  tree.insert(4, 40);
  tree.insert(2, 20);
  tree.insert(6, 60);
  EXPECT_EQ(tree.range(7, 9).size(), 0);
  EXPECT_EQ(tree.range(3, 3).size(), 0);
  const auto single = tree.range(4, 4);
  ASSERT_EQ(single.size(), 1);
  EXPECT_EQ(single[0], (std::pair{4, 40}));
  EXPECT_TRUE(throwsExactly<std::invalid_argument>([&] { (void)tree.range(6, 2); }));
}

TEST_F(RedBlackTreeSafetyTests, PrintWritesSortedKeysAndSkipsEmptyTree) {
  RedBlackTree<int, int> tree;
  std::ostringstream output;
  {
    CoutRedirect redirect(output);
    tree.print();
  }
  EXPECT_TRUE(output.str().empty());
  for (int key : {4, -2, 6, 0}) tree.insert(key, key * 10);
  {
    CoutRedirect redirect(output);
    tree.print();
  }
  EXPECT_EQ(output.str(), "-2 0 4 6 ");
}

TEST_F(RedBlackTreeSafetyTests, PrintReleasesTraversalStackWhenOutputThrows) {
  RedBlackTree<UnprintableKey, int> tree;
  for (int key : {4, 2, 6}) tree.insert({key}, key * 10);

  std::ptrdiff_t balance = 0;
  EXPECT_TRUE(throwsExactly<PrintFailure>([&] {
    test::TrackAllocations track(balance);
    tree.print();
  }));
  EXPECT_EQ(balance, 0);
  EXPECT_EQ(tree.size(), 3);
  EXPECT_EQ(tree.at({2}), 20);
}

TEST_F(RedBlackTreeSafetyTests, RemovesTwoChildNodesWithoutAssigningKeys) {
  tree_test::verifyRemovalPreservesRemainingEntries<RedBlackTree<Faulty, int>, Faulty, int>();
}

TEST_F(RedBlackTreeSafetyTests, RemovesTwoChildNodesWithoutAssigningValues) {
  tree_test::verifyRemovalPreservesRemainingEntries<RedBlackTree<int, Faulty>, int, Faulty>();
}

TEST_F(RedBlackTreeSafetyTests, ComparisonFailuresDuringInsertLeaveNoLeaksOrChanges) {
  // Меняем число сравнений, которые успевают выполниться до исключения.
  for (int budget = 0; budget < 8; ++budget) {
    SCOPED_TRACE(budget);
    RedBlackTree<Faulty, Faulty> tree;
    tree.insert(Faulty(2), Faulty(20));
    const int aliveBefore = Faulty::alive;
    Faulty::comparisonsLeft = budget;
    bool failed = false;
    try {
      tree.insert(Faulty(1), Faulty(10));
    } catch (const ComparisonFailure&) {
      failed = true;
    }
    Faulty::comparisonsLeft = -1;
    if (budget == 0) {
      EXPECT_TRUE(failed);
    }
    EXPECT_EQ(tree.size(), failed ? 1 : 2);
    EXPECT_EQ(Faulty::alive, aliveBefore + (failed ? 0 : 2));
    EXPECT_EQ(tree.at(Faulty(2)).value, 20);
    EXPECT_EQ(tree.containsKey(Faulty(1)), !failed);
    if (!failed) EXPECT_EQ(tree.at(Faulty(1)).value, 10);
  }
}

TEST_F(RedBlackTreeSafetyTests, CopyFailuresDuringInsertLeaveTreeUnchanged) {
  for (bool initiallyEmpty : {true, false}) {
    SCOPED_TRACE(testing::Message() << "initially empty=" << initiallyEmpty);
    for (int budget = 0; budget < 12; ++budget) {
      SCOPED_TRACE(budget);
      RedBlackTree<Faulty, Faulty> tree;
      if (!initiallyEmpty) tree.insert(Faulty(2), Faulty(20));
      const int aliveBefore = Faulty::alive;
      Faulty::copiesLeft = budget;
      bool failed = false;
      try {
        tree.insert(Faulty(1), Faulty(10));
      } catch (const CopyFailure&) {
        failed = true;
      }
      Faulty::copiesLeft = -1;
      if (budget == 0) {
        EXPECT_TRUE(failed);
      }
      EXPECT_EQ(tree.size(), (initiallyEmpty ? 0 : 1) + (failed ? 0 : 1));
      EXPECT_EQ(Faulty::alive, aliveBefore + (failed ? 0 : 2));
      EXPECT_EQ(tree.containsKey(Faulty(1)), !failed);
      if (!initiallyEmpty) EXPECT_EQ(tree.at(Faulty(2)).value, 20);
    }
  }
}

TEST_F(RedBlackTreeSafetyTests, ComparisonFailuresDuringRemoveLeaveTreeUnchanged) {
  for (int budget = 0; budget < 8; ++budget) {
    SCOPED_TRACE(budget);
    RedBlackTree<Faulty, Faulty> tree;
    for (int key : {2, 1, 3}) tree.insert(Faulty(key), Faulty(key * 10));
    Faulty::comparisonsLeft = budget;
    bool failed = false;
    try {
      tree.remove(Faulty(1));
    } catch (const ComparisonFailure&) {
      failed = true;
    }
    Faulty::comparisonsLeft = -1;
    if (budget == 0) {
      EXPECT_TRUE(failed);
    }
    EXPECT_EQ(tree.size(), failed ? 3 : 2);
    EXPECT_EQ(tree.containsKey(Faulty(1)), failed);
    EXPECT_EQ(tree.at(Faulty(2)).value, 20);
    EXPECT_EQ(tree.at(Faulty(3)).value, 30);
  }
}

TEST_F(RedBlackTreeSafetyTests, DestructionAndMoveAssignmentReleaseAllValues) {
  using Tree = RedBlackTree<int, Faulty>;
  {
    Tree tree;
    for (int key = 0; key < 2048; ++key) {
      tree.insert(key, Faulty(key));
    }
  }
  EXPECT_EQ(Faulty::alive, 0);

  Tree source;
  Tree target;
  for (int key : {4, 2, 6, 1, 3, 5, 7}) target.insert(key, Faulty(key));
  source.insert(10, Faulty(100));
  target = std::move(source);
  EXPECT_EQ(Faulty::alive, 1);
  EXPECT_EQ(source.size(), 0);
  EXPECT_EQ(target.size(), 1);
  EXPECT_EQ(target.at(10).value, 100);
}

TEST_F(RedBlackTreeSafetyTests, DestructionAndMoveAssignmentDoNotAllocate) {
  using Tree = RedBlackTree<int, Faulty>;
  auto target = std::make_unique<Tree>();
  Tree source;
  for (int key : {4, 2, 6, 1, 3, 5, 7}) target->insert(key, Faulty(key));
  source.insert(10, Faulty(100));

  {
    test::RejectAllocations guard;
    *target = std::move(source);
  }
  EXPECT_EQ(Faulty::alive, 1);
  EXPECT_EQ(target->size(), 1u);
  EXPECT_EQ(source.size(), 0u);

  target->insert(5, Faulty(50));
  target->insert(15, Faulty(150));
  ASSERT_EQ(target->size(), 3u);
  {
    test::RejectAllocations guard;
    target.reset();
  }
  EXPECT_EQ(Faulty::alive, 0);
}

TEST_F(RedBlackTreeSafetyTests, AllocationFailureDuringInsertPreservesTree) {
  for (bool initiallyEmpty : {true, false}) {
    SCOPED_TRACE(testing::Message() << "initially empty=" << initiallyEmpty);
    RedBlackTree<int, int> tree;
    if (!initiallyEmpty) {
      tree.insert(2, 20);
    }

    bool failed = false;
    {
      test::RejectAllocations guard;
      try {
        tree.insert(1, 10);
      } catch (const std::bad_alloc&) {
        failed = true;
      }
    }
    EXPECT_TRUE(failed);
    EXPECT_EQ(tree.size(), initiallyEmpty ? 0 : 1);
    EXPECT_FALSE(tree.containsKey(1));
    if (!initiallyEmpty) {
      EXPECT_EQ(tree.at(2), 20);
    }
  }
}

TEST_F(RedBlackTreeSafetyTests, CopyFailureReleasesPartialCopyAndPreservesAssignmentTarget) {
  using Tree = RedBlackTree<int, Faulty>;
  Tree source;
  Tree target;
  for (int key : {4, 2, 8, 3, 6, 9, 7}) source.insert(key, Faulty(key));
  target.insert(1, Faulty(10));
  const int aliveBefore = Faulty::alive;

  Faulty::copiesLeft = 5;
  EXPECT_THROW((void)Tree(source), CopyFailure);
  EXPECT_EQ(Faulty::alive, aliveBefore);

  Faulty::copiesLeft = 5;
  EXPECT_THROW(target = source, CopyFailure);
  Faulty::copiesLeft = -1;
  EXPECT_EQ(Faulty::alive, aliveBefore);
  EXPECT_EQ(target.size(), 1);
  EXPECT_EQ(target.at(1).value, 10);
  EXPECT_EQ(source.size(), 7);
  EXPECT_EQ(source.at(7).value, 7);
}

TEST_F(RedBlackTreeSafetyTests, AllocationFailureReleasesPartialCopyAndPreservesBothTrees) {
  using Tree = CheckedRedBlackTree;
  Tree source;
  const std::map<int, int> expected{{2, 20}, {3, 30}, {4, 40}, {6, 60},
                                    {7, 70}, {8, 80}, {9, 90}};
  for (int key : {4, 2, 8, 3, 6, 9, 7}) source.insert(key, expected.at(key));
  Tree target;
  target.insert(1, 10);
  target.insert(5, 50);

  for (std::ptrdiff_t allowed = 0; allowed < std::ssize(expected); ++allowed) {
    SCOPED_TRACE(allowed);
    std::ptrdiff_t balance = 0;
    EXPECT_TRUE(throwsExactly<std::bad_alloc>([&] {
      test::TrackAllocations track(balance);
      test::RejectAllocations reject(allowed);
      (void)Tree(source);
    }));
    EXPECT_EQ(balance, 0);
    source.verify(expected);

    balance = 0;
    EXPECT_TRUE(throwsExactly<std::bad_alloc>([&] {
      test::TrackAllocations track(balance);
      test::RejectAllocations reject(allowed);
      target = source;
    }));
    EXPECT_EQ(balance, 0);
    source.verify(expected);
    target.verify({{1, 10}, {5, 50}});
  }
}

TEST_F(RedBlackTreeSafetyTests, KeyCopyFailureReleasesPartialCopyAndPreservesBothTrees) {
  using Tree = RedBlackTree<Faulty, int>;
  const std::vector<int> keys{4, 2, 8, 3, 6, 9, 7};
  Tree source;
  for (int key : keys) source.insert(Faulty(key), key * 10);
  Tree target;
  target.insert(Faulty(1), 10);
  target.insert(Faulty(5), 50);
  const int aliveBefore = Faulty::alive;

  Faulty::copiesLeft = std::numeric_limits<int>::max();
  (void)Tree(source);
  const int keyCopies = std::numeric_limits<int>::max() - Faulty::copiesLeft;
  Faulty::copiesLeft = -1;
  ASSERT_GT(keyCopies, 0);

  for (int allowed = 0; allowed < keyCopies; ++allowed) {
    SCOPED_TRACE(allowed);
    const auto verifyFailure = [&](auto operation) {
      std::ptrdiff_t balance = 0;
      bool failed = false;
      Faulty::copiesLeft = allowed;
      {
        test::TrackAllocations track(balance);
        try {
          operation();
        } catch (const CopyFailure& error) {
          failed = typeid(error) == typeid(CopyFailure);
        }
      }
      Faulty::copiesLeft = -1;
      EXPECT_TRUE(failed);
      EXPECT_EQ(balance, 0);
      EXPECT_EQ(Faulty::alive, aliveBefore);
      EXPECT_EQ(source.size(), keys.size());
      for (int key : keys) EXPECT_EQ(source.at(Faulty(key)), key * 10);
      EXPECT_EQ(target.size(), 2);
      EXPECT_EQ(target.at(Faulty(1)), 10);
      EXPECT_EQ(target.at(Faulty(5)), 50);
    };
    verifyFailure([&] { (void)Tree(source); });
    verifyFailure([&] { target = source; });
  }
}

TEST_F(RedBlackTreeSafetyTests, CopiesPreserveStructureAndNodeProperties) {
  using Tree = CheckedRedBlackTree;
  Tree source;
  std::map<int, int> expected;
  for (int key : {10, 20, 30, 15, 25, 5, 1, 6}) {
    source.insert(key, key * 10);
    expected.emplace(key, key * 10);
  }
  source.remove(30);
  expected.erase(30);

  Tree constructed(source);
  Tree assigned;
  assigned.insert(99, 990);
  assigned = source;
  const Tree& self = assigned;
  assigned = self;
  constructed.verifyCopyOf(source);
  assigned.verifyCopyOf(source);

  assigned.remove(20);
  assigned.at(15) = 8;
  source.verify(expected);
  expected.erase(20);
  expected.at(15) = 8;
  assigned.verify(expected);

  const Tree empty;
  const Tree emptyCopy(empty);
  emptyCopy.verifyCopyOf(empty);
  assigned = empty;
  assigned.verifyCopyOf(empty);
}

TEST_F(RedBlackTreeSafetyTests, MoveConstructionAndAssignmentPreserveInvariants) {
  using Tree = CheckedRedBlackTree;
  Tree source;
  std::map<int, int> expected;
  for (int key : {10, 20, 30, 15, 25, 5, 1, 6}) {
    source.insert(key, key * 10);
    expected.emplace(key, key * 10);
  }

  Tree constructed(std::move(source));
  source.verify({});
  constructed.verify(expected);
  source.insert(42, 420);
  source.verify({{42, 420}});

  Tree assigned;
  assigned.insert(99, 990);
  assigned = std::move(constructed);
  constructed.verify({});
  assigned.verify(expected);
  assigned.remove(20);
  expected.erase(20);
  assigned.verify(expected);
}

TEST_F(RedBlackTreeSafetyTests, PreservesInvariantsAcrossInsertionOrdersAndRootRemoval) {
  std::vector<int> order{1, 2, 3, 4, 5};
  do {
    SCOPED_TRACE(testing::PrintToString(order));
    CheckedRedBlackTree tree;
    std::map<int, int> expected;
    for (int key : order) {
      tree.insert(key, key * 10);
      expected.emplace(key, key * 10);
      ASSERT_NO_FATAL_FAILURE(tree.verify(expected));
    }
    while (!expected.empty()) {
      const auto key = tree.rootKey();
      ASSERT_TRUE(key.has_value());
      tree.remove(*key);
      expected.erase(*key);
      ASSERT_NO_FATAL_FAILURE(tree.verify(expected));
    }
    EXPECT_TRUE(throwsExactly<std::logic_error>([&] { tree.remove(0); }));
  } while (std::next_permutation(order.begin(), order.end()));
}

TEST_F(RedBlackTreeSafetyTests, MixedOperationsMatchStdMapAndPreserveInvariants) {
  tree_test::verifyMixedOperationsAgainstStdMap<CheckedRedBlackTree>();
}

} // namespace
