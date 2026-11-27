#ifndef DATASTRUCTURES_RBTREE_H
#define DATASTRUCTURES_RBTREE_H

#include "BinaryTree.h"

template<typename Key, typename Data>
class RedBlackTree : protected BinaryTree<Key, Data> {
private:
    using Base = BinaryTree<Key, Data>;
    enum class Color { RED, BLACK };

    struct RBNode : public BinaryTree<Key, Data>::Node {
        using Node = BinaryTree<Key, Data>::Node;

        Color color;
        RBNode(const Key &key, const Data &data, const Color &color) : Node(key, data), color(color) {}

        ~RBNode() override = default;
    };

    using Node = BinaryTree<Key, Data>::Node;

    static RBNode* getLeft(Node* node);
    static RBNode* getRight(Node* node);
    static RBNode* getParent(Node* node);
    RBNode* getRoot() const;

    void leftRotate(RBNode* node);
    void rightRotate(RBNode* node);

    void fixInsert(RBNode* current);
    void fixRemove(RBNode *node, RBNode *parent);

    int getColor(Key key) const;

public:
    using Base::at;
    using Base::containsKey;
    using Base::entries;
    using Base::get_root;
    using Base::keys;
    using Base::min;
    using Base::max;
    using Base::print;
    using Base::range;
    using Base::replace;
    using Base::size;

    // Порождающие
    RedBlackTree() = default;
    explicit RedBlackTree(Key key);

    // Копирующие
    RedBlackTree(const RedBlackTree& other);
    RedBlackTree& operator=(const RedBlackTree& other);

    // Перемещающие
    RedBlackTree(RedBlackTree&& other) noexcept = default;
    RedBlackTree& operator=(RedBlackTree&& other) noexcept = default;

    void insert(Key key, Data data) override;
    void remove(Key key) override;

    bool isRed(Key key) const;
    bool isBlack(Key key) const;
};

#include "RedBlackTree.tpp"

template <typename Key, typename Data>
using Map = RedBlackTree<Key, Data>;

#endif //DATASTRUCTURES_RBTREE_H
