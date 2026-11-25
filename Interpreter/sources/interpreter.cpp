// ReSharper disable CppTooWideScopeInitStatement
// ReSharper disable CppUseStructuredBinding
#include "../headers/interpreter.h"

#include <charconv>
#include <cmath>
#include <cstdint>
#include <ostream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <variant>

namespace {
std::string formatValue(const Interpreter::Value& value) {
  if (const auto* number = std::get_if<double>(&value)) {
    char buffer[128];
    const auto result = std::to_chars(buffer, buffer + sizeof(buffer), *number);
    if (result.ec != std::errc{}) {
      throw std::runtime_error("Ошибка выполнения: не удалось вывести число");
    }
    std::string text(buffer, result.ptr);
    if (std::isfinite(*number) && text.find('.') == std::string::npos) {
      const auto exponent = text.find_first_of("eE");
      text.insert(exponent == std::string::npos ? text.size() : exponent, ".0");
    }
    return text;
  }
  if (const auto* boolean = std::get_if<bool>(&value)) return *boolean ? "true" : "false";
  return std::get<std::string>(value);
}

Interpreter::Value applyNumericOperation(const TokenType operation, const double left, const double right) {
  switch (operation) {
    case TokenType::Plus: return left + right;
    case TokenType::Minus: return left - right;
    case TokenType::Star: return left * right;
    case TokenType::Slash:
      if (right == 0.0) throw std::invalid_argument("Ошибка выполнения: деление на ноль");
      return left / right;
    case TokenType::Less: return left < right;
    case TokenType::LessEqual: return left <= right;
    case TokenType::Greater: return left > right;
    case TokenType::GreaterEqual: return left >= right;
    case TokenType::EqualEqual: return left == right;
    case TokenType::BangEqual: return left != right;
    default: throw std::invalid_argument("Ошибка выполнения: неподдерживаемый оператор");
  }
}
}

void Interpreter::execute(const Array<UniquePtr<ast::Statement>>& statements, std::ostream& outputStream,
                          Observer* trace) {
  environment.reset();
  lastValue.reset();
  output = &outputStream;
  observer = trace;

  try {
    executeStatements(statements);
  } catch (...) {
    // Поток и наблюдатель принадлежат вызывающему коду.
    output = nullptr;
    observer = nullptr;
    lastValue.reset();
    throw;
  }
  output = nullptr;
  observer = nullptr;
  lastValue.reset();
}

void Interpreter::executeStatements(const Array<UniquePtr<ast::Statement>>& statements) {
  for (std::size_t i = 0; i < statements.size(); ++i) {
    if (!statements[i]) throw std::invalid_argument("Ошибка выполнения: пустая инструкция");
    executeStatement(*statements[i]);
  }
}

void Interpreter::executeStatement(const ast::Statement& statement) {
  if (observer) observer->enterStatement(statement);
  statement.accept(*this);
  if (observer) observer->leaveCall(nullptr);
}

Interpreter::Value Interpreter::evaluate(const ast::Expr& expression) {
  if (observer) observer->enterExpression(expression);
  lastValue.reset();
  expression.accept(*this);
  if (!lastValue.has_value()) {
    throw std::logic_error("Ошибка выполнения: выражение не вернуло значение");
  }
  Value result = std::move(lastValue.value());
  lastValue.reset();
  if (observer) observer->leaveCall(&result);
  return result;
}

bool Interpreter::evaluateCondition(const ast::ExpressionTree& condition, const char* instruction) {
  const auto* expression = condition.getRoot();
  if (!expression) {
    throw std::invalid_argument(std::string("Ошибка выполнения: отсутствует условие ") + instruction);
  }
  const Value value = evaluate(*expression);
  const auto* boolean = std::get_if<bool>(&value);
  if (!boolean) {
    throw std::invalid_argument(std::string("Ошибка выполнения: условие ") + instruction +
                                " должно быть логическим");
  }
  return *boolean;
}

void Interpreter::visit(const ast::NumberLiteral& node) {
  double number = 0.0;
  const char* end = node.lexeme.data() + node.lexeme.size();
  const auto result = std::from_chars(node.lexeme.data(), end, number);
  if (result.ec != std::errc{} || result.ptr != end || !std::isfinite(number)) {
    throw std::invalid_argument("Ошибка выполнения: некорректное число: " + node.lexeme);
  }
  lastValue = Value{number};
}

void Interpreter::visit(const ast::Identifier& node) {
  lastValue = environment.lookup(node.name);
}

void Interpreter::visit(const ast::StringLiteral& node) {
  if (node.lexeme.size() < 2 || node.lexeme.front() != '"' || node.lexeme.back() != '"') {
    throw std::invalid_argument("Ошибка выполнения: некорректный строковый литерал");
  }
  lastValue = Value{node.lexeme.substr(1, node.lexeme.size() - 2)};
}

void Interpreter::visit(const ast::Call&) {
  throw std::logic_error("Вычисление вызовов функций ещё не реализовано");
}

void Interpreter::visit(const ast::Binary& node) {
  // Сохраняем левое значение до обхода правого: тот тоже использует lastValue.
  const Value leftValue = evaluate(*node.leftOperand());
  const Value rightValue = evaluate(*node.rightOperand());
  const auto* left = std::get_if<double>(&leftValue);
  const auto* right = std::get_if<double>(&rightValue);
  if (!left || !right) {
    throw std::invalid_argument("Ошибка выполнения: оператор ожидает два числа");
  }
  lastValue = applyNumericOperation(node.operation, *left, *right);
}

void Interpreter::visit(const ast::Function&) {
  throw std::logic_error("Выполнение объявлений функций ещё не реализовано");
}

void Interpreter::visit(const ast::Remember& node) {
  const auto* expression = node.right.getRoot();
  if (!expression) throw std::invalid_argument("Ошибка выполнения: отсутствует выражение remember");
  // Сначала вычисляем правую часть, чтобы при ошибке сохранить старое значение переменной.
  environment.assign(node.variable, evaluate(*expression));
}

void Interpreter::visit(const ast::Print& node) {
  if (!output) throw std::logic_error("Print можно выполнять только во время execute");

  const auto* expression = node.forOutput.getRoot();
  if (!expression) throw std::invalid_argument("Ошибка выполнения: отсутствует выражение print");

  const std::string text = formatValue(evaluate(*expression));
  output->write(text.data(), static_cast<std::streamsize>(text.size()));
  output->put('\n');
  if (!*output) throw std::runtime_error("Ошибка выполнения: не удалось записать результат");
}

void Interpreter::visit(const ast::When& node) {
  const bool condition = evaluateCondition(node.condition, "when");
  executeStatements(condition ? node.thenBranch : node.otherwiseBranch);
}

void Interpreter::visit(const ast::RepeatWhile& node) {
  while (evaluateCondition(node.condition, "repeat while")) {
    executeStatements(node.body);
  }
}

void Interpreter::visit(const ast::For& node) {
  // node.stop хранит дерево аргумента range, например 1 + 2.
  const auto* expression = node.stop.getRoot();
  if (!expression) throw std::invalid_argument("Ошибка выполнения: отсутствует граница range");

  const Value value = evaluate(*expression);
  // Stop указывает на вычисленное число внутри value.
  // Если значение имеет другой тип, get_if вернёт nullptr.
  const auto* stop = std::get_if<double>(&value);

  // До 2^53 каждое значение счётчика точно представимо в double.
  constexpr double maxStop = 0x1p53;
  // isfinite исключает NaN и бесконечности. Сравнение с trunc выявляет дробную часть, abs даёт модуль.
  if (!stop || !std::isfinite(*stop) || std::trunc(*stop) != *stop || std::abs(*stop) > maxStop) {
    throw std::invalid_argument("Ошибка выполнения: граница range должна быть конечным целым числом от -2^53 до 2^53");
  }

  if (*stop <= 0.0) return;

  // Переводим проверенную границу в целое число для сравнения со счётчиком.
  const auto limit = static_cast<std::uint64_t>(*stop);

  environment.withLoopVariable(node.variable, [&](Value& counter) {
    for (std::uint64_t index = 0; index < limit; ++index) {
      counter = static_cast<double>(index);
      executeStatements(node.body);
    }
  });
}
