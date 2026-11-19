#ifndef DATASTRUCTURES_BST_H
#define DATASTRUCTURES_BST_H

#include "BinaryTreeBase.h"
#include "../Stack/stack.h"
#include "../Array/Array.h"
#include <cstddef>
#include <utility>

namespace tree_detail {

template<typename Key, typename Data>
struct SearchNode {
  Key key;
  Data data;
  SearchNode* left;
  SearchNode* right;
  SearchNode* parent;

  explicit SearchNode(Key key, Data data, SearchNode* left = nullptr,
                      SearchNode* right = nullptr, SearchNode* parent = nullptr)
      : key(key), data(data), left(left), right(right), parent(parent) {}

  virtual ~SearchNode() = default;
};

}

template<typename Key, typename Data>
class BinaryTree : protected BinaryTreeBase<tree_detail::SearchNode<Key, Data>> {
protected:
  using Node = tree_detail::SearchNode<Key, Data>;
  using Base = BinaryTreeBase<Node>;
  using Base::root;
  using Base::nodeCount;
  using Base::clear;

  void transplant(Node* oldNode, Node* newNode) noexcept;

  Node* getSuccessor(Node* node);
  Node* findNode(const Key& key);
  const Node* findNode(const Key& key) const;

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
