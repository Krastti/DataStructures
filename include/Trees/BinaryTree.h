#ifndef DATASTRUCTURES_BST_H
#define DATASTRUCTURES_BST_H

#include "../Stack/stack.h"
#include <iostream>

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

private:

  Node* getSuccessor(Node* node);
  //  Node* get_predecessor(Node* node);

public:
  BinaryTree();
  explicit BinaryTree(Key key, Data data);

  virtual void insert(Key key, Data data);
  virtual void remove(Key key);
  void replace(Key key, Data data);

  Data get_root() const;
  Data get(Key key) const;

  Data min() const;
  Data max() const;

  void print() const;

  ~BinaryTree();
};

#include "BinaryTree.tpp"

#endif // DATASTRUCTURES_BST_H