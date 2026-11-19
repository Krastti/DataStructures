#ifndef DATASTRUCTURES_EXPRESSION_TREE_H
#define DATASTRUCTURES_EXPRESSION_TREE_H

#include "UniquePtr.h"
#include "Trees/BinaryTreeBase.h"
#include "expressions.h"

#include <cstddef>

namespace ast {

class ExpressionTree : private BinaryTreeBase<Expr> {
public:
  ExpressionTree() = default;
  ExpressionTree(ExpressionTree&&) noexcept = default;
  ExpressionTree& operator=(ExpressionTree&&) noexcept = default;

  // Забирает узел из UniquePtr и делает его единственным узлом дерева.
  static ExpressionTree makeLeaf(UniquePtr<Expr> leaf);

  // Создаёт узел операции и присоединяет к нему два непустых дерева как левую и правую ветви.
  // После успешного объединения оба исходных дерева пусты.
  static ExpressionTree makeBinary(TokenType operation, ExpressionTree&& left, ExpressionTree&& right);

  // Указатель остаётся действительным, пока узел принадлежит живому дереву.
  [[nodiscard]] const Expr* getRoot() const noexcept;
  [[nodiscard]] std::size_t size() const noexcept;
};

}

#endif //DATASTRUCTURES_EXPRESSION_TREE_H
