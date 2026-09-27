#pragma once
#include <stdexcept>
#include <iostream>

template<typename Key, typename Data>
BinaryTree<Key, Data>::Node * BinaryTree<Key, Data>::getSuccessor(Node *node) {
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
BinaryTree<Key, Data>::BinaryTree() : root(nullptr), nodeCount(0) { }

template <typename Key, typename Data>
BinaryTree<Key, Data>::BinaryTree(Key key) : root(nullptr), nodeCount(0) {
  root = new Node(key, Data());
  nodeCount = 1;
}

template <typename Key, typename Data>
BinaryTree<Key, Data>::BinaryTree(Key key, Data data) : root(nullptr), nodeCount(0) {
  root = new Node(key, data);
  nodeCount = 1;
}

template <typename Key, typename Data>
BinaryTree<Key, Data>::BinaryTree(BinaryTree&& other) noexcept
  : root(std::exchange(other.root, nullptr)), nodeCount(std::exchange(other.nodeCount, 0)) {}

template <typename Key, typename Data>
BinaryTree<Key, Data>& BinaryTree<Key, Data>::operator=(BinaryTree&& other) noexcept {
  if (this != &other) {
    clear();
    root = std::exchange(other.root, nullptr);
    nodeCount = std::exchange(other.nodeCount, 0);
  }
  return *this;
}

template <typename Key, typename Data>
void BinaryTree<Key, Data>::insert(Key key, Data data) {
  if (root == nullptr) {
    root = new Node(key, data);
    nodeCount = 1;
    return;
  }

  Node* current = root;
  Node* parent = nullptr;

  while (current != nullptr) {
    parent = current;

    if (current->key > key) current = current->left;
    else if (current->key < key) current = current->right;
    else return;
  }

  current = new Node(key, data);

  if (current->key < parent->key) parent->left = current;
  else if (current->key > parent->key) parent->right = current;
  current->parent = parent;
  ++nodeCount;
}

template<typename Key, typename Data>
void BinaryTree<Key, Data>::remove(Key key) {
  if (root == nullptr) throw std::logic_error("Root is NULL");

  Node** current = &root;
  Node* parent = nullptr;
  Node* son = nullptr;

  while (*current != nullptr && (*current)->key != key) {
    if ((*current)->key > key) current = &((*current)->left);
    else if ((*current)->key < key) current = &((*current)->right);
  }
  if (*current == nullptr) throw std::out_of_range("Key does not exist");

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
  --nodeCount;
}

template<typename Key, typename Data>
void BinaryTree<Key, Data>::replace(Key key, Data data) {
  if (root == nullptr) throw std::logic_error("Root is NULL");

  Node** current = &root;

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

  (*current)->data = data;
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

template <typename Key, typename Data>
Data BinaryTree<Key, Data>::find(Key key) const {
  return get(key);
}

template<typename Key, typename Data>
Data& BinaryTree<Key, Data>::at(const Key& key) {
  Node* current = root;
  while (current != nullptr) {
    if (current->key == key) return current->data;
    current = current->key > key ? current->left : current->right;
  }
  throw std::out_of_range("Key does not exist");
}

template<typename Key, typename Data>
const Data& BinaryTree<Key, Data>::at(const Key& key) const {
  const Node* current = root;
  while (current != nullptr) {
    if (current->key == key) return current->data;
    current = current->key > key ? current->left : current->right;
  }
  throw std::out_of_range("Key does not exist");
}

template<typename Key, typename Data>
bool BinaryTree<Key, Data>::containsKey(const Key& key) const {
  const Node* current = root;
  while (current != nullptr) {
    if (current->key == key) return true;
    current = current->key > key ? current->left : current->right;
  }
  return false;
}

template<typename Key, typename Data>
size_t BinaryTree<Key, Data>::size() const noexcept { return nodeCount; }

template<typename Key, typename Data>
Array<std::pair<Key, Data>> BinaryTree<Key, Data>::entries() const {
  Array<std::pair<Key, Data>> result;
  Stack<Node*> nodes;
  Node* current = root;

  while (current != nullptr || !nodes.empty()) {
    while (current != nullptr) {
      nodes.push(current);
      current = current->left;
    }
    current = nodes.top();
    nodes.pop();
    result.push_back({current->key, current->data});
    current = current->right;
  }
  return result;
}

template<typename Key, typename Data>
Array<Key> BinaryTree<Key, Data>::keys() const {
  Array<Key> result;
  Stack<Node*> nodes;
  Node* current = root;
  while (current != nullptr || !nodes.empty()) {
    while (current != nullptr) {
      nodes.push(current);
      current = current->left;
    }
    current = nodes.top();
    nodes.pop();
    result.push_back(current->key);
    current = current->right;
  }
  return result;
}

template<typename Key, typename Data>
Array<std::pair<Key, Data>> BinaryTree<Key, Data>::range(const Key& first, const Key& last) const {
  if (first > last) throw std::invalid_argument("The first key must not exceed the last key");
  Array<std::pair<Key, Data>> result;
  Stack<Node*> nodes;
  Node* current = root;
  while (current != nullptr || !nodes.empty()) {
    while (current != nullptr) {
      if (current->key >= first) {
        nodes.push(current);
        current = current->left;
      } else {
        current = current->right;
      }
    }
    if (nodes.empty()) break;
    current = nodes.top();
    nodes.pop();
    if (current->key > last) break;
    result.push_back({current->key, current->data});
    current = current->right;
  }
  return result;
}

template <typename Key, typename Data>
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
void BinaryTree<Key, Data>::print() const {
  Node* current = root;
  auto stack = new Stack<Node*>;

  while (current != nullptr or !stack->empty()) {
    while (current != nullptr) {
      stack->push(current);
      current = current->left;
    }
    current = stack->top();
    stack->pop();
    std::cout << current->key << ' ';
    current = current->right;
  }
  delete stack;
}

template <typename Key, typename Data>
void BinaryTree<Key, Data>::clear() noexcept {
  if (root == nullptr) {
    nodeCount = 0;
    return;
  }

  auto stack = new Stack<Node*>;
  stack->push(root);

  while (!stack->empty()) {
    Node *current = stack->top();
    stack->pop();

    if (current->left != nullptr) stack->push(current->left);
    if (current->right != nullptr) stack->push(current->right);

    delete current;
  }
  delete stack;
  root = nullptr;
  nodeCount = 0;
}

template <typename Key, typename Data>
BinaryTree<Key, Data>::~BinaryTree() {
  clear();
}
