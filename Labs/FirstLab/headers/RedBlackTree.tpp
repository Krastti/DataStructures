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

      // Случай 1: дядя красный. Перекрашиваем узлы и продолжаем от деда
      if (uncle != nullptr && uncle->color == Color::RED) {
        parent->color = Color::BLACK;
        uncle->color = Color::BLACK;
        grandParent->color = Color::RED;
        current = grandParent;
      } else {
        // Случай 2: дядя чёрный, зигзаг. Поворотом вокруг родителя получаем прямую
        if (current == getRight(parent)) {
          current = parent;
          leftRotate(current);
          parent = getParent(current);
        }
        // Случай 3: дядя чёрный, прямая. Перекрашиваем узлы и поворачиваем вокруг деда
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
      const bool leftBlack = (sibLeft == nullptr || sibLeft->color == Color::BLACK);
      const bool rightBlack = (sibRight == nullptr || sibRight->color == Color::BLACK);

      if (leftBlack && rightBlack) {
        // Случай 2: у брата оба ребёнка чёрные
        sibling->color = Color::RED;
        node = parent;
        parent = getParent(node);
      } else {
        if (rightBlack) {
          // Случай 3: ближний ребенок брата красный
          if (sibLeft != nullptr) sibLeft->color = Color::BLACK;
          sibling->color = Color::RED;
          rightRotate(sibling);
          sibling = getRight(parent);
        }
        // Случай 4: дальний ребёнок брата красный
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
      const bool leftBlack = (sibLeft == nullptr || sibLeft->color == Color::BLACK);
      const bool rightBlack = (sibRight == nullptr || sibRight->color == Color::BLACK);

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
int RedBlackTree<Key, Data>::getColor(Key key) const {
  if (this->root == nullptr) throw std::logic_error("Root is NULL");

  const Node* current = this->findNode(key);
  if (current == nullptr) throw std::out_of_range("Key does not exist");

  return static_cast<int>(static_cast<const RBNode*>(current)->color);
}

template <typename Key, typename Data>
RedBlackTree<Key, Data>::RedBlackTree(Key key) {
  this->root = new RBNode(key, Data(), Color::BLACK);
  this->nodeCount = 1;
}

template <typename Key, typename Data>
RedBlackTree<Key, Data>::RedBlackTree(const RedBlackTree& other) : Base() {
  this->copyNodes(other.root, [](const Node& source) {
    const auto& node = static_cast<const RBNode&>(source);
    return new RBNode(node.key, node.data, node.color);
  });
}

template <typename Key, typename Data>
RedBlackTree<Key, Data>& RedBlackTree<Key, Data>::operator=(const RedBlackTree& other) {
  if (this != &other) {
    RedBlackTree copy(other);
    *this = std::move(copy);
  }
  return *this;
}

template <typename Key, typename Data>
void RedBlackTree<Key, Data>::insert(Key key, Data data) {
  if (this->root == nullptr) {
    this->root = new RBNode(key, data, Color::BLACK);
    this->nodeCount = 1;
    return;
  }

  Node** slot = &this->root;
  Node* parent = nullptr;

  while (*slot != nullptr) {
    parent = *slot;

    if (parent->key > key) slot = &parent->left;
    else if (parent->key < key) slot = &parent->right;
    else return;
  }

  auto newNode = new RBNode(key, data, Color::RED);

  *slot = newNode;
  newNode->parent = parent;

  ++this->nodeCount;
  fixInsert(newNode);
}

template <typename Key, typename Data>
void RedBlackTree<Key, Data>::remove(Key key) {
  if (this->root == nullptr) throw std::logic_error("Root is NULL");

  auto deletedNode = static_cast<RBNode*>(this->findNode(key));
  if (deletedNode == nullptr) throw std::out_of_range("Key does not exist");
  bool wasBlack = deletedNode->color == Color::BLACK;
  Node* son = nullptr;
  Node* parent = deletedNode->parent;

  if (deletedNode->left == nullptr) {
    son = deletedNode->right;
    this->transplant(deletedNode, son);
  } else if (deletedNode->right == nullptr) {
    son = deletedNode->left;
    this->transplant(deletedNode, son);
  } else {
    auto successor = static_cast<RBNode*>(this->getSuccessor(deletedNode));
    wasBlack = successor->color == Color::BLACK;
    son = successor->right;
    if (successor == deletedNode->right) {
      parent = successor;
    } else {
      parent = successor->parent;
      this->transplant(successor, son);
      successor->right = deletedNode->right;
      successor->right->parent = successor;
    }
    // Переставляем узел без присваивания ключа и значения, чтобы исключение не повредило дерево.
    this->transplant(deletedNode, successor);
    successor->left = deletedNode->left;
    successor->left->parent = successor;
    successor->color = deletedNode->color;
    if (son != nullptr) son->parent = parent;
  }
  delete deletedNode;
  --this->nodeCount;
  if (wasBlack) fixRemove(static_cast<RBNode*>(son), static_cast<RBNode*>(parent));
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
