#ifndef DATASTRUCTURES_SET_H
#define DATASTRUCTURES_SET_H

#include "../SmartPointers/UniquePtr.h"

template <typename Key>
class Set {
protected:
  struct Node {
    Key key;
    UniquePtr<Node> left;
    UniquePtr<Node> right;
    UniquePtr<Node> parent;

    explicit Node(Key key, Node* left = nullptr, Node* right = nullptr, Node* parent = nullptr) : key(key) {
      this->left(left);
      this->right(right);
      this->parent(parent);
    }
  };

  UniquePtr<Node> root;

public:
  Set();
  explicit Set(Key key);

  void insert(Key key);
  void remove(Key key);

  ~Set() = default;
};

#include "Set.tpp"

#endif // DATASTRUCTURES_SET_H
