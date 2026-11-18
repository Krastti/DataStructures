#ifndef DATASTRUCTURES_AST_STATEMENTS_H
#define DATASTRUCTURES_AST_STATEMENTS_H

#include "expressionTree.h"

#include <string>

namespace ast {

struct StatementVisitor;

struct Statement {
  virtual ~Statement() = default;
  virtual void accept(StatementVisitor& visitor) const = 0;
};

struct Remember : Statement {
  std::string variable;
  ExpressionTree right;

  Remember(std::string variable, ExpressionTree right);
  void accept(StatementVisitor& visitor) const override;
};

struct Print : Statement {
  ExpressionTree forOutput;

  explicit Print(ExpressionTree forOutput);
  void accept(StatementVisitor& visitor) const override;
};

}

#endif //DATASTRUCTURES_AST_STATEMENTS_H
