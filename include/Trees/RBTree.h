#ifndef DATASTRUCTURES_RBTREE_H
#define DATASTRUCTURES_RBTREE_H

#include "BinaryTree.h"

template<typename Key, typename Data>
class RBTree : public BinaryTree<Key, Data> {
private:
    enum class Color { RED, BLACK };

protected:
    struct RBNode : public BinaryTree<Key, Data>::Node {
        Color color;
        RBNode(const Key &key, const Data &data) : key(key), data(data) {}
    };

public:
    RBTree();

    ~RBTree();
};

#include "RBTree.tpp"

#endif //DATASTRUCTURES_RBTREE_H
