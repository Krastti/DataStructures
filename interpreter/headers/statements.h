#ifndef DATASTRUCTURES_AST_STATEMENTS_H
#define DATASTRUCTURES_AST_STATEMENTS_H

#include "expressions.h"

#include <string>

namespace ast {

struct StatementVisitor;

struct Statement {
  virtual ~Statement() = default;
  virtual void accept(StatementVisitor& visitor) const = 0;
};

struct Remember : Statement {
  std::string variable;
  UniquePtr<Expr> right;

  Remember(std::string variable, UniquePtr<Expr> right);
  void accept(StatementVisitor& visitor) const override;
};

struct Print : Statement {
  UniquePtr<Expr> forOutput;

  explicit Print(UniquePtr<Expr> forOutput);
  void accept(StatementVisitor& visitor) const override;
};

}

#endif //DATASTRUCTURES_AST_STATEMENTS_H
