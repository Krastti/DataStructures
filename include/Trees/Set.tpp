#pragma once
#include <stdexcept>
#include <utility>

template <typename Key>
Set<Key>::Set() : root(nullptr) {}

template <typename Key>
Set<Key>::Set(Key key) : root(new Node(key)) {}

template <typename Key>
void Set<Key>::insert(Key key) {
  if (root == nullptr) {
    root.reset(new Node(std::move(key)));
    return;
  }

  Node* current = root.get();
  Node* parent = nullptr;

  while (current != nullptr) {
    parent = current;

    if (current->key > key) current = current->left.get();
    else if (current->key < key) current = current->right.get();
    else return;
  }

  UniquePtr<Node> inserted(new Node(std::move(key), parent));

  if (inserted->key < parent->key) parent->left = std::move(inserted);
  else parent->right = std::move(inserted);
}

template <typename Key>
bool Set<Key>::contains(const Key& key) const {
  Node* current = root.get();
  while (current != nullptr) {
    if (current->key == key) return true;
    current = current->key > key ? current->left.get() : current->right.get();
  }
  return false;
}

template <typename Key>
void Set<Key>::remove(const Key& key) {
  Node* target = root.get();
  while (target != nullptr && target->key != key) {
    target = target->key > key ? target->left.get() : target->right.get();
  }
  if (target == nullptr) throw std::out_of_range("Key does not exist");

  if (target->left != nullptr && target->right != nullptr) {
    Node* successor = target->right.get();
    while (successor->left != nullptr) successor = successor->left.get();
    target->key = successor->key;
    target = successor;
  }

  Node* parent = target->parent;
  UniquePtr<Node> replacement;
  if (target->left != nullptr) replacement = std::move(target->left);
  else if (target->right != nullptr) replacement = std::move(target->right);
  if (replacement != nullptr) replacement->parent = parent;

  if (parent == nullptr) {
    root = std::move(replacement);
  } else if (parent->left.get() == target) {
    parent->left = std::move(replacement);
  } else {
    parent->right = std::move(replacement);
  }
}
