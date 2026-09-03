#pragma once

template <typename Key, typename Data>
RedBlackTree<Key, Data>::Node* RedBlackTree<Key, Data>::left() {
  return static_cast<RBNode*>(this->left);
}

template <typename Key, typename Data>
RedBlackTree<Key, Data>::Node* RedBlackTree<Key, Data>::right() const {
  return static_cast<RBNode*>(this->right);
}

template <typename Key, typename Data>
RedBlackTree<Key, Data>::Node* RedBlackTree<Key, Data>::parent() const {
  return static_cast<RBNode*>(this->parent);
}

template <typename Key, typename Data>
RedBlackTree<Key, Data>::RedBlackTree() : root(nullptr) {}

