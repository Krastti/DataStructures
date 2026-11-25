#ifndef DATASTRUCTURES_INTERPRETER_H
#define DATASTRUCTURES_INTERPRETER_H

#include "Array/Array.h"
#include "Optional/Optional.h"
#include "UniquePtr.h"
#include "RedBlackTree.h"
#include "statements.h"
#include "visitors.h"

#include <iosfwd>
#include <string>
#include <variant>

class Interpreter : public ast::ExprVisitor, public ast::StatementVisitor {
public:
  // Значение MiniLang при выполнении: число, логическое значение или строка.
  using Value = std::variant<double, bool, std::string>;

  // Каждый запуск начинает с пустой таблицы переменных. Поток должен жить до конца вызова.
  void execute(const Array<UniquePtr<ast::Statement>>& statements, std::ostream& output);

  void visit(const ast::NumberLiteral& node) override;
  void visit(const ast::Identifier& node) override;
  void visit(const ast::StringLiteral& node) override;
  void visit(const ast::Binary& node) override;

  void visit(const ast::Remember& node) override;
  void visit(const ast::Print& node) override;
  void visit(const ast::When& node) override;
  void visit(const ast::RepeatWhile& node) override;
  void visit(const ast::For& node) override;

private:
  struct LoopVariable;

  Map<std::string, Value> variables;
  LoopVariable* activeLoopVariable = nullptr;

  // visit возвращает void, поэтому результат выражения сохраняем отдельно.
  Optional<Value> lastValue;
  std::ostream* output = nullptr;

  // Забирает результат после accept, до перехода к следующему выражению.
  Value evaluate(const ast::Expr& expression);
  bool evaluateCondition(const ast::ExpressionTree& condition, const char* instruction);
  void executeStatements(const Array<UniquePtr<ast::Statement>>& statements);
  Value* findLoopVariable(const std::string& name);
  void storeVariable(const std::string& name, Value value);
};

#endif //DATASTRUCTURES_INTERPRETER_H
