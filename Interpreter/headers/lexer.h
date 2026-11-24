#ifndef DATASTRUCTURES_LEXER_H
#define DATASTRUCTURES_LEXER_H

#include "Array/Array.h"
#include "token.h"
#include "../enums/lexerEvent.h"
#include "../enums/lexerState.h"

#include <string>

class Lexer {
private:
  static TokenType singleSymbolType(char symbol);

public:
  static Array<Token> tokenize(const std::string& source);
};

#endif //DATASTRUCTURES_LEXER_H
