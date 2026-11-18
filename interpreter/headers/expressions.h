#ifndef DATASTRUCTURES_AST_EXPRESSIONS_H
#define DATASTRUCTURES_AST_EXPRESSIONS_H

#include "../../include/SmartPointers/UniquePtr.h"
#include "token.h"

#include <string>

namespace ast {

struct ExprVisitor;

// accept выбирает visit по типу узла. Обход дочерних узлов задаёт сам посетитель.
struct Expr {
  virtual ~Expr() = default;
  virtual void accept(ExprVisitor& visitor) const = 0;
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

// Операнды принадлежат этому узлу. Передаём их через std::move.
struct Binary : Expr {
  TokenType operation;
  UniquePtr<Expr> left;
  UniquePtr<Expr> right;

  Binary(TokenType operation, UniquePtr<Expr> left, UniquePtr<Expr> right);
  void accept(ExprVisitor& visitor) const override;
};

}

#endif //DATASTRUCTURES_AST_EXPRESSIONS_H
