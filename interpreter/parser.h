#ifndef DATASTRUCTURES_PARSER_H
#define DATASTRUCTURES_PARSER_H

#include "../include/Array/Array.h"
#include "../include/SmartPointers/UniquePtr.h"
#include "token.h"

#include <cstddef>
#include <string>

class Parser {
public:
  struct Expr {
    virtual ~Expr() = default;
  };

  struct Statement {
    virtual ~Statement() = default;
  };

  struct NumberLiteral : Expr {
    std::string lexeme;

    explicit NumberLiteral(std::string lexeme);
  };

  struct Identifier : Expr {
    std::string name;

    explicit Identifier(std::string name);
  };

  struct Binary : Expr {
    TokenType operation;
    UniquePtr<Expr> left;
    UniquePtr<Expr> right;

    Binary(TokenType operation, UniquePtr<Expr> left, UniquePtr<Expr> right);
  };

  struct Remember : Statement {
    std::string variable;
    UniquePtr<Expr> right;

    Remember(std::string variable, UniquePtr<Expr> right);
  };

  struct Print : Statement {
    UniquePtr<Expr> forOutput;

    explicit Print(UniquePtr<Expr> forOutput);
  };

  Array<UniquePtr<Statement>> parse(const Array<Token>& tokens);

private:
  struct ParsedExpr {
    UniquePtr<Expr> expression;
    std::size_t nextPosition;
  };

  const Token& requireToken(const Array<Token>& tokens, std::size_t position,
                            TokenType expected, const std::string& message);
  UniquePtr<Expr> parseOperand(const Array<Token>& tokens, std::size_t position);
  ParsedExpr parsePrimary(const Array<Token>& tokens, std::size_t position);
  ParsedExpr parseMultiplication(const Array<Token>& tokens, std::size_t position);
  ParsedExpr parseAddition(const Array<Token>& tokens, std::size_t position);
  ParsedExpr parseComparison(const Array<Token>& tokens, std::size_t position);
  ParsedExpr parseEquality(const Array<Token>& tokens, std::size_t position);
};

#endif //DATASTRUCTURES_PARSER_H
