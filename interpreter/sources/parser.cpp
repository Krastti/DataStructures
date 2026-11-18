#include "../headers/parser.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

Array<UniquePtr<ast::Statement>> Parser::parse(const Array<Token>& tokens) {
  if (tokens.size() == 0 || tokens[tokens.size() - 1].type != TokenType::EndOfFile) {
    throw std::invalid_argument("Ожидался токен конца файла");
  }

  Array<UniquePtr<ast::Statement>> statements;
  std::size_t position = 0;
  while (position < tokens.size()) {
    const TokenType type = tokens[position].type;
    if (type == TokenType::Newline) {
      ++position;
      continue;
    }
    if (type == TokenType::EndOfFile) {
      if (position != tokens.size() - 1) {
        throw std::invalid_argument("Лишние токены после конца файла");
      }
      return statements;
    }

    auto parsedStatement = parseStatement(tokens, position);
    requireStatementEnd(tokens, parsedStatement.nextPosition);
    statements.push_back(std::move(parsedStatement.statement));
    position = parsedStatement.nextPosition;
  }
  throw std::invalid_argument("Ожидался токен конца файла");
}

Parser::ParsedStatement Parser::parseStatement(const Array<Token>& tokens, std::size_t position) {
  switch (tokens[position].type) {
    case TokenType::Remember: {
      const Token& name = requireToken(tokens, position + 1, TokenType::Identifier,
                                      "Ожидалось имя переменной после remember");
      requireToken(tokens, position + 2, TokenType::Equals,
                   "Ожидался знак = после имени переменной");
      auto value = parseEquality(tokens, position + 3);
      return {UniquePtr<ast::Statement>(new ast::Remember(name.lexeme, std::move(value.expression))),
              value.nextPosition};
    }
    case TokenType::Print: {
      auto value = parseEquality(tokens, position + 1);
      return {UniquePtr<ast::Statement>(new ast::Print(std::move(value.expression))), value.nextPosition};
    }
    default:
      throw std::invalid_argument("Неожиданный токен в начале инструкции");
  }
}

void Parser::requireStatementEnd(const Array<Token>& tokens, std::size_t position) {
  if (position < tokens.size()) {
    const TokenType type = tokens[position].type;
    if (type == TokenType::Newline || type == TokenType::EndOfFile) return;
  }
  throw std::invalid_argument("Ожидался перевод строки или конец файла");
}

const Token& Parser::requireToken(const Array<Token>& tokens, std::size_t position,
                                 TokenType expected, const char* message) {
  if (position >= tokens.size() || tokens[position].type != expected) {
    throw std::invalid_argument(message);
  }
  return tokens[position];
}

ast::ExpressionTree Parser::parseOperand(const Array<Token>& tokens, std::size_t position) {
  if (position >= tokens.size()) {
    throw std::invalid_argument("Ожидалось число, строка или имя, получен конец ввода");
  }
  const Token& token = tokens[position];
  switch (token.type) {
    case TokenType::Number: return ast::ExpressionTree::makeLeaf(UniquePtr<ast::Expr>(new ast::NumberLiteral(token.lexeme)));
    case TokenType::String: return ast::ExpressionTree::makeLeaf(UniquePtr<ast::Expr>(new ast::StringLiteral(token.lexeme)));
    case TokenType::Identifier: return ast::ExpressionTree::makeLeaf(UniquePtr<ast::Expr>(new ast::Identifier(token.lexeme)));
    default: throw std::invalid_argument("Ожидалось число, строка или имя");
  }
}

Parser::ParsedExpr Parser::parsePrimary(const Array<Token>& tokens, std::size_t position) {
  if (position < tokens.size() && tokens[position].type == TokenType::LeftParen) {
    auto inner = parseEquality(tokens, position + 1);
    requireToken(tokens, inner.nextPosition, TokenType::RightParen, "Ожидалась закрывающая скобка");
    // Пропускаем закрывающую скобку. Отдельный узел для скобок не нужен.
    ++inner.nextPosition;
    return inner;
  }
  return {parseOperand(tokens, position), position + 1};
}

Parser::ParsedExpr Parser::parseMultiplication(const Array<Token>& tokens, std::size_t position) {
  return parseBinaryChain(tokens, position, parsePrimary, {TokenType::Star, TokenType::Slash});
}

Parser::ParsedExpr Parser::parseAddition(const Array<Token>& tokens, std::size_t position) {
  return parseBinaryChain(tokens, position, parseMultiplication, {TokenType::Plus, TokenType::Minus});
}

Parser::ParsedExpr Parser::parseComparison(const Array<Token>& tokens, std::size_t position) {
  return parseBinaryChain(tokens, position, parseAddition,
                          {TokenType::Less, TokenType::LessEqual,
                           TokenType::Greater, TokenType::GreaterEqual});
}

Parser::ParsedExpr Parser::parseEquality(const Array<Token>& tokens, std::size_t position) {
  return parseBinaryChain(tokens, position, parseComparison,
                          {TokenType::EqualEqual, TokenType::BangEqual});
}

Parser::ParsedExpr Parser::parseBinaryChain(const Array<Token>& tokens, std::size_t position,
                                           ExpressionParser parseOperandAtNextLevel,
                                           std::initializer_list<TokenType> operations) {
  auto left = parseOperandAtNextLevel(tokens, position);
  while (left.nextPosition < tokens.size()) {
    const TokenType operation = tokens[left.nextPosition].type;
    if (std::find(operations.begin(), operations.end(), operation) == operations.end()) break;

    auto right = parseOperandAtNextLevel(tokens, left.nextPosition + 1);
    // Собираем слева направо: a - b - c превращается в (a - b) - c.
    left.expression = ast::ExpressionTree::makeBinary(
        operation, std::move(left.expression), std::move(right.expression));
    left.nextPosition = right.nextPosition;
  }
  return left;
}
