#include "../headers/lexer.h"

#include "StateMachine/StateMachine.h"
#include <cstddef>
#include <stdexcept>
#include <utility>

namespace {
using State = LexerState;
using Event = LexerEvent;

bool isDigit(const char symbol) {
  return symbol >= '0' && symbol <= '9';
}

bool isIdentifierStart(const char symbol) {
  return (symbol >= 'a' && symbol <= 'z') ||
         (symbol >= 'A' && symbol <= 'Z') || symbol == '_';
}

LexerEvent classify(const char symbol) {
  if (isDigit(symbol)) return LexerEvent::Digit;
  if (isIdentifierStart(symbol)) return LexerEvent::WordStart;

  switch (symbol) {
    case ' ':
    case '\t':
      return LexerEvent::Space;
    case '"':
      return LexerEvent::Quote;
    case '=':
      return LexerEvent::EqualsSign;
    case '<':
      return LexerEvent::LessSign;
    case '>':
      return LexerEvent::GreaterSign;
    case '!':
      return LexerEvent::Bang;
    case '+':
    case '-':
    case '*':
    case '/':
    case '(':
    case ')':
      return LexerEvent::SingleSymbol;
    case '\r':
      return LexerEvent::CR;
    case '\n':
      return LexerEvent::LF;
    default:
      return LexerEvent::Other;
  }
}

StateMachine<State, Event> makeLexerMachine() {
  StateMachine<State, Event> machine(State::Start);
  machine.addTransition(State::Start, State::ReadingNumber, Event::Digit);
  machine.addTransition(State::Start, State::ReadingWord, Event::WordStart);
  machine.addTransition(State::ReadingNumber, State::ReadingNumber, Event::Digit);
  machine.addTransition(State::ReadingWord, State::ReadingWord, Event::WordStart);
  machine.addTransition(State::ReadingWord, State::ReadingWord, Event::Digit);

  machine.addTransition(State::Start, State::ReadingString, Event::Quote);
  machine.addTransition(State::ReadingString, State::StringClosed, Event::Quote);

  constexpr Event stringContentEvents[] = {
    Event::WordStart, Event::Digit, Event::Space, Event::EqualsSign,
    Event::LessSign, Event::GreaterSign, Event::Bang, Event::SingleSymbol, Event::Other
  };
  for (Event event : stringContentEvents) {
    machine.addTransition(State::ReadingString, State::ReadingString, event);
  }

  machine.addTransition(State::Start, State::AfterEquals, Event::EqualsSign);
  machine.addTransition(State::Start, State::AfterLess, Event::LessSign);
  machine.addTransition(State::Start, State::AfterGreater, Event::GreaterSign);
  machine.addTransition(State::Start, State::AfterBang, Event::Bang);
  machine.addTransition(State::Start, State::SingleSymbol, Event::SingleSymbol);
  machine.addTransition(State::AfterEquals, State::TwoCharOperator, Event::EqualsSign);
  machine.addTransition(State::AfterLess, State::TwoCharOperator, Event::EqualsSign);
  machine.addTransition(State::AfterGreater, State::TwoCharOperator, Event::EqualsSign);
  machine.addTransition(State::AfterBang, State::TwoCharOperator, Event::EqualsSign);

  machine.addTransition(State::Start, State::Newline, Event::LF);
  machine.addTransition(State::Start, State::AfterCR, Event::CR);
  machine.addTransition(State::AfterCR, State::Newline, Event::LF);

  return machine;
}

TokenType twoCharOperatorType(char firstSymbol) {
  switch (firstSymbol) {
    case '=': return TokenType::EqualEqual;
    case '!': return TokenType::BangEqual;
    case '<': return TokenType::LessEqual;
    case '>': return TokenType::GreaterEqual;
    default: throw std::logic_error("Неизвестный двухсимвольный оператор");
  }
}
}

Array<Token> Lexer::tokenize(const std::string& source) {
  auto machine = makeLexerMachine();
  Array<Token> tokens;

  for (std::size_t position = 0; position < source.size();) {
    if (classify(source[position]) == Event::Space) {
      ++position;
      continue;
    }

    const std::size_t start = position;
    machine.reset();
    // Символ без перехода остаётся на этой позиции для следующего токена.
    while (position < source.size() && machine.step(classify(source[position]))) {
      ++position;
    }

    const std::size_t length = position - start;
    TokenType type;
    switch (machine.getCurrentState()) {
      case State::ReadingNumber:
        type = TokenType::Number;
        break;
      case State::ReadingWord:
        type = TokenType::Identifier;
        if (source.compare(start, length, "remember") == 0) type = TokenType::Remember;
        else if (source.compare(start, length, "print") == 0) type = TokenType::Print;
        break;
      case State::StringClosed:
        type = TokenType::String;
        break;
      case State::TwoCharOperator:
        type = twoCharOperatorType(source[start]);
        break;
      case State::Newline:
        type = TokenType::Newline;
        break;
      case State::SingleSymbol:
      case State::AfterEquals:
      case State::AfterLess:
      case State::AfterGreater:
        type = singleSymbolType(source[start]);
        break;
      case State::Start:
      case State::AfterBang:
      case State::AfterCR:
        throw std::invalid_argument(std::string("Неожиданный символ: ") + source[start]);
      case State::ReadingString:
        if (position == source.size()) {
          throw std::invalid_argument("Ожидалась закрывающая кавычка");
        }
        throw std::invalid_argument("Строка не закрыта до перевода строки");
      default:
        throw std::logic_error("Неизвестное состояние лексера");
    }

    std::string lexeme = source.substr(start, length);
    tokens.push_back({.type = type, .lexeme = std::move(lexeme)});
  }

  tokens.push_back({.type = TokenType::EndOfFile, .lexeme = ""});
  return tokens;
}

TokenType Lexer::singleSymbolType(const char symbol) {
  switch (symbol) {
    case '+': return TokenType::Plus;
    case '-': return TokenType::Minus;
    case '*': return TokenType::Star;
    case '/': return TokenType::Slash;
    case '=': return TokenType::Equals;
    case '<': return TokenType::Less;
    case '>': return TokenType::Greater;
    case '(': return TokenType::LeftParen;
    case ')': return TokenType::RightParen;
    default:
      throw std::logic_error("Неизвестный односимвольный токен");
  }
}
