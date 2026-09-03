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

        ~RBNode() override = default;
    };

    using Node = RBNode;

protected:
    Node* getLeft() const;
    Node* getRight() const;
    Node* getParent() const;
    Node* getRoot() const;

    void leftRotate(Node* node);
    void rightRotate(Node* node);

    void fixInsert(Node* current);

public:
    void insert(Key key, Data data) override;
    void remove(Key key) override;
};

#include "RedBlackTree.tpp"

#endif //DATASTRUCTURES_RBTREE_H
