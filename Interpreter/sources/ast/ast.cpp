#include "ast/expressionTree.h"
#include "ast/expressions.h"
#include "ast/statements.h"
#include "ast/visitors.h"

#include <utility>

namespace ast {

NumberLiteral::NumberLiteral(std::string lexeme) : lexeme(std::move(lexeme)) {}

StringLiteral::StringLiteral(std::string lexeme) : lexeme(std::move(lexeme)) {}

Identifier::Identifier(std::string name) : name(std::move(name)) {}

Call::Call(std::string name, Array<ExpressionTree> arguments)
    : name(std::move(name)), arguments(std::move(arguments)) {}

Call::~Call() = default;

Binary::Binary(const TokenType operation) : operation(operation) {}

Update::Update(std::string variable, const TokenType operation, const bool postfix)
    : variable(std::move(variable)), operation(operation), postfix(postfix) {}

Remember::Remember(std::string variable, ExpressionTree right, const TokenType operation)
    : variable(std::move(variable)), right(std::move(right)), operation(operation) {}

UpdateStatement::UpdateStatement(ExpressionTree expression) : expression(std::move(expression)) {}

Print::Print(ExpressionTree forOutput) : forOutput(std::move(forOutput)) {}

When::When(ExpressionTree condition, Array<UniquePtr<Statement>> thenBranch,
           Array<UniquePtr<Statement>> otherwiseBranch)
    : condition(std::move(condition)),
      thenBranch(std::move(thenBranch)),
      otherwiseBranch(std::move(otherwiseBranch)) {}

RepeatWhile::RepeatWhile(ExpressionTree condition, Array<UniquePtr<Statement>> body)
    : condition(std::move(condition)), body(std::move(body)) {}

For::For(std::string variable, ExpressionTree stop, Array<UniquePtr<Statement>> body)
    : variable(std::move(variable)), stop(std::move(stop)), body(std::move(body)) {}

Function::Function(std::string name, Array<std::string> parameters, ExpressionTree result,
                   Array<UniquePtr<Statement>> body)
    : name(std::move(name)), parameters(std::move(parameters)), body(std::move(body)),
      result(std::move(result)) {}

void NumberLiteral::accept(ExprVisitor& visitor) const {
  visitor.visit(*this);
}

void Identifier::accept(ExprVisitor& visitor) const {
  visitor.visit(*this);
}

void StringLiteral::accept(ExprVisitor& visitor) const {
  visitor.visit(*this);
}

void Call::accept(ExprVisitor& visitor) const {
  visitor.visit(*this);
}

void Binary::accept(ExprVisitor& visitor) const {
  visitor.visit(*this);
}

void Update::accept(ExprVisitor& visitor) const {
  visitor.visit(*this);
}

void Remember::accept(StatementVisitor& visitor) const {
  visitor.visit(*this);
}

void UpdateStatement::accept(StatementVisitor& visitor) const {
  visitor.visit(*this);
}

void Print::accept(StatementVisitor& visitor) const {
  visitor.visit(*this);
}

void When::accept(StatementVisitor& visitor) const {
  visitor.visit(*this);
}

void RepeatWhile::accept(StatementVisitor& visitor) const {
  visitor.visit(*this);
}

void For::accept(StatementVisitor& visitor) const {
  visitor.visit(*this);
}

void Function::accept(StatementVisitor &visitor) const {
  visitor.visit(*this);
}
}
