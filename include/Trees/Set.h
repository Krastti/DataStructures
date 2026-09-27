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
    Node* parent;

    explicit Node(Key key, Node* parent = nullptr) : key(std::move(key)), parent(parent) {}
  };

  UniquePtr<Node> root;

public:
  Set();
  explicit Set(Key key);

  void insert(Key key);
  void remove(const Key& key);
  [[nodiscard]] bool contains(const Key& key) const;

  ~Set() = default;
};

#include "Set.tpp"

#endif // DATASTRUCTURES_SET_H
