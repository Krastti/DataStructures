#include "../headers/expressionTree.h"

#include <stdexcept>
#include <utility>

namespace ast {

ExpressionTree ExpressionTree::makeLeaf(UniquePtr<Expr> leaf) {
  if (!leaf) throw std::invalid_argument("Ожидался лист дерева выражения");

  ExpressionTree tree;
  tree.root = leaf.release();
  tree.nodeCount = 1;
  return tree;
}

ExpressionTree ExpressionTree::makeBinary(TokenType operation, ExpressionTree&& left, ExpressionTree&& right) {
  if (&left == &right || left.root == nullptr || right.root == nullptr) {
    throw std::invalid_argument("Ожидались два отдельных непустых дерева выражений");
  }

  // Сначала создаём узел операции. Если выделение памяти не удастся, обе ветви останутся у вызывающего кода.
  ExpressionTree tree;
  auto* node = new Binary(operation);
  node->left = std::exchange(left.root, nullptr);
  node->right = std::exchange(right.root, nullptr);
  node->left->parent = node;
  node->right->parent = node;

  tree.root = node;
  tree.nodeCount = left.nodeCount + right.nodeCount + 1;
  left.nodeCount = 0;
  right.nodeCount = 0;
  return tree;
}

const Expr* ExpressionTree::getRoot() const noexcept {
  return root;
}

std::size_t ExpressionTree::size() const noexcept {
  return nodeCount;
}

}
