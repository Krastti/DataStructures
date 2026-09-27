#pragma once
#include <stdexcept>

template <typename Key>
Set<Key>::Set() : root(nullptr) {}

template <typename Key>
Set<Key>::Set(Key key) : root(new Node(key)) {}

template <typename Key>
void Set<Key>::insert(Key key) {
  if (root == nullptr) {
    root = new Node(key);
    return;
  }

  Node* current = root;
  Node* parent = nullptr;

  while (current != nullptr) {
    parent = current;

    if (current->key > key) current = current->left;
    else if (current->key < key) current = current->right;
  }

  current = new Node(key);

  if (current->key < parent->key) parent->left = current;
  else if (current->key > parent->key) parent->right = current;
  current->parent = parent;
}

template <typename Key>
void Set<Key>::remove(Key key) {
  if (root == nullptr) throw std::logic_error("Root is NULL");

  Node** current = &root;
  Node* parent = nullptr;
  Node* son = nullptr;

  while (*current != nullptr && (*current)->key != key) {
    if ((*current)->key > key) current = &((*current)->left);
    else if ((*current)->key < key) current = &((*current)->right);
  }
  if (*current == nullptr) throw std::out_of_range("Key does not exist");
  if (*current-> key == key) throw std::out_of_range("Key already exists");

  // Случай, когда нет потомком или один потомок
  if ((*current)->right == nullptr || (*current)->left == nullptr) {
    parent = (*current)->parent;

    if ((*current)->left != nullptr) son = (*current)->left;
    else if ((*current)->right != nullptr) son = (*current)->right;

    if (son != nullptr) son->parent = parent;
    if (parent == nullptr) {
      delete *current;
      root = son;
    } else if (*current == parent->left) {
      delete *current;
      parent->left = son;
    } else if (*current == parent->right) {
      delete *current;
      parent->right = son;
    }
  }

  // Случай, когда два потомка
  else if ((*current)->left != nullptr && (*current)->right != nullptr) {
    son = getSuccessor(*current);

    if (son != (*current)->right) son->parent->left = son->right;
    else (*current)->right = son->right;

    if (son->right != nullptr) son->right->parent = son->parent;
    (*current)->key = son->key;
    (*current)->data = son->data;
    delete son;
  }
}
