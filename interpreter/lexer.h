#ifndef DATASTRUCTURES_LEXER_H
#define DATASTRUCTURES_LEXER_H

#include "../include/Array/Array.h"
#include "token.h"

#include <cstddef>
#include <string>

class Lexer {
private:
  static Token scanToken(char symbol);
  static std::size_t readNumber(const std::string& source, std::size_t start);
  static std::size_t readIdentifier(const std::string& source, std::size_t start);
  // start указывает на открывающую кавычку; результат — позиция после закрывающей.
  static std::size_t readString(const std::string& source, std::size_t start);

public:
  static Array<Token> tokenize(const std::string& source);
};

#endif //DATASTRUCTURES_LEXER_H
