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
void RedBlackTree<Key, Data>::fixRemove(RBNode *node, RBNode *parent) {
  while (node != getRoot() && (node == nullptr || node->color == Color::BLACK)) {
    if (parent == nullptr) break;

    if (node == parent->left) {
      RBNode* sibling = getRight(parent);

      // Случай 1: брат красный
      if (sibling->color == Color::RED) {
        sibling->color = Color::BLACK;
        parent->color = Color::RED;
        leftRotate(parent);
        sibling = getRight(parent);
      }

      RBNode* sibLeft = getLeft(sibling);
      RBNode* sibRight = getRight(sibling);
      bool leftBlack = (sibLeft == nullptr || sibLeft->color == Color::BLACK);
      bool rightBlack = (sibRight == nullptr || sibRight->color == Color::BLACK);

      if (leftBlack && rightBlack) {
        // Случай 2: у брата оба ребёнка чёрные
        sibling->color = Color::RED;
        node = parent;
        parent = getParent(node);
      } else {
        if (rightBlack) {
          // Случай 3: ближний (левый) ребёнок брата красный
          if (sibLeft != nullptr) sibLeft->color = Color::BLACK;
          sibling->color = Color::RED;
          rightRotate(sibling);
          sibling = getRight(parent);
        }
        // Случай 4: дальний (правый) ребёнок брата красный
        sibling->color = parent->color;
        parent->color = Color::BLACK;
        RBNode* sr = getRight(sibling);
        if (sr != nullptr) sr->color = Color::BLACK;
        leftRotate(parent);
        node = getRoot();
        parent = nullptr;
      }
    } else {
      RBNode* sibling = getLeft(parent);

      if (sibling->color == Color::RED) {
        sibling->color = Color::BLACK;
        parent->color = Color::RED;
        rightRotate(parent);
        sibling = getLeft(parent);
      }

      RBNode* sibLeft = getLeft(sibling);
      RBNode* sibRight = getRight(sibling);
      bool leftBlack = (sibLeft == nullptr || sibLeft->color == Color::BLACK);
      bool rightBlack = (sibRight == nullptr || sibRight->color == Color::BLACK);

      if (leftBlack && rightBlack) {
        sibling->color = Color::RED;
        node = parent;
        parent = getParent(node);
      } else {
        if (leftBlack) {
          if (sibRight != nullptr) sibRight->color = Color::BLACK;
          sibling->color = Color::RED;
          leftRotate(sibling);
          sibling = getLeft(parent);
        }
        sibling->color = parent->color;
        parent->color = Color::BLACK;
        RBNode* sl = getLeft(sibling);
        if (sl != nullptr) sl->color = Color::BLACK;
        rightRotate(parent);
        node = getRoot();
        parent = nullptr;
      }
    }
  }

  if (node != nullptr) node->color = Color::BLACK;
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
void RedBlackTree<Key, Data>::remove(Key key) {
  if (this->root == nullptr) throw std::logic_error("Root is NULL");

  Node** current = &(this->root);

  while (*current != nullptr && (*current)->key != key) {
    if ((*current)->key > key) current = &((*current)->left);
    else if ((*current)->key < key) current = &((*current)->right);
  }
  if (*current == nullptr) throw std::out_of_range("Key does not exist");

  // Случай, когда два потомка
  if ((*current)->left != nullptr && (*current)->right != nullptr) {
    Node* successor = getSuccessor(*current);
    Node* parent = successor->parent;
    Node* son = successor->right;

    if (successor != (*current)->right) parent->left = son;
    else (*current)->right = son;

    if (son != nullptr) son->parent = parent;

    (*current)->key = successor->key;
    (*current)->data = successor->data;

    bool wasBlack = (static_cast<RBNode*>(successor)->color == Color::BLACK);
    delete successor;

    if (wasBlack) fixRemove(static_cast<RBNode*>(son), static_cast<RBNode*>(parent));
  }

  // Случай, когда нет потомков или один потомок
  else {
    Node* deletedNode = *current;
    Node* parent = deletedNode->parent;
    Node* son = (deletedNode->left != nullptr) ? deletedNode->left : deletedNode->right;

    if (son != nullptr) son->parent = parent;
    *current = son; // *current — это либо this->root, либо parent->left/right, в зависимости от пути поиска

    bool wasBlack = (static_cast<RBNode*>(deletedNode)->color == Color::BLACK);
    delete deletedNode;

    if (wasBlack) fixRemove(static_cast<RBNode*>(son), static_cast<RBNode*>(parent));
  }
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
