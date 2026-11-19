#ifndef DATASTRUCTURES_AST_EXPRESSIONS_H
#define DATASTRUCTURES_AST_EXPRESSIONS_H

#include "token.h"

#include <string>

template<typename Node> class BinaryTreeBase;

namespace ast {

struct ExprVisitor;
class ExpressionTree;

// Accept выбирает visit по типу узла. Обход дочерних узлов задаёт сам посетитель.
struct Expr {
  Expr() = default;
  Expr(const Expr&) = delete;
  Expr& operator=(const Expr&) = delete;
  virtual ~Expr() = default;
  virtual void accept(ExprVisitor& visitor) const = 0;

protected:
  const Expr* leftChild() const noexcept { return left; }
  const Expr* rightChild() const noexcept { return right; }

private:
  friend class ::BinaryTreeBase<Expr>;
  friend class ExpressionTree;

  Expr* left = nullptr;
  Expr* right = nullptr;
  Expr* parent = nullptr;
};

struct NumberLiteral : Expr {
  std::string lexeme;

  explicit NumberLiteral(std::string lexeme);
  void accept(ExprVisitor& visitor) const override;
};

struct Identifier : Expr {
  std::string name;

  explicit Identifier(std::string name);
  void accept(ExprVisitor& visitor) const override;
};

// Храним строку вместе с кавычками, как её прочитал лексер.
struct StringLiteral : Expr {
  std::string lexeme;

  explicit StringLiteral(std::string lexeme);
  void accept(ExprVisitor& visitor) const override;
};

struct Binary : Expr {
  TokenType operation;

  const Expr* leftOperand() const noexcept { return leftChild(); }
  const Expr* rightOperand() const noexcept { return rightChild(); }
  void accept(ExprVisitor& visitor) const override;

private:
  friend class ExpressionTree;
  explicit Binary(TokenType operation);
};

}

#endif //DATASTRUCTURES_AST_EXPRESSIONS_H
