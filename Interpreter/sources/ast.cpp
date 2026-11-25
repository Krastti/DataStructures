#include "../headers/expressionTree.h"
#include "../headers/expressions.h"
#include "../headers/statements.h"
#include "../headers/visitors.h"

#include <utility>

namespace ast {

NumberLiteral::NumberLiteral(std::string lexeme) : lexeme(std::move(lexeme)) {}

StringLiteral::StringLiteral(std::string lexeme) : lexeme(std::move(lexeme)) {}

Identifier::Identifier(std::string name) : name(std::move(name)) {}

Call::Call(std::string name, Array<ExpressionTree> arguments)
    : name(std::move(name)), arguments(std::move(arguments)) {}

Call::~Call() = default;

Binary::Binary(const TokenType operation) : operation(operation) {}

Remember::Remember(std::string variable, ExpressionTree right)
    : variable(std::move(variable)), right(std::move(right)) {}

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

Function::Function(std::string name, Array<std::string> parameters, ExpressionTree result)
    : name(std::move(name)), parameters(std::move(parameters)), result(std::move(result)) { }

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

void Remember::accept(StatementVisitor& visitor) const {
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
