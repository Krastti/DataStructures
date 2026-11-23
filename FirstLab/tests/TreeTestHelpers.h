#ifndef DATASTRUCTURES_TREE_TEST_HELPERS_H
#define DATASTRUCTURES_TREE_TEST_HELPERS_H

#include "BinaryTree.h"
#include "RedBlackTree.h"
#include <gtest/gtest.h>

#include <cstddef>
#include <exception>
#include <limits>
#include <map>
#include <typeinfo>

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

struct BinaryTag {
  template<class K, class V> using Tree = BinaryTree<K, V>;
  static constexpr bool balanced = false;
};
struct RedBlackTag {
  template<class K, class V> using Tree = RedBlackTree<K, V>;
  static constexpr bool balanced = true;
};

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

  void verifyCopyOf(const CheckedTree& source) const {
    const auto check = [this, &source](const auto& visit, const Node* original,
                                      const Node* copy, const Node* parent) -> void {
      if (original == nullptr) {
        EXPECT_EQ(copy, nullptr);
        return;
      }
      ASSERT_NE(copy, nullptr);
      EXPECT_NE(copy, original);
      EXPECT_EQ(copy->parent, parent);
      EXPECT_EQ(copy->key, original->key);
      EXPECT_EQ(copy->data, original->data);
      if constexpr (Tag::balanced) {
        EXPECT_EQ(this->isRed(copy->key), source.isRed(original->key));
      }
      visit(visit, original->left, copy->left, copy);
      visit(visit, original->right, copy->right, copy);
    };
    EXPECT_EQ(this->size(), source.size());
    check(check, source.root, this->root, nullptr);
  }
};

} // namespace tree_test

#endif // DATASTRUCTURES_TREE_TEST_HELPERS_H
