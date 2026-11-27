#ifndef DATASTRUCTURES_AST_EXPRESSIONS_H
#define DATASTRUCTURES_AST_EXPRESSIONS_H

#include "Array/Array.h"
#include "tokenType.h"

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
  [[nodiscard]] const Expr* leftChild() const noexcept { return left; }
  [[nodiscard]] const Expr* rightChild() const noexcept { return right; }

private:
  friend class ::BinaryTreeBase<Expr>;
  friend class ExpressionTree;

  Expr* left = nullptr;
  Expr* right = nullptr;
  Expr* parent = nullptr;
};

// Хранит запись числа, включая минус отрицательного литерала.
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

struct Call : Expr {
  std::string name;
  Array<ExpressionTree> arguments;

  Call(std::string name, Array<ExpressionTree> arguments);
  ~Call() override;
  void accept(ExprVisitor& visitor) const override;
};

struct Update : Expr {
  std::string variable;
  TokenType operation;
  bool postfix;

  Update(std::string variable, TokenType operation, bool postfix);
  void accept(ExprVisitor& visitor) const override;
};

struct Binary : Expr {
  TokenType operation;

  [[nodiscard]] const Expr* leftOperand() const noexcept { return leftChild(); }
  [[nodiscard]] const Expr* rightOperand() const noexcept { return rightChild(); }
  void accept(ExprVisitor& visitor) const override;

private:
  friend class ExpressionTree;
  explicit Binary(TokenType operation);
};

}

#endif //DATASTRUCTURES_AST_EXPRESSIONS_H
