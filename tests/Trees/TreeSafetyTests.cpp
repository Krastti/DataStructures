#include "Trees/BinaryTree.h"
#include "Trees/RedBlackTree.h"
#include "Trees/Set.h"
#include <gtest/gtest.h>

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <map>
#include <new>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {
thread_local bool rejectAllocations = false;
}

// Отдельный исполняемый файл изолирует отказ выделения памяти от остальных тестов.
// Включаем отказ только на время проверяемой операции.
void* operator new(std::size_t size) {
  if (rejectAllocations) throw std::bad_alloc();
  if (void* memory = std::malloc(size == 0 ? 1 : size)) return memory;
  throw std::bad_alloc();
}

void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }
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
  RejectAllocations() { rejectAllocations = true; }
  ~RejectAllocations() { rejectAllocations = false; }
};

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
  EXPECT_THROW(tree.replace(1, 10), std::logic_error);
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
  EXPECT_THROW(tree.replace(8, 80), std::out_of_range);
  EXPECT_EQ(tree.size(), 7);
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
    EXPECT_THROW(tree.remove(0), std::logic_error);
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
    } else {
      EXPECT_THROW(tree.remove(key), std::logic_error);
    }
    tree.verify(expected);
  }
}
}
