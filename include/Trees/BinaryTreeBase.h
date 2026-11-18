#ifndef DATASTRUCTURES_BINARY_TREE_BASE_H
#define DATASTRUCTURES_BINARY_TREE_BASE_H

#include <cstddef>

template<typename Node>
class BinaryTreeBase {
protected:
  Node* root = nullptr;
  std::size_t nodeCount = 0;

  BinaryTreeBase() noexcept = default;
  BinaryTreeBase(const BinaryTreeBase&) = delete;
  BinaryTreeBase& operator=(const BinaryTreeBase&) = delete;
  BinaryTreeBase(BinaryTreeBase&& other) noexcept;
  BinaryTreeBase& operator=(BinaryTreeBase&& other) noexcept;
  ~BinaryTreeBase();

  void clear() noexcept;
};

#include "BinaryTreeBase.tpp"

#endif //DATASTRUCTURES_BINARY_TREE_BASE_H
