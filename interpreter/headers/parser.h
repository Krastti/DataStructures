#ifndef DATASTRUCTURES_PARSER_H
#define DATASTRUCTURES_PARSER_H

#include "../../include/Array/Array.h"
#include "../../include/SmartPointers/UniquePtr.h"
#include "statements.h"
#include "token.h"

#include <cstddef>
#include <initializer_list>

// Собирает дерево программы из токенов.
class Parser {
public:
  // В конце массива должен быть один EOF. При ошибке собранные узлы удалятся через UniquePtr.
  Array<UniquePtr<ast::Statement>> parse(const Array<Token>& tokens);

private:
  // nextPosition — индекс следующего непрочитанного токена.
  struct ParsedExpr {
    ast::ExpressionTree expression;
    std::size_t nextPosition;
  };

  struct ParsedStatement {
    UniquePtr<ast::Statement> statement;
    std::size_t nextPosition;
  };

  // Функция, которая разбирает операнд на следующем уровне приоритета.
  using ExpressionParser = ParsedExpr (*)(const Array<Token>&, std::size_t);

  static const Token& requireToken(const Array<Token>& tokens, std::size_t position,
                                  TokenType expected, const char* message);
  static void requireStatementEnd(const Array<Token>& tokens, std::size_t position);
  static ParsedStatement parseStatement(const Array<Token>& tokens, std::size_t position);

  static ast::ExpressionTree parseOperand(const Array<Token>& tokens, std::size_t position);
  static ParsedExpr parsePrimary(const Array<Token>& tokens, std::size_t position);

  // Сначала разбираем операции с более высоким приоритетом.
  static ParsedExpr parseMultiplication(const Array<Token>& tokens, std::size_t position);
  static ParsedExpr parseAddition(const Array<Token>& tokens, std::size_t position);
  static ParsedExpr parseComparison(const Array<Token>& tokens, std::size_t position);
  static ParsedExpr parseEquality(const Array<Token>& tokens, std::size_t position);
  static ParsedExpr parseBinaryChain(const Array<Token>& tokens, std::size_t position,
                                    ExpressionParser parseOperandAtNextLevel,
                                    std::initializer_list<TokenType> operations);
};

#endif //DATASTRUCTURES_PARSER_H
