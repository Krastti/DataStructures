#pragma once

template<typename Node>
BinaryTreeBase<Node>::BinaryTreeBase(BinaryTreeBase&& other) noexcept
    : root(other.root), nodeCount(other.nodeCount) {
  other.root = nullptr;
  other.nodeCount = 0;
}

template<typename Node>
BinaryTreeBase<Node>& BinaryTreeBase<Node>::operator=(BinaryTreeBase&& other) noexcept {
  if (this == &other) return *this;

  clear();
  root = other.root;
  nodeCount = other.nodeCount;
  other.root = nullptr;
  other.nodeCount = 0;
  return *this;
}

template<typename Node>
void BinaryTreeBase<Node>::clear() noexcept {
  Node* current = root;
  // Возвращаемся по parent, чтобы обойтись без дополнительного стека.
  while (current != nullptr) {
    if (current->left != nullptr) {
      current = current->left;
      continue;
    }
    if (current->right != nullptr) {
      current = current->right;
      continue;
    }

    Node* parent = current->parent;
    if (parent != nullptr) {
      if (parent->left == current) parent->left = nullptr;
      else parent->right = nullptr;
    }
    delete current;
    current = parent;
  }
  root = nullptr;
  nodeCount = 0;
}

template<typename Node>
BinaryTreeBase<Node>::~BinaryTreeBase() {
  clear();
}
