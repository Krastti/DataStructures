#pragma once
#include <stdexcept>

template<typename Key, typename Data>
typename BinaryTree<Key, Data>::Node * BinaryTree<Key, Data>::get_successor(Node *node) {
  if (root == nullptr) throw std::logic_error("Root is NULL");
  if (node == nullptr) throw std::logic_error("Node is NULL");

  Node *ptr = nullptr;
  if (node->right) {
    Node *p = node->right;
    while (p->left != nullptr) p = p->left;
    return p;
  }
  ptr = node->parent;
  while (ptr != nullptr && ptr->right == node) {
    node = ptr;
    ptr = node->parent;
  }
  return ptr;
}

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
  current->parent = par;
}

template<typename Key, typename Data>
void BinaryTree<Key, Data>::remove(Key key) {
  if (root == nullptr) throw std::logic_error("Root is NULL");

  Node** current = &root;
  Node* par = nullptr;
  Node* p = nullptr;

  while ((*current)->key != key) {
    if ((*current)->key > key) {
      current = &((*current)->left);
      if (*current == nullptr) throw std::out_of_range("Key does not exist");
    }
    else if ((*current)->key < key) {
      current = &((*current)->right);
      if (*current == nullptr) throw std::out_of_range("Key does not exist");
    }
  }

  // Случай, когда нет потомком или один потомок
  if ((*current)->right == nullptr || (*current)->left == nullptr) {
    par = (*current)->parent;

    if ((*current)->left != nullptr) p = (*current)->left;
    else if ((*current)->right != nullptr) p = (*current)->right;

    if (p != nullptr) p->parent = par;
    if (par == nullptr) {
      delete *current;
      root = p;
    } else if (*current == par->left) {
      delete *current;
      par->left = p;
    } else if (*current == par->right) {
      delete *current;
      par->right = p;
    }
  }

  // Случай, когда два потомка
  else if ((*current)->left != nullptr && (*current)->right != nullptr) {
    p = get_successor(*current);

    if (p != (*current)->right) p->parent->left = p->right;
    else (*current)->right = p->right;

    if (p->right != nullptr) p->right->parent = p->parent;
    (*current)->key = p->key;
    (*current)->data = p->data;
    delete p;
  }
}

template <typename Key, typename Data>
Data BinaryTree<Key, Data>::get_root() const {
  if (root == nullptr) throw std::logic_error("Root is NULL");
  return root->data;
}

template <typename Key, typename Data>
Data BinaryTree<Key, Data>::get(Key key) const {
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
Data BinaryTree<Key, Data>::min() const {
  if (root == nullptr) throw std::logic_error("Root is NULL");

  Node *current = root;
  while (current->left != nullptr) current = current->left;
  return current->data;
}

template<typename Key, typename Data>
Data BinaryTree<Key, Data>::max() const {
  if (root == nullptr) throw std::logic_error("Root is NULL");

  Node *current = root;
  while (current->right != nullptr) current = current->right;
  return current->data;
}

template<typename Key, typename Data>
Data BinaryTree<Key, Data>::get_data_parent(Key key) {
  if (root == nullptr) throw std::logic_error("Root is NULL");

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
  if (current->parent == nullptr) {
    throw std::logic_error("Parent is NULL");
  } else {
    return current->parent->data;
  }

}

template <typename Key, typename Data>
BinaryTree<Key, Data>::~BinaryTree() = default;
