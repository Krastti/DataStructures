#ifndef DATASTRUCTURES_TOKEN_H
#define DATASTRUCTURES_TOKEN_H

#include "tokenType.h"

#include <string>

struct Token {
  TokenType type;
  std::string lexeme;
};

#endif //DATASTRUCTURES_TOKEN_H
