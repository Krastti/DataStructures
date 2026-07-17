#pragma once

#include <iostream>
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
  while (current != nullptr) {
    if (current->key < key) current = current->left;
    else if (current->key > key) current = current->right;
  }

  current = new Node(key, data);
}

template <typename Key, typename Data>
Data &BinaryTree<Key, Data>::get_root() const {
  if (root == nullptr) throw std::runtime_error("Root is NULL");
  return root->data;
}

template <typename Key, typename Data>
Data &BinaryTree<Key, Data>::get(Key key) const {
  if (root->key == key) return get_root();

  Node *current = root;
  try {
    while (current->key != key) {
      if (current->key < key) current = current->left;
      else current = current->right;
    }
    throw std::out_of_range("Key does not exist");
  }
  catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
  }

  return current->data;
}

template <typename Key, typename Data>
BinaryTree<Key, Data>::~BinaryTree() = default;