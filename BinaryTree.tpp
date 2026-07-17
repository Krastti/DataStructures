#pragma once

#include <stdexcept>

template <typename Key, typename Data>
BinaryTree<Key, Data>::BinaryTree() {
  root = nullptr;
}

template <typename Key, typename Data>
BinaryTree<Key, Data>::BinaryTree(Key key, Data data) {
  root = new Node(key, data);
}

template <typename Key, typename Data>
void BinaryTree<Key, Data>::insert(Key key, Data data) {
  if (root == nullptr) {
    root = new Node(key, data);
    return;
  }

  Node *current = root;
  Node *par = nullptr;

  while (current != nullptr) {
    par = current;

    if (current->key > key) current = current->left;
    else if (current->key < key) current = current->right;
  }

  current = new Node(key, data);
  if (current->key < par->key) par->left = current;
  else if (current->key > par->key) par->right = current;
}

/*
 * 1. Найти реально удаляемый элемент y
 * 2. Найти поддерево удаляемого элемента - р
 *    Родительский узел для узла у - par
 * 3. Переопределить родительский узел
 * 4. Проверить случай, что у - это корень дерева
 * 5. Переопределить дочерний узел для узла par
 * 6. Скопировать необходимые данные
 */
template<typename Key, typename Data>
void BinaryTree<Key, Data>::remove(Key key) {
  if (root == nullptr) throw std::logic_error("Root is NULL");

  Node** current = &root;
  
}

template <typename Key, typename Data>
Data &BinaryTree<Key, Data>::get_root() const {
  if (root == nullptr) throw std::logic_error("Root is NULL");
  return root->data;
}

template <typename Key, typename Data>
Data &BinaryTree<Key, Data>::get(Key key) const {
  if (root == nullptr) throw std::logic_error("Root is NULL");
  if (root->key == key) return get_root();

  Node *current = root;
  while (current->key != key) {
    if (current->key > key) {
      current = current->left;
      if (current == nullptr) throw std::out_of_range("Key does not exist");
    }
    else if (current->key < key) {
      current = current->right;
      if (current == nullptr) throw std::out_of_range("Key does not exist");
    }
  }
  return current->data;
}

template<typename Key, typename Data>
Data & BinaryTree<Key, Data>::min() const {
  if (root == nullptr) throw std::logic_error("Root is NULL");

  Node *current = root;
  while (current->left != nullptr) current = current->left;
  return current->data;
}

template<typename Key, typename Data>
Data & BinaryTree<Key, Data>::max() const {
  if (root == nullptr) throw std::logic_error("Root is NULL");

  Node *current = root;
  while (current->right != nullptr) current = current->right;
  return current->data;
}

template <typename Key, typename Data>
BinaryTree<Key, Data>::~BinaryTree() = default;
