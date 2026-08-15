#include <iostream>
#include "include/Trees/BinaryTree.h"
#include "include/Stack/stack.h"

int main() {
    BinaryTree tree(6, 10);
    tree.insert(3, 6);
    tree.insert(2, 2);
    tree.insert(4, 4);
    tree.insert(8, 8);
    tree.insert(13, 13);
    tree.insert(9, 9);

    tree.print();

    return 0;
}
