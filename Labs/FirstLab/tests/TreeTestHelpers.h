#ifndef DATASTRUCTURES_TREE_TEST_HELPERS_H
#define DATASTRUCTURES_TREE_TEST_HELPERS_H

#include <gtest/gtest.h>

#include <algorithm>
#include <compare>
#include <cstddef>
#include <exception>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <map>
#include <random>
#include <stdexcept>
#include <typeinfo>
#include <utility>

namespace tree_test {

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

struct CopyFailure : std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct ComparisonFailure : std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct Faulty {
  inline static int alive = 0;
  inline static int comparisonsLeft = -1;
  inline static int copiesLeft = -1;
  int value;

  Faulty(int value = 0) : value(value) { ++alive; }
  Faulty(const Faulty& other) : value(other.value) {
    if (copiesLeft == 0) throw CopyFailure("copy failed");
    if (copiesLeft > 0) --copiesLeft;
    ++alive;
  }
  ~Faulty() { --alive; }
  Faulty& operator=(const Faulty&) { throw std::logic_error("unexpected assignment"); }

  static void compare() {
    if (comparisonsLeft == 0) throw ComparisonFailure("comparison failed");
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

// Проверяем ссылки и границы ключей до вызова публичных методов обхода.
template<class Node>
void verifySearchOrder(const Node* node, const Node* parent, long long lowerBound,
                       long long upperBound, std::size_t& count) {
  if (node == nullptr) {
    return;
  }
  ASSERT_EQ(node->parent, parent) << "node key=" << node->key;
  ASSERT_GT(node->key, lowerBound);
  ASSERT_LT(node->key, upperBound);
  ++count;
  ASSERT_NO_FATAL_FAILURE(verifySearchOrder(node->left, node, lowerBound, node->key, count));
  ASSERT_NO_FATAL_FAILURE(verifySearchOrder(node->right, node, node->key, upperBound, count));
}

template<class Tree, class Key, class Value>
void verifyRemovalPreservesRemainingEntries() {
  // В первом случае преемник — правый ребёнок, во втором он глубже и сам имеет правого ребёнка.
  const std::initializer_list<int> insertionOrders[] = {{2, 1, 3}, {4, 2, 8, 1, 3, 6, 9, 7}};
  for (const auto& order : insertionOrders) {
    SCOPED_TRACE(testing::PrintToString(order));
    Tree tree;
    for (int key : order) {
      tree.insert(Key(key), Value(key * 10));
    }

    for (auto removed = order.begin(); removed != order.end(); ++removed) {
      SCOPED_TRACE(testing::Message() << "removed key=" << *removed);
      ASSERT_NO_THROW(tree.remove(Key(*removed)));
      EXPECT_FALSE(tree.containsKey(Key(*removed)));
      EXPECT_EQ(tree.size(), static_cast<std::size_t>(std::distance(removed + 1, order.end())));
      for (auto remaining = removed + 1; remaining != order.end(); ++remaining) {
        SCOPED_TRACE(testing::Message() << "remaining key=" << *remaining);
        ASSERT_TRUE(tree.containsKey(Key(*remaining)));
        EXPECT_EQ(tree.at(Key(*remaining)), Value(*remaining * 10));
      }
    }
  }
}

template<class CheckedTree>
void verifyMixedOperationsAgainstStdMap() {
  enum class Operation { Insert, Remove, Replace, Lookup, Range };

  CheckedTree tree;
  std::map<int, int> expected;
  std::mt19937 random(147);
  for (int step = 0; step < 20000; ++step) {
    const int key = static_cast<int>(random() % 128) - 64;
    const auto found = expected.find(key);
    const auto operation = static_cast<Operation>(random() % 5);
    SCOPED_TRACE(testing::Message() << "step=" << step << ", key=" << key);
    switch (operation) {
      case Operation::Insert:
        tree.insert(key, step);
        expected.emplace(key, step);
        break;
      case Operation::Remove:
        if (found != expected.end()) {
          tree.remove(key);
          expected.erase(found);
        } else if (expected.empty()) {
          EXPECT_TRUE(throwsExactly<std::logic_error>([&] { tree.remove(key); }));
        } else {
          EXPECT_TRUE(throwsExactly<std::out_of_range>([&] { tree.remove(key); }));
        }
        break;
      case Operation::Replace:
        if (found != expected.end()) {
          tree.replace(key, -step);
          found->second = -step;
        } else if (expected.empty()) {
          EXPECT_TRUE(throwsExactly<std::logic_error>([&] { tree.replace(key, -step); }));
        } else {
          EXPECT_TRUE(throwsExactly<std::out_of_range>([&] { tree.replace(key, -step); }));
        }
        break;
      case Operation::Lookup:
        EXPECT_EQ(tree.containsKey(key), found != expected.end());
        if (found != expected.end()) {
          EXPECT_EQ(std::as_const(tree).at(key), found->second);
        } else {
          EXPECT_TRUE(throwsExactly<std::out_of_range>([&] { (void)std::as_const(tree).at(key); }));
        }
        break;
      case Operation::Range: {
        const int other = static_cast<int>(random() % 160) - 80;
        const auto [first, last] = std::minmax(key, other);
        const auto matches = tree.range(first, last);
        auto entry = expected.lower_bound(first);
        const auto end = expected.upper_bound(last);
        ASSERT_EQ(matches.size(), static_cast<std::size_t>(std::distance(entry, end)));
        for (std::size_t i = 0; entry != end; ++entry, ++i) {
          EXPECT_EQ(matches[i], (std::pair{entry->first, entry->second}));
        }
        break;
      }
    }
    ASSERT_NO_FATAL_FAILURE(tree.verify(expected));
    if (!expected.empty()) {
      EXPECT_EQ(tree.min(), expected.begin()->second);
      EXPECT_EQ(tree.max(), expected.rbegin()->second);
    }
  }
}

} // namespace tree_test

#endif // DATASTRUCTURES_TREE_TEST_HELPERS_H
