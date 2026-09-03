// ReSharper disable CppDFAUnreachableFunctionCall
#pragma once

template <typename Key, typename Data>
RedBlackTree<Key, Data>::RBNode* RedBlackTree<Key, Data>::getLeft(Node* node) {
  return static_cast<RBNode*>(node->left);
}

template <typename Key, typename Data>
RedBlackTree<Key, Data>::RBNode* RedBlackTree<Key, Data>::getRight(Node* node) {
  return static_cast<RBNode*>(node->right);
}

template <typename Key, typename Data>
RedBlackTree<Key, Data>::RBNode* RedBlackTree<Key, Data>::getParent(Node* node) {
  return static_cast<RBNode*>(node->parent);
}

template <typename Key, typename Data>
RedBlackTree<Key, Data>::RBNode* RedBlackTree<Key, Data>::getRoot() const {
  return static_cast<RBNode*>(this->root);
}

template <typename Key, typename Data>
void RedBlackTree<Key, Data>::leftRotate(RBNode *node) {
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
void RedBlackTree<Key, Data>::rightRotate(RBNode *node) {
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
void RedBlackTree<Key, Data>::fixInsert(RBNode *current) {
  RBNode* parent = getParent(current);

  while (parent != nullptr && parent->color == Color::RED) {
    RBNode* grandParent = getParent(parent);

    if (parent == getLeft(grandParent)) {
      RBNode* uncle = getRight(grandParent);

      if (uncle != nullptr && uncle->color == Color::RED) {
        parent->color = Color::BLACK;
        uncle->color = Color::BLACK;
        grandParent->color = Color::RED;
        current = grandParent;
      } else {
        if (current == getRight(parent)) {
          current = parent;
          leftRotate(current);
          parent = getParent(current);
        }
        parent->color = Color::BLACK;
        grandParent->color = Color::RED;
        rightRotate(grandParent);
      }
    } else {
      RBNode* uncle = getLeft(grandParent);

      if (uncle != nullptr && uncle->color == Color::RED) {
        parent->color = Color::BLACK;
        uncle->color = Color::BLACK;
        grandParent->color = Color::RED;
        current = grandParent;
      } else {
        if (current == getLeft(parent)) {
          current = parent;
          rightRotate(current);
          parent = getParent(current);
        }
        parent->color = Color::BLACK;
        grandParent->color = Color::RED;
        leftRotate(grandParent);
      }
    }

    parent = getParent(current);
  }

  getRoot()->color = Color::BLACK;
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
    else return;
  }

  auto newNode = new RBNode(key, data, Color::RED);

  if (newNode->key < parent->key) parent->left = newNode;
  else parent->right = newNode;
  newNode->parent = parent;

  fixInsert(newNode);
}

template <typename Key, typename Data>
bool RedBlackTree<Key, Data>::isRed(Key key) const {
  if (int colorNode = getColor(key);
      colorNode ==  static_cast<int>(Color::RED)) return true;
  else return false;
}

template <typename Key, typename Data>
bool RedBlackTree<Key, Data>::isBlack(Key key) const {
  if (int colorNode = getColor(key);
      colorNode == static_cast<int>(Color::BLACK)) return true;
  else return false;
}

template <typename Key, typename Data>
int RedBlackTree<Key, Data>::getColor(Key key) const {
  if (this->root == nullptr) throw std::logic_error("Root is NULL");

  Node* current = this->root;

  while (current != nullptr && current->key != key) {
    if (current->key > key) current = current->left;
    else current = current->right;
  }

  if (current == nullptr) throw std::out_of_range("Key does not exist");

  return static_cast<int>(static_cast<RBNode*>(current)->color);
}
