#ifndef DATASTRUCTURES_BST_H
#define DATASTRUCTURES_BST_H

template<typename Key, typename Data>
class BinaryTree {
private:
  struct Node {
    Key key;
    Data data;
    Node *left, *right;

    explicit Node(Key key, Data data, Node* left=nullptr, Node* right=nullptr) : key(key), data(data) {
      this->left = left;
      this->right = right;
    }
  };

  Node *root;
public:
  BinaryTree();
  explicit BinaryTree(Key key, Data data);

  // Модификации
  void insert(Key key, Data data);
  // void remove(Key key);
  // void replace(Key key, Data data);

  // Геттеры
  Data& get_root() const;
  Data& get(Key key) const;

  ~BinaryTree();
};

#include "BinaryTree.tpp"

#endif // DATASTRUCTURES_BST_H