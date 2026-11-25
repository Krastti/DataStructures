#ifndef DATASTRUCTURES_LEXER_EVENT_H
#define DATASTRUCTURES_LEXER_EVENT_H

enum class LexerEvent {
  WordStart, // Латинская буква или подчёркивание
  Digit, // ASCII-цифра
  Space, // Пробел или табуляция
  Quote, // Двойная кавычка

  EqualsSign,
  LessSign,
  GreaterSign,
  PlusSign,
  MinusSign,
  Dot,
  Bang,
  SingleSymbol, // * / ( ) ,

  CR,
  LF,
  Other // Остальные байты
};

#endif // DATASTRUCTURES_LEXER_EVENT_H
