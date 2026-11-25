// ReSharper disable CppTooWideScopeInitStatement
// ReSharper disable CppUseStructuredBinding
#include "../headers/interpreter.h"

#include <charconv>
#include <cmath>
#include <ostream>
#include <stdexcept>
#include <system_error>
#include <utility>

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

struct Interpreter::LoopVariable {
  const std::string& name;
  Value value{0.0};
  LoopVariable* const outer;
  LoopVariable*& active;

  LoopVariable(const std::string& name, LoopVariable*& active) noexcept
      : name(name), outer(active), active(active) {
    active = this;
  }

  LoopVariable(const LoopVariable&) = delete;
  LoopVariable& operator=(const LoopVariable&) = delete;

  ~LoopVariable() {
    // Возвращаем внешнюю переменную в область поиска и при выходе с ошибкой.
    active = outer;
  }
};

void Interpreter::execute(const Array<UniquePtr<ast::Statement>>& statements, std::ostream& outputStream) {
  variables = Map<std::string, Value>{};
  lastValue.reset();
  output = &outputStream;

  try {
    executeStatements(statements);
  } catch (...) {
    // Поток принадлежит вызывающему коду, поэтому не сохраняем ссылку на него после запуска.
    output = nullptr;
    lastValue.reset();
    throw;
  }
  output = nullptr;
  lastValue.reset();
}

void Interpreter::executeStatements(const Array<UniquePtr<ast::Statement>>& statements) {
  for (std::size_t i = 0; i < statements.size(); ++i) {
    if (!statements[i]) throw std::invalid_argument("Ошибка выполнения: пустая инструкция");
    statements[i]->accept(*this);
  }
}

Interpreter::Value Interpreter::evaluate(const ast::Expr& expression) {
  lastValue.reset();
  expression.accept(*this);
  if (!lastValue.has_value()) {
    throw std::logic_error("Ошибка выполнения: выражение не вернуло значение");
  }
  Value result = std::move(lastValue.value());
  lastValue.reset();
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
  if (const auto* local = findLoopVariable(node.name)) {
    lastValue = *local;
    return;
  }
  if (!variables.containsKey(node.name)) {
    throw std::invalid_argument("Ошибка выполнения: неизвестная переменная: " + node.name);
  }
  lastValue = variables.at(node.name);
}

void Interpreter::visit(const ast::StringLiteral& node) {
  if (node.lexeme.size() < 2 || node.lexeme.front() != '"' || node.lexeme.back() != '"') {
    throw std::invalid_argument("Ошибка выполнения: некорректный строковый литерал");
  }
  lastValue = Value{node.lexeme.substr(1, node.lexeme.size() - 2)};
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

void Interpreter::visit(const ast::Remember& node) {
  const auto* expression = node.right.getRoot();
  if (!expression) throw std::invalid_argument("Ошибка выполнения: отсутствует выражение remember");
  // Сначала вычисляем правую часть, чтобы при ошибке сохранить старое значение переменной.
  storeVariable(node.variable, evaluate(*expression));
}

Interpreter::Value* Interpreter::findLoopVariable(const std::string& name) {
  for (auto* local = activeLoopVariable; local; local = local->outer) {
    if (local->name == name) return &local->value;
  }
  return nullptr;
}

void Interpreter::storeVariable(const std::string& name, Value value) {
  if (auto* local = findLoopVariable(name)) {
    *local = std::move(value);
    return;
  }
  if (variables.containsKey(name)) variables.replace(name, std::move(value));
  else variables.insert(name, std::move(value));
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
  const auto* expression = node.stop.getRoot();
  if (!expression) throw std::invalid_argument("Ошибка выполнения: отсутствует граница range");

  const Value value = evaluate(*expression);
  const auto* stop = std::get_if<double>(&value);
  // До 2^53 счётчик double точно представляет каждый целый шаг.
  constexpr double maxStop = 0x1p53;
  if (!stop || !std::isfinite(*stop) || std::trunc(*stop) != *stop || std::abs(*stop) > maxStop) {
    throw std::invalid_argument("Ошибка выполнения: граница range должна быть конечным целым числом от -2^53 до 2^53");
  }

  LoopVariable variable(node.variable, activeLoopVariable);
  for (double index = 0.0; index < *stop; ++index) {
    variable.value = index;
    executeStatements(node.body);
  }
}
