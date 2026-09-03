#ifndef DATASTRUCTURES_RBTREE_H
#define DATASTRUCTURES_RBTREE_H

#include "BinaryTree.h"

template<typename Key, typename Data>
class RedBlackTree : public BinaryTree<Key, Data> {
private:
    enum class Color { RED, BLACK };

    struct RBNode : public BinaryTree<Key, Data>::Node {
        using Node = typename BinaryTree<Key, Data>::Node;

        Color color;
        RBNode(const Key &key, const Data &data, const Color &color) : Node(key, data), color(color) {}

        ~RBNode() override = default;
    };

    using Node = typename BinaryTree<Key, Data>::Node;

    static RBNode* getLeft(Node* node);
    static RBNode* getRight(Node* node);
    static RBNode* getParent(Node* node);
    RBNode* getRoot() const;

    void leftRotate(RBNode* node);
    void rightRotate(RBNode* node);

    void fixInsert(RBNode* current);

public:
    void insert(Key key, Data data) override;
    // void remove(Key key) override;
};

#include "RedBlackTree.tpp"

template <typename Key, typename Data>
using Map = RedBlackTree<Key, Data>;

#endif //DATASTRUCTURES_RBTREE_H
