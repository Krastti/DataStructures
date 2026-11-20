#include "BinaryTree.h"
#include "RedBlackTree.h"
#include "Set.h"
#include "lexer.h"
#include "parser.h"
#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <limits>
#include <map>
#include <new>
#include <random>
#include <stdexcept>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <vector>

namespace {
thread_local std::ptrdiff_t allocationBudget = -1;
thread_local std::ptrdiff_t* allocationBalance = nullptr;
}

// Отдельный исполняемый файл изолирует отказ выделения памяти от остальных тестов.
// Включаем отказ только на время проверяемой операции.
void* operator new(std::size_t size) {
  if (allocationBudget == 0) throw std::bad_alloc();
  if (void* memory = std::malloc(size == 0 ? 1 : size)) {
    if (allocationBudget > 0) --allocationBudget;
    if (allocationBalance) ++*allocationBalance;
    return memory;
  }
  throw std::bad_alloc();
}

void operator delete(void* memory) noexcept {
  if (memory && allocationBalance) --*allocationBalance;
  std::free(memory);
}
void operator delete(void* memory, std::size_t) noexcept { ::operator delete(memory); }
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete[](void* memory) noexcept { ::operator delete(memory); }
void operator delete[](void* memory, std::size_t) noexcept { ::operator delete(memory); }
void* operator new(std::size_t size, const std::nothrow_t&) noexcept {
  try { return ::operator new(size); }
  catch (...) { return nullptr; }
}
void* operator new[](std::size_t size, const std::nothrow_t&) noexcept {
  return ::operator new(size, std::nothrow);
}
void operator delete(void* memory, const std::nothrow_t&) noexcept {
  ::operator delete(memory);
}
void operator delete[](void* memory, const std::nothrow_t&) noexcept {
  ::operator delete(memory);
}

namespace {
struct RejectAllocations {
  explicit RejectAllocations(std::ptrdiff_t allowed = 0) : previousBudget(allocationBudget) {
    allocationBudget = allowed;
  }

  ~RejectAllocations() { allocationBudget = previousBudget; }

private:
  std::ptrdiff_t previousBudget;
};

struct TrackAllocations {
  explicit TrackAllocations(std::ptrdiff_t& balance) { allocationBalance = &balance; }
  ~TrackAllocations() { allocationBalance = nullptr; }
};

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

struct BinaryTag {
  template<class K, class V> using Tree = BinaryTree<K, V>;
  static constexpr bool balanced = false;
};
struct RedBlackTag {
  template<class K, class V> using Tree = RedBlackTree<K, V>;
  static constexpr bool balanced = true;
};

template<class Tag> class TreeSafetyTests : public testing::Test {
protected:
  void TearDown() override {
    Faulty::comparisonsLeft = -1;
    Faulty::copiesLeft = -1;
    EXPECT_EQ(Faulty::alive, 0);
  }
};
using TreeTypes = testing::Types<BinaryTag, RedBlackTag>;
TYPED_TEST_SUITE(TreeSafetyTests, TreeTypes);

template<class Tag> struct CheckedTree : Tag::template Tree<int, int> {
  using Node = typename BinaryTree<int, int>::Node;

  int verifyNode(Node* node, Node* parent, long long low, long long high,
                 std::size_t& count) const {
    if (node == nullptr) return 1;
    EXPECT_EQ(node->parent, parent);
    EXPECT_GT(node->key, low);
    EXPECT_LT(node->key, high);
    ++count;
    int leftHeight = verifyNode(node->left, node, low, node->key, count);
    int rightHeight = verifyNode(node->right, node, node->key, high, count);
    if constexpr (Tag::balanced) {
      EXPECT_EQ(leftHeight, rightHeight) << "key=" << node->key;
      if (this->isRed(node->key)) {
        if (node->left) EXPECT_TRUE(this->isBlack(node->left->key));
        if (node->right) EXPECT_TRUE(this->isBlack(node->right->key));
      }
      return leftHeight + (this->isBlack(node->key) ? 1 : 0);
    }
    return 0;
  }

  void verify(const std::map<int, int>& expected) const {
    std::size_t count = 0;
    verifyNode(this->root, nullptr, std::numeric_limits<long long>::min(),
               std::numeric_limits<long long>::max(), count);
    EXPECT_EQ(count, this->size());
    ASSERT_EQ(count, expected.size());
    if constexpr (Tag::balanced) {
      if (this->root) EXPECT_TRUE(this->isBlack(this->root->key));
    }
    auto entries = this->entries();
    ASSERT_EQ(entries.size(), expected.size());
    std::size_t index = 0;
    for (const auto& [key, value] : expected) {
      EXPECT_EQ(entries[index].first, key);
      EXPECT_EQ(entries[index].second, value);
      ++index;
    }
  }

  int rootKey() const { return this->root->key; }
};

TYPED_TEST(TreeSafetyTests, LookupAndReplacePreserveContracts) {
  typename TypeParam::template Tree<int, int> tree;
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

TYPED_TEST(TreeSafetyTests, RootAndExtremaReturnValuesAndThrowOnEmptyTree) {
  typename TypeParam::template Tree<int, int> tree;
  EXPECT_TRUE(throwsExactly<std::logic_error>([&] { (void)tree.get_root(); }));
  EXPECT_TRUE(throwsExactly<std::logic_error>([&] { (void)tree.min(); }));
  EXPECT_TRUE(throwsExactly<std::logic_error>([&] { (void)tree.max(); }));

  tree.insert(4, 40);
  tree.insert(2, 90);
  tree.insert(6, 10);
  EXPECT_EQ(tree.get_root(), 40);
  EXPECT_EQ(tree.min(), 90);
  EXPECT_EQ(tree.max(), 10);
}

TYPED_TEST(TreeSafetyTests, RangeHandlesEmptyResultsEqualBoundsAndReversedBounds) {
  typename TypeParam::template Tree<int, int> tree;
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

TEST(SetSafetyTests, InsertFailuresPreserveExistingKeys) {
  Set<Faulty> values;
  for (int key : {4, 2, 6}) values.insert(Faulty(key));
  const int before = Faulty::alive;
  const auto verifyUnchanged = [&] {
    EXPECT_EQ(Faulty::alive, before);
    for (int key : {4, 2, 6}) EXPECT_TRUE(values.contains(Faulty(key)));
    EXPECT_FALSE(values.contains(Faulty(3)));
  };

  EXPECT_TRUE(throwsExactly<std::bad_alloc>([&] {
    RejectAllocations guard;
    values.insert(Faulty(3));
  }));
  verifyUnchanged();

  Faulty::comparisonsLeft = 1;
  EXPECT_TRUE(throwsExactly<std::runtime_error>([&] { values.insert(Faulty(3)); }));
  Faulty::comparisonsLeft = -1;
  verifyUnchanged();

  Faulty::copiesLeft = 1;
  EXPECT_TRUE(throwsExactly<std::runtime_error>([&] { values.insert(Faulty(3)); }));
  Faulty::copiesLeft = -1;
  verifyUnchanged();

  values.insert(Faulty(3));
  EXPECT_TRUE(values.contains(Faulty(3)));
}

TEST(SetSafetyTests, RemovalDoesNotAssignKeysAndDestructionReleasesAllKeys) {
  {
    Set<Faulty> values;
    for (int key : {4, 2, 6, 1, 3, 5, 7}) values.insert(Faulty(key));
    for (int key : {4, 2, 6, 1, 3, 5, 7}) {
      EXPECT_NO_THROW(values.remove(Faulty(key)));
      EXPECT_FALSE(values.contains(Faulty(key)));
    }
  }
  EXPECT_EQ(Faulty::alive, 0);
  auto* values = new Set<Faulty>;
  for (int key = 0; key < 1024; ++key) values->insert(Faulty(key));
  delete values;
  EXPECT_EQ(Faulty::alive, 0);
}

TYPED_TEST(TreeSafetyTests, RemovesTwoChildNodesWithoutAssigningKeys) {
  // Проверяем непосредственного преемника, преемника в глубине поддерева и преемника с правым ребёнком.
  for (const std::vector<int>& keys : {
         std::vector<int>{2, 1, 3}, {4, 2, 8, 1, 3, 6, 9, 7}}) {
    typename TypeParam::template Tree<Faulty, int> tree;
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

TYPED_TEST(TreeSafetyTests, RemovesTwoChildNodesWithoutAssigningValues) {
  for (const std::vector<int>& keys : {
         std::vector<int>{2, 1, 3}, {4, 2, 8, 1, 3, 6, 9, 7}}) {
    typename TypeParam::template Tree<int, Faulty> tree;
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

TYPED_TEST(TreeSafetyTests, ComparisonFailuresDuringInsertLeaveNoLeaksOrChanges) {
  // Проверяем исключение на каждом сравнении, включая сравнение после выделения узла в прежней реализации.
  for (int budget = 0; budget < 8; ++budget) {
    typename TypeParam::template Tree<Faulty, Faulty> tree;
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

TYPED_TEST(TreeSafetyTests, CopyFailuresDuringInsertLeaveTreeUnchanged) {
  for (bool initiallyEmpty : {true, false}) {
    for (int budget = 0; budget < 12; ++budget) {
      typename TypeParam::template Tree<Faulty, Faulty> tree;
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

TYPED_TEST(TreeSafetyTests, ComparisonFailuresDuringRemoveLeaveTreeUnchanged) {
  for (int budget = 0; budget < 8; ++budget) {
    typename TypeParam::template Tree<Faulty, Faulty> tree;
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

TYPED_TEST(TreeSafetyTests, DestructionAndMoveAssignmentReleaseAllValues) {
  using Tree = typename TypeParam::template Tree<int, Faulty>;
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

TYPED_TEST(TreeSafetyTests, AllocationFailureDuringInsertPreservesTree) {
  typename TypeParam::template Tree<int, int> tree;
  for (bool initiallyEmpty : {true, false}) {
    bool failed = false;
    {
      RejectAllocations guard;
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

TYPED_TEST(TreeSafetyTests, AllocationFailureReleasesPartialCopyAndPreservesBothTrees) {
  using Tree = CheckedTree<TypeParam>;
  if constexpr (std::is_copy_constructible_v<Tree> && std::is_copy_assignable_v<Tree>) {
    Tree source;
    const std::map<int, int> expected{{2, 20}, {3, 30}, {4, 40}, {6, 60},
                                      {7, 70}, {8, 80}, {9, 90}};
    for (int key : {4, 2, 8, 3, 6, 9, 7}) source.insert(key, expected.at(key));
    Tree target;
    target.insert(1, 10);
    target.insert(5, 50);

    for (int allowed : {2, 6}) {
      SCOPED_TRACE(allowed);
      std::ptrdiff_t balance = 0;
      EXPECT_TRUE(throwsExactly<std::bad_alloc>([&] {
        TrackAllocations track(balance);
        RejectAllocations reject(allowed);
        (void)Tree(source);
      }));
      EXPECT_EQ(balance, 0);
      source.verify(expected);

      balance = 0;
      EXPECT_TRUE(throwsExactly<std::bad_alloc>([&] {
        TrackAllocations track(balance);
        RejectAllocations reject(allowed);
        target = source;
      }));
      EXPECT_EQ(balance, 0);
      source.verify(expected);
      target.verify({{1, 10}, {5, 50}});
    }
  } else {
    GTEST_SKIP() << "Tree copying is unavailable";
  }
}

TYPED_TEST(TreeSafetyTests, MoveConstructionAndAssignmentPreserveInvariants) {
  using Tree = CheckedTree<TypeParam>;
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

TYPED_TEST(TreeSafetyTests, PreservesInvariantsAcrossInsertionOrdersAndRootRemoval) {
  std::vector<int> order{1, 2, 3, 4, 5};
  do {
    CheckedTree<TypeParam> tree;
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

TYPED_TEST(TreeSafetyTests, MixedOperationsMatchStdMapAndPreserveInvariants) {
  CheckedTree<TypeParam> tree;
  std::map<int, int> expected;
  std::mt19937 random(147);
  for (int step = 0; step < 20000; ++step) {
    int key = static_cast<int>(random() % 128);
    if (random() % 2) {
      tree.insert(key, step);
      expected.emplace(key, step);
    } else if (expected.erase(key)) {
      tree.remove(key);
    } else if (expected.empty()) {
      EXPECT_TRUE(throwsExactly<std::logic_error>([&] { tree.remove(key); }));
    } else {
      EXPECT_TRUE(throwsExactly<std::out_of_range>([&] { tree.remove(key); }));
    }
    tree.verify(expected);
  }
}

TYPED_TEST(TreeSafetyTests, DestructionAndMoveAssignmentDoNotAllocate) {
  using Tree = typename TypeParam::template Tree<int, Faulty>;
  auto* target = new Tree;
  Tree source;
  for (int key : {4, 2, 6, 1, 3, 5, 7}) target->insert(key, Faulty(key));
  source.insert(10, Faulty(100));

  {
    RejectAllocations guard;
    *target = std::move(source);
  }
  EXPECT_EQ(Faulty::alive, 1);
  EXPECT_EQ(target->size(), 1u);
  EXPECT_EQ(source.size(), 0u);
  {
    RejectAllocations guard;
    delete target;
  }
  EXPECT_EQ(Faulty::alive, 0);
}


TEST(ExpressionTreeAllocationTests, AllocationFailureKeepsOperandTrees) {
  auto left = ast::ExpressionTree::makeLeaf(UniquePtr<ast::Expr>(new ast::NumberLiteral("1")));
  auto right = ast::ExpressionTree::makeLeaf(UniquePtr<ast::Expr>(new ast::NumberLiteral("2")));
  const auto* leftRoot = left.getRoot();
  const auto* rightRoot = right.getRoot();
  bool failed = false;

  {
    RejectAllocations guard;
    try { ast::ExpressionTree::makeBinary(TokenType::Plus, std::move(left), std::move(right)); }
    catch (const std::bad_alloc&) { failed = true; }
  }

  EXPECT_TRUE(failed);
  EXPECT_EQ(left.getRoot(), leftRoot);
  EXPECT_EQ(right.getRoot(), rightRoot);
  EXPECT_EQ(left.size(), 1u);
  EXPECT_EQ(right.size(), 1u);
}

TEST(ExpressionTreeAllocationTests, ParseErrorReleasesPartialProgram) {
  const auto tokens = Lexer::tokenize("remember longIdentifier = 1 + 2\nprint (3 * 4 + 5");
  std::ptrdiff_t balance = 0;
  bool failed = false;
  {
    // Токены созданы заранее. Считаем только ресурсы, выделенные и освобождённые при разборе.
    TrackAllocations guard(balance);
    try { Parser{}.parse(tokens); }
    catch (const std::invalid_argument&) { failed = true; }
  }
  EXPECT_TRUE(failed);
  EXPECT_EQ(balance, 0);
}

}
