#include <iostream>
#include "BinaryTree.h"
using namespace std;

int main() {
  BinaryTree tree(5, 10);

  tree.insert(6, 12);

  cout << tree.get_root() << endl;
  cout << tree.get(6) << endl;

  return 0;
}