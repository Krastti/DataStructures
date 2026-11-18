#ifndef DATASTRUCTURES_EXPRESSION_TREE_H
#define DATASTRUCTURES_EXPRESSION_TREE_H

#include "../../include/SmartPointers/UniquePtr.h"
#include "../../include/Trees/BinaryTreeBase.h"
#include "expressions.h"

#include <cstddef>

namespace ast {

class ExpressionTree : private BinaryTreeBase<Expr> {
public:
  ExpressionTree() = default;
  ExpressionTree(ExpressionTree&&) noexcept = default;
  ExpressionTree& operator=(ExpressionTree&&) noexcept = default;

  static ExpressionTree makeLeaf(UniquePtr<Expr> leaf);
  static ExpressionTree makeBinary(TokenType operation, ExpressionTree&& left, ExpressionTree&& right);

  // Указатель остаётся действительным, пока узел принадлежит живому дереву.
  const Expr* getRoot() const noexcept;
  [[nodiscard]] std::size_t size() const noexcept;
};

}

#endif //DATASTRUCTURES_EXPRESSION_TREE_H
