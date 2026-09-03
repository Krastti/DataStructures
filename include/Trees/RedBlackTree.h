#ifndef DATASTRUCTURES_RBTREE_H
#define DATASTRUCTURES_RBTREE_H

#include "BinaryTree.h"

template<typename Key, typename Data>
class RedBlackTree : public BinaryTree<Key, Data> {
private:
    enum class Color { RED, BLACK };

    struct RBNode : public BinaryTree<Key, Data>::Node {
        using Node = BinaryTree<Key, Data>::Node;

        Color color;
        RBNode(const Key &key, const Data &data, const Color &color) : Node(key, data), color(color) {}
    };

    using Node = RBNode;

    Node* root;

protected:
    Node* left();
    Node* right() const;
    Node* parent() const;

public:
    RedBlackTree();

    ~RedBlackTree();
};

#include "RedBlackTree.tpp"

#endif //DATASTRUCTURES_RBTREE_H
