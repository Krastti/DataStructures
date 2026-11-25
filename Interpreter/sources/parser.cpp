// ReSharper disable CppUseStructuredBinding
// ReSharper disable CppTooWideScopeInitStatement
#include "../headers/parser.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>

namespace {
bool isUpdateOperator(const TokenType type) {
  return type == TokenType::Increment || type == TokenType::Decrement;
}
}

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

    // ReSharper disable once CppLocalVariableMayBeConst
    auto parsedStatement = type == TokenType::Function
      ? parseFunction(tokens, position)
      : parseStatement(tokens, position);
    requireStatementEnd(tokens, parsedStatement.nextPosition);
    statements.push_back(std::move(parsedStatement.statement));
    position = parsedStatement.nextPosition;
  }
  throw std::invalid_argument("Ожидался токен конца файла");
}

Parser::ParsedStatement Parser::parseStatement(const Array<Token>& tokens, const std::size_t position) {
  switch (tokens[position].type) {
    case TokenType::Remember: {
      const Token& name = requireToken(tokens, position + 1, TokenType::Identifier,
                                      "Ожидалось имя переменной после remember");
      requireToken(tokens, position + 2, TokenType::Equals,
                   "Ожидался знак = после имени переменной");
      auto value = parseEquality(tokens, position + 3);

      return {.statement = UniquePtr<ast::Statement>(
        new ast::Remember(name.lexeme, std::move(value.expression))),
        .nextPosition = value.nextPosition
      };
    }
    case TokenType::Identifier:
    case TokenType::Increment:
    case TokenType::Decrement: {
      if (tokens[position].type == TokenType::Identifier) {
        const TokenType operation = tokens[position + 1].type;
        if (operation == TokenType::PlusEquals || operation == TokenType::MinusEquals) {
          auto value = parseEquality(tokens, position + 2);
          return {
            .statement = UniquePtr<ast::Statement>(
              new ast::Remember(tokens[position].lexeme, std::move(value.expression), operation)),
            .nextPosition = value.nextPosition
          };
        }
        if (!isUpdateOperator(operation)) {
          throw std::invalid_argument("Ожидался оператор изменения переменной после имени");
        }
      }
      auto update = parseUnary(tokens, position);
      return {
        .statement = UniquePtr<ast::Statement>(new ast::UpdateStatement(std::move(update.expression))),
        .nextPosition = update.nextPosition
      };
    }
    case TokenType::Print: {
      auto value = parseEquality(tokens, position + 1);
      return {.statement = UniquePtr<ast::Statement>(
        new ast::Print(std::move(value.expression))),
        .nextPosition = value.nextPosition
      };
    }
    case TokenType::When: {
      const TokenType firstConditionToken = tokens[position + 1].type;
      if (firstConditionToken == TokenType::Newline || firstConditionToken == TokenType::EndOfFile) {
        throw std::invalid_argument("Ожидалось условие после when");
      }

      auto condition = parseEquality(tokens, position + 1);
      requireToken(tokens, condition.nextPosition, TokenType::Newline,
                   "Ожидался перевод строки после условия when");

      auto thenBranch = parseBody(tokens, condition.nextPosition + 1,
                                  {TokenType::Otherwise, TokenType::End}, "when");
      std::size_t endPosition = thenBranch.nextPosition;

      Array<UniquePtr<ast::Statement>> otherwiseBranch;
      if (tokens[endPosition].type == TokenType::Otherwise) {
        requireToken(tokens, endPosition + 1, TokenType::Newline,
                     "Ожидался перевод строки после otherwise");

        auto branch = parseBody(tokens, endPosition + 2, {TokenType::End}, "when");
        otherwiseBranch = std::move(branch.statements);
        endPosition = branch.nextPosition;
      }

      requireToken(tokens, endPosition, TokenType::End, "Ожидался end для блока when");
      return {.statement = UniquePtr<ast::Statement>(
        new ast::When(std::move(condition.expression), std::move(thenBranch.statements),
                      std::move(otherwiseBranch))),
        .nextPosition = endPosition + 1
      };
    }
    case TokenType::Repeat: {
      requireToken(tokens, position + 1, TokenType::While, "Ожидался while после repeat");

      const TokenType firstConditionToken = tokens[position + 2].type;
      if (firstConditionToken == TokenType::Newline || firstConditionToken == TokenType::EndOfFile) {
        throw std::invalid_argument("Ожидалось условие после repeat while");
      }

      auto condition = parseEquality(tokens, position + 2);
      requireToken(tokens, condition.nextPosition, TokenType::Newline,
                   "Ожидался перевод строки после условия repeat while");

      auto body = parseBody(tokens, condition.nextPosition + 1, {TokenType::End}, "repeat while");
      return {.statement = UniquePtr<ast::Statement>(
        new ast::RepeatWhile(std::move(condition.expression), std::move(body.statements))),
        .nextPosition = body.nextPosition + 1
      };
    }
    case TokenType::For: {
      const Token& name = requireToken(tokens, position + 1, TokenType::Identifier,
                                      "Ожидалось имя переменной после for");
      requireToken(tokens, position + 2, TokenType::In, "Ожидалось in после имени переменной");
      requireToken(tokens, position + 3, TokenType::Range, "Ожидалось range после in");
      requireToken(tokens, position + 4, TokenType::LeftParen, "Ожидалась открывающая скобка после range");

      auto stop = parseEquality(tokens, position + 5);
      requireToken(tokens, stop.nextPosition, TokenType::RightParen, "Ожидалась закрывающая скобка после аргумента range");
      requireToken(tokens, stop.nextPosition + 1, TokenType::Newline,
                   "Ожидался перевод строки после заголовка for");

      auto body = parseBody(tokens, stop.nextPosition + 2, {TokenType::End}, "for");
      return {.statement = UniquePtr<ast::Statement>(
        new ast::For(name.lexeme, std::move(stop.expression), std::move(body.statements))),
        .nextPosition = body.nextPosition + 1
      };
    }
    case TokenType::Function:
      throw std::invalid_argument("Объявление функции допустимо только на верхнем уровне программы");
    case TokenType::Return:
      throw std::invalid_argument("return допустим только в конце многострочного тела функции");
    case TokenType::Otherwise:
      throw std::invalid_argument("Неожиданный otherwise вне блока when");
    case TokenType::End:
      throw std::invalid_argument("Неожиданный end вне блока");
    default:
      throw std::invalid_argument("Неожиданный токен в начале инструкции");
  }
}

Parser::ParsedStatement Parser::parseFunction(const Array<Token>& tokens, const std::size_t position) {
  const Token& name = requireToken(tokens, position + 1, TokenType::Identifier,
                                  "Ожидалось имя функции после function");
  requireToken(tokens, position + 2, TokenType::LeftParen,
               "Ожидалась открывающая скобка после имени функции");

  Array<std::string> parameters;
  std::size_t next = position + 3;
  if (next < tokens.size() && tokens[next].type != TokenType::RightParen) {
    while (true) {
      const Token& parameter = requireToken(tokens, next, TokenType::Identifier,
                                           "Ожидалось имя параметра функции");
      for (std::size_t i = 0; i < parameters.size(); ++i) {
        if (parameters[i] == parameter.lexeme) {
          throw std::invalid_argument("Повторяющееся имя параметра: " + parameter.lexeme);
        }
      }
      parameters.push_back(parameter.lexeme);
      ++next;
      if (next >= tokens.size() || tokens[next].type != TokenType::Comma) break;
      ++next;
    }
  }
  requireToken(tokens, next, TokenType::RightParen,
               "Ожидалась закрывающая скобка после списка параметров");
  requireToken(tokens, next + 1, TokenType::Arrow,
               "Ожидалась стрелка -> после списка параметров");

  next += 2;
  Array<UniquePtr<ast::Statement>> body;
  const bool multiline = next < tokens.size() && tokens[next].type == TokenType::Newline;
  if (multiline) {
    auto parsedBody = parseBody(tokens, next + 1, {TokenType::Return, TokenType::End}, "function");
    requireToken(tokens, parsedBody.nextPosition, TokenType::Return,
                 "Ожидался итоговый return в теле функции");
    body = std::move(parsedBody.statements);
    next = parsedBody.nextPosition + 1;
  }

  auto result = parseEquality(tokens, next);
  next = result.nextPosition;
  if (multiline) {
    requireToken(tokens, next, TokenType::Newline, "Ожидался перевод строки после return");
    do {
      ++next;
    } while (next < tokens.size() && tokens[next].type == TokenType::Newline);
  }
  requireToken(tokens, next, TokenType::End, "Ожидался end после выражения функции");
  return {
    .statement = UniquePtr<ast::Statement>(
      new ast::Function(name.lexeme, std::move(parameters), std::move(result.expression), std::move(body))),
    .nextPosition = next + 1
  };
}

Parser::ParsedBody Parser::parseBody(const Array<Token>& tokens, std::size_t position,
                                     const std::initializer_list<TokenType> terminators,
                                     const char* blockName) {
  Array<UniquePtr<ast::Statement>> statements;
  while (position < tokens.size()) {
    const TokenType type = tokens[position].type;
    if (type == TokenType::EndOfFile) break;
    if (type == TokenType::Newline) {
      ++position;
      continue;
    }
    if (std::ranges::find(terminators, type) != terminators.end()) {
      return {.statements = std::move(statements), .nextPosition = position};
    }
    if (type == TokenType::Otherwise) {
      throw std::invalid_argument(std::string("Неожиданный otherwise в блоке ") + blockName);
    }

    auto statement = parseStatement(tokens, position);
    position = statement.nextPosition;
    if (position == tokens.size() || tokens[position].type == TokenType::EndOfFile) break;
    requireToken(tokens, position, TokenType::Newline,
                 "Ожидался перевод строки после инструкции в блоке");
    statements.push_back(std::move(statement.statement));
    ++position;
  }
  throw std::invalid_argument(std::string("Ожидался end для блока ") + blockName);
}

void Parser::requireStatementEnd(const Array<Token>& tokens, const std::size_t position) {
  if (position < tokens.size()) {
    const TokenType type = tokens[position].type;
    if (type == TokenType::Newline || type == TokenType::EndOfFile) return;
  }
  throw std::invalid_argument("Ожидался перевод строки или конец файла");
}

const Token& Parser::requireToken(const Array<Token>& tokens, const std::size_t position, const TokenType expected, const char* message) {
  if (position >= tokens.size() || tokens[position].type != expected) {
    throw std::invalid_argument(message);
  }
  return tokens[position];
}

ast::ExpressionTree Parser::parseOperand(const Array<Token>& tokens, const std::size_t position) {
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

Parser::ParsedExpr Parser::parsePrimary(const Array<Token>& tokens, const std::size_t position) {
  if (position < tokens.size() && tokens[position].type == TokenType::Identifier &&
      position + 1 < tokens.size() && tokens[position + 1].type == TokenType::LeftParen) {

    Array<ast::ExpressionTree> arguments;
    std::size_t next = position + 2;

    if (next < tokens.size() && tokens[next].type != TokenType::RightParen) {
      while (true) {
        auto argument = parseEquality(tokens, next);
        arguments.push_back(std::move(argument.expression));
        next = argument.nextPosition;
        if (next >= tokens.size() || tokens[next].type != TokenType::Comma) break;
        ++next;
      }
    }
    requireToken(tokens, next, TokenType::RightParen, "Ожидалась закрывающая скобка вызова");
    return {
      .expression = ast::ExpressionTree::makeLeaf(
        UniquePtr<ast::Expr>(new ast::Call(tokens[position].lexeme, std::move(arguments)))),
      .nextPosition = next + 1
    };
  }

  // Если текущий токен является открывающей скобкой, то разбираем всё выражение внутри скобок.
  if (position < tokens.size() && tokens[position].type == TokenType::LeftParen) {
    auto inner = parseEquality(tokens, position + 1);
    requireToken(tokens, inner.nextPosition, TokenType::RightParen, "Ожидалась закрывающая скобка");
    // Пропускаем закрывающую скобку. Отдельный узел для скобок не нужен.
    ++inner.nextPosition;
    return inner;
  }
  return {.expression = parseOperand(tokens, position), .nextPosition = position + 1};
}

Parser::ParsedExpr Parser::parseUnary(const Array<Token>& tokens, const std::size_t position) {
  if (position < tokens.size()) {
    const auto& current = tokens[position];
    if (isUpdateOperator(current.type)) {
      const auto& name = requireToken(tokens, position + 1, TokenType::Identifier,
                                      "Ожидалось имя переменной после ++ или --");
      return {
        .expression = ast::ExpressionTree::makeLeaf(
          UniquePtr<ast::Expr>(new ast::Update(name.lexeme, current.type, false))),
        .nextPosition = position + 2
      };
    }
    if (current.type == TokenType::Identifier && position + 1 < tokens.size() &&
        isUpdateOperator(tokens[position + 1].type)) {
      return {
        .expression = ast::ExpressionTree::makeLeaf(
          UniquePtr<ast::Expr>(new ast::Update(current.lexeme, tokens[position + 1].type, true))),
        .nextPosition = position + 2
      };
    }
  }
  return parsePrimary(tokens, position);
}

Parser::ParsedExpr Parser::parseMultiplication(const Array<Token>& tokens, const std::size_t position) {
  return parseBinaryChain(tokens, position, parseUnary, {TokenType::Star, TokenType::Slash});
}

Parser::ParsedExpr Parser::parseAddition(const Array<Token>& tokens, const std::size_t position) {
  return parseBinaryChain(tokens, position, parseMultiplication,
{TokenType::Plus, TokenType::Minus});
}

Parser::ParsedExpr Parser::parseComparison(const Array<Token>& tokens, const std::size_t position) {
  return parseBinaryChain(tokens, position, parseAddition,
{TokenType::Less, TokenType::LessEqual, TokenType::Greater, TokenType::GreaterEqual});
}

Parser::ParsedExpr Parser::parseEquality(const Array<Token>& tokens, const std::size_t position) {
  return parseBinaryChain(tokens, position, parseComparison,
{TokenType::EqualEqual, TokenType::BangEqual});
}

Parser::ParsedExpr Parser::parseBinaryChain(const Array<Token>& tokens, const std::size_t position,
                                           const ExpressionParser parseOperandAtNextLevel,
                                           const std::initializer_list<TokenType> operations) {
  auto left = parseOperandAtNextLevel(tokens, position);

  while (left.nextPosition < tokens.size()) {
    const TokenType operation = tokens[left.nextPosition].type;

    // ReSharper disable once CppUseRangeAlgorithm
    if (std::find(operations.begin(), operations.end(), operation) == operations.end()) break;

    auto right = parseOperandAtNextLevel(tokens, left.nextPosition + 1);
    // Собираем слева направо: a - b - c превращается в (a - b) - c.
    left.expression = ast::ExpressionTree::makeBinary(
        operation, std::move(left.expression), std::move(right.expression)
    );
    left.nextPosition = right.nextPosition;
  }
  return left;
}
