#ifndef DATASTRUCTURES_TOKEN_H
#define DATASTRUCTURES_TOKEN_H

#include <string>

enum class TokenType {
  Remember,
  Print,

  Identifier,
  Number,
  String,

  Plus,
  Minus,
  Star,
  Slash,

  Equals,
  EqualEqual,
  BangEqual,
  Less,
  LessEqual,
  Greater,
  GreaterEqual,

  LeftParen,
  RightParen,
  Newline,
  EndOfFile,

  When,
  Otherwise,
  Repeat,
  While,
  End,

  For,
  In,
  Range,

  Function,
  Arrow,
  Comma,
  Return,
  Increment,
  Decrement,
  PlusEquals,
  MinusEquals,
};

struct Token {
  TokenType type;
  std::string lexeme;
};

#endif //DATASTRUCTURES_TOKEN_H
