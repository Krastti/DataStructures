#ifndef DATASTRUCTURES_LEXER_STATE_H
#define DATASTRUCTURES_LEXER_STATE_H

enum class LexerState {
  Start,
  ReadingNumber,
  ReadingWord,
  ReadingString,
  StringClosed,

  AfterEquals,
  AfterLess,
  AfterGreater,
  AfterBang,
  TwoCharOperator,
  SingleSymbol,

  AfterCR,
  Newline
};

#endif // DATASTRUCTURES_LEXER_STATE_H
