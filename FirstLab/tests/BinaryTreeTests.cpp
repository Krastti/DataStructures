#include "BinaryTree.h"
#include "RedBlackTree.h"
#include "../../DataStructures/tests/Array/ArrayTestHelpers.h"
#include "../../DataStructures/tests/AllocationFailure.h"
#include <gtest/gtest.h>

#include <algorithm>
#include <compare>
#include <cstddef>
#include <exception>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <new>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <vector>

namespace {

TEST(BinaryTreeTests, InsertsKeyAndValue) {
  BinaryTree<int, int> tree;

  tree.insert(4, 40);

  EXPECT_EQ(tree.size(), 1);
  EXPECT_EQ(tree.at(4), 40);
}

TEST(BinaryTreeTests, FindsValueByKey) {
  BinaryTree<int, int> tree;
  tree.insert(4, 40);
  tree.insert(2, 20);

  EXPECT_EQ(tree.at(2), 20);
}

TEST(BinaryTreeTests, RemovesKey) {
  BinaryTree<int, int> tree;
  tree.insert(4, 40);
  tree.insert(2, 20);

  tree.remove(2);

  EXPECT_FALSE(tree.containsKey(2));
  EXPECT_TRUE(tree.containsKey(4));
  EXPECT_EQ(tree.size(), 1);
}

template<class Exception, class Operation>
testing::AssertionResult throwsExactly(Operation operation) {
  try {
    operation();
  } catch (const Exception& error) {
    if (typeid(error) == typeid(Exception)) return testing::AssertionSuccess();
    return testing::AssertionFailure() << "Expected " << typeid(Exception).name()
                                       << ", caught " << typeid(error).name();
  } catch (const std::exception& error) {
    return testing::AssertionFailure() << "Expected " << typeid(Exception).name()
                                       << ", caught " << typeid(error).name();
  } catch (...) {
    return testing::AssertionFailure() << "Expected " << typeid(Exception).name()
                                       << ", caught a non-standard exception";
  }
  return testing::AssertionFailure() << "Expected " << typeid(Exception).name()
                                     << ", no exception thrown";
}

class CheckedBinaryTree : public BinaryTree<int, int> {
public:
  void verify(const std::map<int, int>& expected) const {
    std::size_t count = 0;
    verifyNode(root, nullptr, std::numeric_limits<long long>::min(),
               std::numeric_limits<long long>::max(), count);
    EXPECT_EQ(count, size());
    ASSERT_EQ(count, expected.size());
    const auto actual = entries();
    ASSERT_EQ(actual.size(), expected.size());
    std::size_t index = 0;
    for (const auto& [key, value] : expected) {
      EXPECT_EQ(actual[index].first, key);
      EXPECT_EQ(actual[index].second, value);
      ++index;
    }
  }

  int rootKey() const { return root->key; }

  void verifyCopyOf(const CheckedBinaryTree& source) const {
    EXPECT_EQ(size(), source.size());
    verifyCopiedNode(source.root, root, nullptr);
  }

private:
  void verifyNode(const Node* node, const Node* parent, long long low, long long high,
                  std::size_t& count) const {
    if (node == nullptr) return;
    EXPECT_EQ(node->parent, parent);
    EXPECT_GT(node->key, low);
    EXPECT_LT(node->key, high);
    ++count;
    verifyNode(node->left, node, low, node->key, count);
    verifyNode(node->right, node, node->key, high, count);
  }

  void verifyCopiedNode(const Node* original, const Node* copy,
                        const Node* parent) const {
    if (original == nullptr) {
      EXPECT_EQ(copy, nullptr);
      return;
    }
    ASSERT_NE(copy, nullptr);
    EXPECT_NE(copy, original);
    EXPECT_EQ(copy->parent, parent);
    EXPECT_EQ(copy->key, original->key);
    EXPECT_EQ(copy->data, original->data);
    verifyCopiedNode(original->left, copy->left, copy);
    verifyCopiedNode(original->right, copy->right, copy);
  }
};

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

TEST(BinaryTreeTests, CopiesOwnTheirNodesAndReplacePreviousContents) {
  BinaryTree<int, int> original;
  for (int key : {4, 2, 8, 3, 6, 9, 7}) original.insert(key, key * 10);

  BinaryTree<int, int> constructed(original);
  BinaryTree<int, int> assigned(1, 10);
  assigned = original;

  for (auto* copy : {&constructed, &assigned}) {
    EXPECT_EQ(copy->size(), original.size());
    EXPECT_EQ(copy->get_root(), original.get_root());
    EXPECT_EQ(toVector(copy->entries()), toVector(original.entries()));
    EXPECT_NE(&copy->at(7), &original.at(7));
    copy->at(7) = 700;
    copy->remove(4);
    EXPECT_EQ(copy->size(), 6);
    EXPECT_EQ(copy->get_root(), 60);
    EXPECT_EQ(copy->at(7), 700);
  }
  EXPECT_EQ(original.size(), 7);
  EXPECT_EQ(original.at(4), 40);
  EXPECT_EQ(original.at(7), 70);

  const auto& self = assigned;
  assigned = self;
  EXPECT_EQ(toVector(assigned.entries()), toVector(constructed.entries()));

  const BinaryTree<int, int> empty;
  const BinaryTree<int, int> emptyCopy(empty);
  EXPECT_EQ(emptyCopy.size(), 0);
  assigned = empty;
  EXPECT_EQ(assigned.size(), 0);
  EXPECT_FALSE(assigned.containsKey(7));
}

struct Faulty {
  inline static int alive = 0;
  inline static int comparisonsLeft = -1;
  inline static int copiesLeft = -1;
  int value;

  Faulty(int value = 0) : value(value) { ++alive; }
  Faulty(const Faulty& other) : value(other.value) {
    if (copiesLeft == 0) throw std::runtime_error("copy failed");
    if (copiesLeft > 0) --copiesLeft;
    ++alive;
  }
  ~Faulty() { --alive; }
  Faulty& operator=(const Faulty&) { throw std::runtime_error("assignment failed"); }

  static void compare() {
    if (comparisonsLeft == 0) throw std::runtime_error("comparison failed");
    if (comparisonsLeft > 0) --comparisonsLeft;
  }
  friend bool operator<(const Faulty& a, const Faulty& b) {
    compare();
    return a.value < b.value;
  }
  friend bool operator>(const Faulty& a, const Faulty& b) {
    compare();
    return a.value > b.value;
  }
  friend bool operator!=(const Faulty& a, const Faulty& b) {
    compare();
    return a.value != b.value;
  }
  friend bool operator==(const Faulty& a, const Faulty& b) {
    compare();
    return a.value == b.value;
  }
};

struct PrintFailure : std::exception {};

struct UnprintableKey {
  int value;

  auto operator<=>(const UnprintableKey&) const = default;

  friend std::ostream& operator<<(std::ostream&, const UnprintableKey&) {
    throw PrintFailure{};
  }
};

struct CoutRedirect {
  explicit CoutRedirect(std::ostream& output) : previous(std::cout.rdbuf(output.rdbuf())) {}
  ~CoutRedirect() { std::cout.rdbuf(previous); }

  CoutRedirect(const CoutRedirect&) = delete;
  CoutRedirect& operator=(const CoutRedirect&) = delete;

private:
  std::streambuf* previous;
};

class BinaryTreeSafetyTests : public testing::Test {
protected:
  void TearDown() override {
    Faulty::comparisonsLeft = -1;
    Faulty::copiesLeft = -1;
    EXPECT_EQ(Faulty::alive, 0);
  }
};

TEST_F(BinaryTreeSafetyTests, LookupAndReplacePreserveContracts) {
  BinaryTree<int, int> tree;
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

TEST_F(BinaryTreeSafetyTests, RootAndExtremaReturnValuesAndThrowOnEmptyTree) {
  BinaryTree<int, int> tree;
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

TEST_F(BinaryTreeSafetyTests, RangeHandlesEmptyResultsEqualBoundsAndReversedBounds) {
  BinaryTree<int, int> tree;
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

TEST_F(BinaryTreeSafetyTests, PrintWritesSortedKeysAndSkipsEmptyTree) {
  BinaryTree<int, int> tree;
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

TEST_F(BinaryTreeSafetyTests, PrintReleasesTraversalStackWhenOutputThrows) {
  BinaryTree<UnprintableKey, int> tree;
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

TEST_F(BinaryTreeSafetyTests, RemovesTwoChildNodesWithoutAssigningKeys) {
  // Проверяем непосредственного преемника, преемника в глубине поддерева и преемника с правым ребёнком.
  for (const std::vector<int>& keys : {
         std::vector<int>{2, 1, 3}, {4, 2, 8, 1, 3, 6, 9, 7}}) {
    BinaryTree<Faulty, int> tree;
    for (int key : keys) tree.insert(Faulty(key), key * 10);
    for (int key : keys) {
      EXPECT_NO_THROW(tree.remove(Faulty(key)));
      EXPECT_FALSE(tree.containsKey(Faulty(key)));
      for (int remaining : keys) {
        if (tree.containsKey(Faulty(remaining))) {
          EXPECT_EQ(tree.at(Faulty(remaining)), remaining * 10);
        }
      }
    }
    EXPECT_EQ(tree.size(), 0);
  }
}

TEST_F(BinaryTreeSafetyTests, RemovesTwoChildNodesWithoutAssigningValues) {
  for (const std::vector<int>& keys : {
         std::vector<int>{2, 1, 3}, {4, 2, 8, 1, 3, 6, 9, 7}}) {
    BinaryTree<int, Faulty> tree;
    for (int key : keys) tree.insert(key, Faulty(key * 10));
    for (int key : keys) {
      EXPECT_NO_THROW(tree.remove(key));
      EXPECT_FALSE(tree.containsKey(key));
      for (int remaining : keys) {
        if (tree.containsKey(remaining)) {
          EXPECT_EQ(tree.at(remaining).value, remaining * 10);
        }
      }
    }
    EXPECT_EQ(tree.size(), 0);
  }
}

TEST_F(BinaryTreeSafetyTests, ComparisonFailuresDuringInsertLeaveNoLeaksOrChanges) {
  // Проверяем исключение на каждом сравнении, включая сравнение после выделения узла в прежней реализации.
  for (int budget = 0; budget < 8; ++budget) {
    BinaryTree<Faulty, Faulty> tree;
    tree.insert(Faulty(2), Faulty(20));
    int before = Faulty::alive;
    Faulty::comparisonsLeft = budget;
    bool failed = false;
    try { tree.insert(Faulty(1), Faulty(10)); }
    catch (const std::runtime_error&) { failed = true; }
    Faulty::comparisonsLeft = -1;
    EXPECT_EQ(tree.size(), failed ? 1 : 2);
    EXPECT_EQ(Faulty::alive, before + (failed ? 0 : 2));
    EXPECT_EQ(tree.at(Faulty(2)).value, 20);
    EXPECT_EQ(tree.containsKey(Faulty(1)), !failed);
    if (!failed) EXPECT_EQ(tree.at(Faulty(1)).value, 10);
  }
}

TEST_F(BinaryTreeSafetyTests, CopyFailuresDuringInsertLeaveTreeUnchanged) {
  for (bool initiallyEmpty : {true, false}) {
    for (int budget = 0; budget < 12; ++budget) {
      BinaryTree<Faulty, Faulty> tree;
      if (!initiallyEmpty) tree.insert(Faulty(2), Faulty(20));
      int before = Faulty::alive;
      Faulty::copiesLeft = budget;
      bool failed = false;
      try { tree.insert(Faulty(1), Faulty(10)); }
      catch (const std::runtime_error&) { failed = true; }
      Faulty::copiesLeft = -1;
      EXPECT_EQ(tree.size(), (initiallyEmpty ? 0 : 1) + (failed ? 0 : 1));
      EXPECT_EQ(Faulty::alive, before + (failed ? 0 : 2));
      EXPECT_EQ(tree.containsKey(Faulty(1)), !failed);
      if (!initiallyEmpty) EXPECT_EQ(tree.at(Faulty(2)).value, 20);
    }
  }
}

TEST_F(BinaryTreeSafetyTests, ComparisonFailuresDuringRemoveLeaveTreeUnchanged) {
  for (int budget = 0; budget < 8; ++budget) {
    BinaryTree<Faulty, Faulty> tree;
    for (int key : {2, 1, 3}) tree.insert(Faulty(key), Faulty(key * 10));
    Faulty::comparisonsLeft = budget;
    bool failed = false;
    try { tree.remove(Faulty(1)); }
    catch (const std::runtime_error&) { failed = true; }
    Faulty::comparisonsLeft = -1;
    EXPECT_EQ(tree.size(), failed ? 3 : 2);
    EXPECT_EQ(tree.containsKey(Faulty(1)), failed);
    EXPECT_EQ(tree.at(Faulty(2)).value, 20);
    EXPECT_EQ(tree.at(Faulty(3)).value, 30);
  }
}

TEST_F(BinaryTreeSafetyTests, DestructionAndMoveAssignmentReleaseAllValues) {
  using Tree = BinaryTree<int, Faulty>;
  // Длинная цепочка в обычном BST дополнительно проверяет очистку без рекурсии.
  auto* tree = new Tree;
  for (int key = 0; key < 2048; ++key) tree->insert(key, Faulty(key));
  delete tree;
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

TEST_F(BinaryTreeSafetyTests, DestructionAndMoveAssignmentDoNotAllocate) {
  using Tree = BinaryTree<int, Faulty>;
  auto* target = new Tree;
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
  {
    test::RejectAllocations guard;
    delete target;
  }
  EXPECT_EQ(Faulty::alive, 0);
}

TEST_F(BinaryTreeSafetyTests, AllocationFailureDuringInsertPreservesTree) {
  BinaryTree<int, int> tree;
  for (bool initiallyEmpty : {true, false}) {
    bool failed = false;
    {
      test::RejectAllocations guard;
      try { tree.insert(1, 10); }
      catch (const std::bad_alloc&) { failed = true; }
    }
    EXPECT_TRUE(failed);
    EXPECT_EQ(tree.size(), initiallyEmpty ? 0 : 1);
    EXPECT_FALSE(tree.containsKey(1));
    if (initiallyEmpty) tree.insert(2, 20);
    else EXPECT_EQ(tree.at(2), 20);
  }
}

TEST_F(BinaryTreeSafetyTests, CopyFailureReleasesPartialCopyAndPreservesAssignmentTarget) {
  using Tree = BinaryTree<int, Faulty>;
  Tree source;
  Tree target;
  for (int key : {4, 2, 8, 3, 6, 9, 7}) source.insert(key, Faulty(key));
  target.insert(1, Faulty(10));
  const int before = Faulty::alive;

  Faulty::copiesLeft = 5;
  EXPECT_THROW((void)Tree(source), std::runtime_error);
  EXPECT_EQ(Faulty::alive, before);

  Faulty::copiesLeft = 5;
  EXPECT_THROW(target = source, std::runtime_error);
  Faulty::copiesLeft = -1;
  EXPECT_EQ(Faulty::alive, before);
  EXPECT_EQ(target.size(), 1);
  EXPECT_EQ(target.at(1).value, 10);
  EXPECT_EQ(source.size(), 7);
  EXPECT_EQ(source.at(7).value, 7);
}

TEST_F(BinaryTreeSafetyTests, AllocationFailureReleasesPartialCopyAndPreservesBothTrees) {
  using Tree = CheckedBinaryTree;
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

TEST_F(BinaryTreeSafetyTests, KeyCopyFailureReleasesPartialCopyAndPreservesBothTrees) {
  using Tree = BinaryTree<Faulty, int>;
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
        } catch (const std::runtime_error& error) {
          failed = typeid(error) == typeid(std::runtime_error);
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

TEST_F(BinaryTreeSafetyTests, CopiesPreserveStructureAndNodeProperties) {
  using Tree = CheckedBinaryTree;
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

TEST_F(BinaryTreeSafetyTests, MoveConstructionAndAssignmentPreserveInvariants) {
  using Tree = CheckedBinaryTree;
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

TEST_F(BinaryTreeSafetyTests, PreservesInvariantsAcrossInsertionOrdersAndRootRemoval) {
  std::vector<int> order{1, 2, 3, 4, 5};
  do {
    CheckedBinaryTree tree;
    std::map<int, int> expected;
    for (int key : order) {
      tree.insert(key, key * 10);
      expected.emplace(key, key * 10);
      tree.verify(expected);
    }
    while (!expected.empty()) {
      int key = tree.rootKey();
      tree.remove(key);
      expected.erase(key);
      tree.verify(expected);
    }
    EXPECT_TRUE(throwsExactly<std::logic_error>([&] { tree.remove(0); }));
  } while (std::next_permutation(order.begin(), order.end()));
}

TEST_F(BinaryTreeSafetyTests, MixedOperationsMatchStdMapAndPreserveInvariants) {
  CheckedBinaryTree tree;
  std::map<int, int> expected;
  std::mt19937 random(147);
  for (int step = 0; step < 20000; ++step) {
    const int key = static_cast<int>(random() % 128) - 64;
    const auto found = expected.find(key);
    switch (random() % 5) {
      case 0:
        tree.insert(key, step);
        expected.emplace(key, step);
        break;
      case 1:
        if (found != expected.end()) {
          tree.remove(key);
          expected.erase(found);
        } else if (expected.empty()) {
          EXPECT_TRUE(throwsExactly<std::logic_error>([&] { tree.remove(key); }));
        } else {
          EXPECT_TRUE(throwsExactly<std::out_of_range>([&] { tree.remove(key); }));
        }
        break;
      case 2:
        if (found != expected.end()) {
          tree.replace(key, -step);
          found->second = -step;
        } else if (expected.empty()) {
          EXPECT_TRUE(throwsExactly<std::logic_error>([&] { tree.replace(key, -step); }));
        } else {
          EXPECT_TRUE(throwsExactly<std::out_of_range>([&] { tree.replace(key, -step); }));
        }
        break;
      case 3:
        EXPECT_EQ(tree.containsKey(key), found != expected.end());
        if (found != expected.end()) {
          EXPECT_EQ(std::as_const(tree).at(key), found->second);
        } else {
          EXPECT_TRUE(throwsExactly<std::out_of_range>([&] { (void)std::as_const(tree).at(key); }));
        }
        break;
      default: {
        const int other = static_cast<int>(random() % 160) - 80;
        const auto [first, last] = std::minmax(key, other);
        const auto matches = tree.range(first, last);
        auto begin = expected.lower_bound(first);
        const auto end = expected.upper_bound(last);
        ASSERT_EQ(matches.size(), static_cast<std::size_t>(std::distance(begin, end)));
        for (std::size_t i = 0; begin != end; ++begin, ++i) {
          EXPECT_EQ(matches[i], (std::pair{begin->first, begin->second}));
        }
        break;
      }
    }
    if (!expected.empty()) {
      EXPECT_EQ(tree.min(), expected.begin()->second);
      EXPECT_EQ(tree.max(), expected.rbegin()->second);
    }
    tree.verify(expected);
  }
}

} // namespace
