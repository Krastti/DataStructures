#pragma once

template <typename Key, typename Data>
RedBlackTree<Key, Data>::Node* RedBlackTree<Key, Data>::getLeft() const {
  return static_cast<RBNode*>(this->left);
}

template <typename Key, typename Data>
RedBlackTree<Key, Data>::Node* RedBlackTree<Key, Data>::getRight() const {
  return static_cast<RBNode*>(this->right);
}

template <typename Key, typename Data>
RedBlackTree<Key, Data>::Node* RedBlackTree<Key, Data>::getParent() const {
  return static_cast<RBNode*>(this->parent);
}
template <typename Key, typename Data>
RedBlackTree<Key, Data>::Node* RedBlackTree<Key, Data>::getRoot() const {
  return static_cast<RBNode*>(this->root);
}

template <typename Key, typename Data>
void RedBlackTree<Key, Data>::leftRotate(Node *node) {
  if (node->right == nullptr) return;

  Node* parent = node->parent;
  Node* newRoot = node->right;
  Node* temp = newRoot->left;

  node->right = temp;
  if (temp != nullptr) temp->parent = node;

  newRoot->left = node;
  node->parent = newRoot;

  newRoot->parent = parent;
  if (parent == nullptr) {
    this->root = newRoot;
  } else if (parent->left == node) {
    parent->left = newRoot;
  } else {
    parent->right = newRoot;
  }
}

template <typename Key, typename Data>
void RedBlackTree<Key, Data>::rightRotate(Node *node) {
  if (node->left == nullptr) return;

  Node* parent = node->parent;
  Node* newRoot = node->left;
  Node* temp = newRoot->right;

  node->left = temp;
  if (temp != nullptr) temp->parent = node;

  newRoot->right = node;
  node->parent = newRoot;

  newRoot->parent = parent;
  if (parent == nullptr) {
    this->root = newRoot;
  } else if (parent->left == node) {
    parent->left = newRoot;
  } else {
    parent->right = newRoot;
  }
}

template <typename Key, typename Data>
void RedBlackTree<Key, Data>::fixInsert(Node *current) {
  Node* parent = current->parent;

  if (parent->color == Color::BLACK) return;

  while (current->parent->color == Color::RED) {
    Node* grandParent = parent->parent;
    Node* uncle;

    if (parent == grandParent->left) {
      uncle = grandParent->right;
    } else {
      uncle = grandParent->left;
    }

    if (uncle->color == Color::RED && uncle != nullptr) {
      parent->color = Color::BLACK;
      uncle->color = Color::BLACK;
      grandParent->color = Color::RED;
      current = grandParent;
    }

    if (uncle->color == Color::BLACK) {}
  }
}

template <typename Key, typename Data>
void RedBlackTree<Key, Data>::insert(Key key, Data data) {
  if (this->root == nullptr) {
    this->root = new RBNode(key, data, Color::BLACK);
    return;
  }

  Node* current = this->root;
  Node* parent = nullptr;

  while (current != nullptr) {
    parent = current;

    if (current->key > key) current = current->left;
    else if (current->key < key) current = current->right;
  }

  current = new RBNode(key, data, Color::RED);

  if (current->key < parent->key) parent->left = current;
  else if (current->key > parent->key) parent->right = current;
  current->parent = parent;
}

