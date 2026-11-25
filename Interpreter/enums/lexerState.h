#ifndef DATASTRUCTURES_LEXER_STATE_H
#define DATASTRUCTURES_LEXER_STATE_H

enum class LexerState {
  Start,
  ReadingNumber,
  AfterDecimalPoint,
  ReadingFraction,
  ReadingWord,
  ReadingString,
  StringClosed,

  AfterEquals,
  AfterLess,
  AfterGreater,
  AfterPlus,
  AfterMinus,
  AfterBang,
  TwoCharOperator,
  SingleSymbol,

  AfterCR,
  Newline
};

#endif // DATASTRUCTURES_LEXER_STATE_H
