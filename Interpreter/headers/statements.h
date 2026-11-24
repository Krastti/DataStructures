#ifndef DATASTRUCTURES_AST_STATEMENTS_H
#define DATASTRUCTURES_AST_STATEMENTS_H

#include "Array/Array.h"
#include "UniquePtr.h"
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

struct When : Statement {
  ExpressionTree condition;
  Array<UniquePtr<Statement>> thenBranch;
  Array<UniquePtr<Statement>> otherwiseBranch;

  explicit When(ExpressionTree condition, Array<UniquePtr<Statement>> thenBranch,
    Array<UniquePtr<Statement>> otherwiseBranch);
  void accept(StatementVisitor& visitor) const override;
};

struct RepeatWhile : Statement {
  ExpressionTree condition;
  Array<UniquePtr<Statement>> body;

  explicit RepeatWhile(ExpressionTree condition, Array<UniquePtr<Statement>> body);
  void accept(StatementVisitor& visitor) const override;
};

}

#endif //DATASTRUCTURES_AST_STATEMENTS_H
