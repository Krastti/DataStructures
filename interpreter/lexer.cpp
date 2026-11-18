#include "lexer.h"

#include "../include/Optional/Optional.h"
#include <stdexcept>

namespace {
bool isDigit(const char symbol) {
  return symbol >= '0' && symbol <= '9';
}

bool isIdentifierStart(const char symbol) {
  return (symbol >= 'a' && symbol <= 'z') ||
         (symbol >= 'A' && symbol <= 'Z') || symbol == '_';
}
}

Array<Token> Lexer::tokenize(const std::string& source) {
  Array<Token> tokens;

  for (std::size_t position = 0; position < source.size(); ++position) {
    const char symbol = source[position];

    if (symbol == ' ' || symbol == '\t') continue;

    if (symbol == '"') {
      const std::size_t end = readString(source, position);
      tokens.push_back({TokenType::String, source.substr(position, end - position)});
      position = end - 1;
      continue;
    }

    if (symbol == '\r' && position + 1 < source.size() && source[position + 1] == '\n') {
      tokens.push_back({.type = TokenType::Newline, .lexeme = "\r\n"});
      ++position;
      continue;
    }

    if (position + 1 < source.size() && source[position + 1] == '=') {
      Optional<TokenType> type;
      switch (symbol) {
        case '=': type = TokenType::EqualEqual; break;
        case '!': type = TokenType::BangEqual; break;
        case '<': type = TokenType::LessEqual; break;
        case '>': type = TokenType::GreaterEqual; break;
        default: break;
      }
      if (type.has_value()) {
        tokens.push_back({.type = *type, .lexeme = source.substr(position, 2)});
        ++position;
        continue;
      }
    }

    if (isDigit(symbol)) {
      const std::size_t end = readNumber(source, position);
      tokens.push_back({.type = TokenType::Number, .lexeme = source.substr(position, end - position)});
      position = end - 1;
      continue;
    }

    if (isIdentifierStart(symbol)) {
      const std::size_t end = readIdentifier(source, position);
      const std::string word = source.substr(position, end - position);
      TokenType type = TokenType::Identifier;
      if (word == "remember") type = TokenType::Remember;
      else if (word == "print") type = TokenType::Print;
      tokens.push_back({.type = type, .lexeme = word});
      position = end - 1;
      continue;
    }

    tokens.push_back(scanToken(symbol));
  }

  tokens.push_back({TokenType::EndOfFile, ""});
  return tokens;
}

Token Lexer::scanToken(const char symbol) {
  TokenType type;
  switch (symbol) {
    case '+': type = TokenType::Plus; break;
    case '-': type = TokenType::Minus; break;
    case '*': type = TokenType::Star; break;
    case '/': type = TokenType::Slash; break;
    case '=': type = TokenType::Equals; break;
    case '<': type = TokenType::Less; break;
    case '>': type = TokenType::Greater; break;
    case '(': type = TokenType::LeftParen; break;
    case ')': type = TokenType::RightParen; break;
    case '\n': type = TokenType::Newline; break;
    default:
      throw std::invalid_argument(std::string("Неожиданный символ: ") + symbol);
  }
  return {.type = type, .lexeme = std::string(1, symbol)};
}

std::size_t Lexer::readNumber(const std::string& source, const std::size_t start) {
  std::size_t end = start;
  while (end < source.size() && isDigit(source[end])) ++end;
  return end;
}

std::size_t Lexer::readIdentifier(const std::string& source, const std::size_t start) {
  std::size_t end = start + 1;
  while (end < source.size() && (isIdentifierStart(source[end]) || isDigit(source[end]))) ++end;
  return end;
}

std::size_t Lexer::readString(const std::string& source, const std::size_t start) {
  if (start >= source.size() || source[start] != '"') {
    throw std::invalid_argument("Ожидалась открывающая кавычка");
  }

  for (std::size_t end = start + 1; end < source.size(); ++end) {
    if (source[end] == '"') return end + 1;
    if (source[end] == '\n' || source[end] == '\r') {
      throw std::invalid_argument("Строка не закрыта до перевода строки");
    }
  }

  throw std::invalid_argument("Ожидалась закрывающая кавычка");
}
