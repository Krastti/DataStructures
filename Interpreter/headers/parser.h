#ifndef DATASTRUCTURES_PARSER_H
#define DATASTRUCTURES_PARSER_H

#include "Array/Array.h"
#include "UniquePtr.h"
#include "statements.h"
#include "token.h"

#include <cstddef>
#include <initializer_list>

// Собирает дерево программы из токенов.
class Parser {
public:
  // Собирает инструкции программы по порядку. В конце массива токенов ожидает EOF.
  static Array<UniquePtr<ast::Statement>> parse(const Array<Token>& tokens);

private:
  // nextPosition — индекс следующего непрочитанного токена.
  struct ParsedExpr {
    ast::ExpressionTree expression;
    std::size_t nextPosition;
  };

  // Хранит разобранную инструкцию и позицию для продолжения разбора.
  // nextPosition — индекс первого непрочитанного токена после инструкции.
  struct ParsedStatement {
    UniquePtr<ast::Statement> statement;
    std::size_t nextPosition;
  };

  struct ParsedBody {
    Array<UniquePtr<ast::Statement>> statements;
    std::size_t nextPosition;
  };

  // Функция, которая разбирает операнд на следующем уровне приоритета.
  using ExpressionParser = ParsedExpr (*)(const Array<Token>&, std::size_t);

  // Возвращает токен нужного типа или выбрасывает исключение с переданным сообщением.
  static const Token& requireToken(const Array<Token>& tokens, std::size_t position,
                                  TokenType expected, const char* message);

  // Проверяет, что после инструкции идёт перевод строки или EOF.
  static void requireStatementEnd(const Array<Token>& tokens, std::size_t position);

  // Разбирает одну инструкцию и возвращает позицию после неё.
  static ParsedStatement parseStatement(const Array<Token>& tokens, std::size_t position);

  static ParsedStatement parseFunction(const Array<Token>& tokens, std::size_t position);

  // Собирает инструкции блока и оставляет позицию на маркере его окончания.
  static ParsedBody parseBody(const Array<Token>& tokens, std::size_t position,
                              std::initializer_list<TokenType> terminators, const char* blockName);

  // Создаёт лист дерева из токена числа, строки или имени переменной.
  static ast::ExpressionTree parseOperand(const Array<Token>& tokens, std::size_t position);

  // Разбирает простой операнд или выражение в скобках.
  static ParsedExpr parsePrimary(const Array<Token>& tokens, std::size_t position);

  // Разбирает префиксное и постфиксное изменение переменной.
  static ParsedExpr parseUnary(const Array<Token>& tokens, std::size_t position);

  // Разбирает * и /, получая операнды через parseUnary.
  static ParsedExpr parseMultiplication(const Array<Token>& tokens, std::size_t position);

  // Разбирает + и -, сначала собирая произведения и частные.
  static ParsedExpr parseAddition(const Array<Token>& tokens, std::size_t position);

  // Разбирает <, <=, > и >= после арифметических операций.
  static ParsedExpr parseComparison(const Array<Token>& tokens, std::size_t position);

  // Разбирает == и !=. Разбор выражения начинается здесь, потому что у этих операций самый низкий приоритет.
  static ParsedExpr parseEquality(const Array<Token>& tokens, std::size_t position);

  // Собирает цепочку операций одного уровня слева направо, разбирая операнды на следующем уровне.
  static ParsedExpr parseBinaryChain(const Array<Token>& tokens, std::size_t position,
                                    ExpressionParser parseOperandAtNextLevel,
                                    std::initializer_list<TokenType> operations);
};

#endif //DATASTRUCTURES_PARSER_H
