#pragma once
#include <stdexcept>
#include <iostream>

template <typename Key, typename Data>
void BinaryTree<Key, Data>::clear() noexcept {
  // Поднимаемся от удалённого листа по ссылке на родителя без выделения памяти.
  Node* current = root;
  while (current != nullptr) {
    if (current->left != nullptr) {
      current = current->left;
    } else if (current->right != nullptr) {
      current = current->right;
    } else {
      Node* parent = current->parent;
      if (parent != nullptr) {
        if (parent->left == current) parent->left = nullptr;
        else parent->right = nullptr;
      }
      delete current;
      current = parent;
    }
  }
  root = nullptr;
  nodeCount = 0;
}

template<typename Key, typename Data>
void BinaryTree<Key, Data>::transplant(Node* oldNode, Node* newNode) noexcept {

  if (oldNode->parent == nullptr) {
    root = newNode;
  } else if (oldNode == oldNode->parent->left) {
    oldNode->parent->left = newNode;
  } else {
    oldNode->parent->right = newNode;
  }

  if (newNode != nullptr) {
    newNode->parent = oldNode->parent;
  }
}

template <typename Key, typename Data>
template <typename CloneNode>
void BinaryTree<Key, Data>::copyNodes(const Node* source, CloneNode cloneNode) {
  if (source == nullptr) return;

  root = cloneNode(*source);
  Node* copy = root;
  size_t copiedCount = 0;

  // Готовые ветви копии отмечают пройденные ветви исходного дерева.
  while (copy != nullptr) {
    if (source->left != nullptr && copy->left == nullptr) {
      source = source->left;
      copy->left = cloneNode(*source);
      copy->left->parent = copy;
      copy = copy->left;
    } else if (source->right != nullptr && copy->right == nullptr) {
      source = source->right;
      copy->right = cloneNode(*source);
      copy->right->parent = copy;
      copy = copy->right;
    } else {
      ++copiedCount;
      source = source->parent;
      copy = copy->parent;
    }
  }
  nodeCount = copiedCount;
}

template <typename Key, typename Data>
typename BinaryTree<Key, Data>::Node* BinaryTree<Key, Data>::clonePlainNode(const Node& source) {
  return new Node(source.key, source.data);
}

template<typename Key, typename Data>
const typename BinaryTree<Key, Data>::Node* BinaryTree<Key, Data>::findNode(const Key& key) const {
  const Node* current = root;
  while (current != nullptr) {
    if (current->key == key) return current;
    current = current->key > key ? current->left : current->right;
  }
  return nullptr;
}

template<typename Key, typename Data>
typename BinaryTree<Key, Data>::Node* BinaryTree<Key, Data>::findNode(const Key& key) {
  return const_cast<Node*>(std::as_const(*this).findNode(key));
}


template<typename Key, typename Data>
typename BinaryTree<Key, Data>::Node* BinaryTree<Key, Data>::getSuccessor(Node* node) {
  return const_cast<Node*>(std::as_const(*this).getSuccessor(node));
}

template<typename Key, typename Data>
const typename BinaryTree<Key, Data>::Node* BinaryTree<Key, Data>::getSuccessor(const Node* node) const {
  if (root == nullptr) throw std::logic_error("Root is NULL");
  if (node == nullptr) throw std::logic_error("Node is NULL");

  if (node->right) {
    const Node* next = node->right;
    while (next->left != nullptr) next = next->left;
    return next;
  }
  const Node* parent = node->parent;
  while (parent != nullptr && parent->right == node) {
    node = parent;
    parent = node->parent;
  }
  return parent;
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
BinaryTree<Key, Data>::BinaryTree(const BinaryTree& other) : BinaryTree() {
  copyNodes(other.root, clonePlainNode);
}

template <typename Key, typename Data>
BinaryTree<Key, Data>& BinaryTree<Key, Data>::operator=(const BinaryTree& other) {
  if (this != &other) {
    BinaryTree copy(other);
    *this = std::move(copy);
  }
  return *this;
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

  Node** slot = &root;
  Node* parent = nullptr;

  while (*slot != nullptr) {
    parent = *slot;

    if (parent->key > key) slot = &parent->left;
    else if (parent->key < key) slot = &parent->right;
    else return;
  }

  // Завершаем сравнения до выделения памяти, чтобы исключение не привело к утечке.
  *slot = new Node(key, data, nullptr, nullptr, parent);
  ++nodeCount;
}

template<typename Key, typename Data>
void BinaryTree<Key, Data>::remove(Key key) {
  if (root == nullptr) throw std::logic_error("Root is NULL");

  Node* deletedNode = findNode(key);
  if (deletedNode == nullptr) throw std::out_of_range("Key does not exist");

  if (deletedNode->left == nullptr) {
    transplant(deletedNode, deletedNode->right);
  } else if (deletedNode->right == nullptr) {
    transplant(deletedNode, deletedNode->left);
  } else {
    // Переставляем сам узел-преемник, поскольку присваивание ключа или значения может бросить исключение.
    Node* successor = getSuccessor(deletedNode);
    if (successor != deletedNode->right) {
      transplant(successor, successor->right);
      successor->right = deletedNode->right;
      successor->right->parent = successor;
    }
    transplant(deletedNode, successor);
    successor->left = deletedNode->left;
    successor->left->parent = successor;
  }
  delete deletedNode;
  --nodeCount;
}

template<typename Key, typename Data>
void BinaryTree<Key, Data>::replace(Key key, Data data) {
  if (root == nullptr) throw std::logic_error("Root is NULL");

  Node* node = findNode(key);
  if (node == nullptr) throw std::out_of_range("Key does not exist");
  node->data = data;
}

template <typename Key, typename Data>
Data BinaryTree<Key, Data>::get_root() const {
  if (root == nullptr) throw std::logic_error("Root is NULL");
  return root->data;
}

template<typename Key, typename Data>
Data& BinaryTree<Key, Data>::at(const Key& key) {
  Node* node = findNode(key);
  if (node == nullptr) throw std::out_of_range("Key does not exist");
  return node->data;
}

template<typename Key, typename Data>
const Data& BinaryTree<Key, Data>::at(const Key& key) const {
  const Node* node = findNode(key);
  if (node == nullptr) throw std::out_of_range("Key does not exist");
  return node->data;
}

template<typename Key, typename Data>
bool BinaryTree<Key, Data>::containsKey(const Key& key) const {
  return findNode(key) != nullptr;
}

template<typename Key, typename Data>
bool BinaryTree<Key, Data>::containsSubtree(const BinaryTree& subtree) const {
  if (subtree.root == nullptr) return true;

  const Node* candidate = findNode(subtree.root->key);
  if (candidate == nullptr) return false;

  Stack<std::pair<const Node*, const Node*>> nodesToCompare;
  nodesToCompare.push({candidate, subtree.root});

  while (!nodesToCompare.empty()) {
    const auto [actual, expected] = nodesToCompare.top();
    nodesToCompare.pop();

    if (actual == nullptr || expected == nullptr) {
      if (actual != expected) return false;
      continue;
    }

    if (!(actual->key == expected->key) || !(actual->data == expected->data)) {
      return false;
    }

    nodesToCompare.push({actual->right, expected->right});
    nodesToCompare.push({actual->left, expected->left});
  }
  return true;
}

template<typename Key, typename Data>
BinaryTree<Key, Data> BinaryTree<Key, Data>::extractSubtree(const Key& key) const {
  const Node* subtreeRoot = findNode(key);
  if (subtreeRoot == nullptr) throw std::out_of_range("Key does not exist");

  BinaryTree result;
  result.copyNodes(subtreeRoot, clonePlainNode);
  return result;
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
  const Node* current = root;
  Stack<const Node*> stack;

  while (current != nullptr || !stack.empty()) {
    while (current != nullptr) {
      stack.push(current);
      current = current->left;
    }
    current = stack.top();
    stack.pop();
    std::cout << current->key << ' ';
    current = current->right;
  }
}

template <typename Key, typename Data>
BinaryTree<Key, Data>::~BinaryTree() {
  clear();
}
