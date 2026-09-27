#ifndef DATASTRUCTURES_BST_H
#define DATASTRUCTURES_BST_H

#include "../Stack/stack.h"
#include "../Array/Array.h"
#include <cstddef>
#include <utility>

template<typename Key, typename Data>
class BinaryTree {
protected:
  struct Node {
    Key key;
    Data data;
    Node *left, *right;
    Node *parent;

    explicit Node(Key key, Data data, Node* left=nullptr, Node* right=nullptr, Node* parent=nullptr) : key(key), data(data) {
      this->left = left;
      this->right = right;
      this->parent = parent;
    }

    virtual ~Node() = default;
  };

  Node* root;
  size_t nodeCount;
  void clear() noexcept;

  Node* getSuccessor(Node* node);
  //  Node* get_predecessor(Node* node);

public:
  BinaryTree();
  explicit BinaryTree(Key key);
  explicit BinaryTree(Key key, Data data);
  BinaryTree(const BinaryTree&) = delete;
  BinaryTree& operator=(const BinaryTree&) = delete;
  BinaryTree(BinaryTree&& other) noexcept;
  BinaryTree& operator=(BinaryTree&& other) noexcept;

  virtual void insert(Key key, Data data);
  virtual void remove(Key key);
  void replace(Key key, Data data);

  Data get_root() const;
  Data get(Key key) const;
  Data find(Key key) const;
  Data& at(const Key& key);
  const Data& at(const Key& key) const;
  [[nodiscard]] bool containsKey(const Key& key) const;
  [[nodiscard]] size_t size() const noexcept;

  Array<std::pair<Key, Data>> entries() const;
  Array<Key> keys() const;
  Array<std::pair<Key, Data>> range(const Key& first, const Key& last) const;

  Data min() const;
  Data max() const;

  void print() const;

  virtual ~BinaryTree();
};

#include "BinaryTree.tpp"

#endif // DATASTRUCTURES_BST_H
