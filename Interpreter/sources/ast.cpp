#include "../headers/expressions.h"
#include "../headers/statements.h"
#include "../headers/visitors.h"

#include <utility>

namespace ast {

NumberLiteral::NumberLiteral(std::string lexeme) : lexeme(std::move(lexeme)) {}

StringLiteral::StringLiteral(std::string lexeme) : lexeme(std::move(lexeme)) {}

Identifier::Identifier(std::string name) : name(std::move(name)) {}

Binary::Binary(TokenType operation) : operation(operation) {}

Remember::Remember(std::string variable, ExpressionTree right)
    : variable(std::move(variable)), right(std::move(right)) {}

Print::Print(ExpressionTree forOutput) : forOutput(std::move(forOutput)) {}

void NumberLiteral::accept(ExprVisitor& visitor) const {
  visitor.visit(*this);
}

void Identifier::accept(ExprVisitor& visitor) const {
  visitor.visit(*this);
}

void StringLiteral::accept(ExprVisitor& visitor) const {
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

}
