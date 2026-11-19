#ifndef DATASTRUCTURES_INTERPRETER_H
#define DATASTRUCTURES_INTERPRETER_H

#include "Array/Array.h"
#include "Optional/Optional.h"
#include "UniquePtr.h"
#include "Trees/RedBlackTree.h"
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

private:
  Map<std::string, Value> variables;

  // visit возвращает void, поэтому результат выражения сохраняем отдельно.
  Optional<Value> lastValue;
  std::ostream* output = nullptr;

  // Забирает результат после accept, до перехода к следующему выражению.
  Value evaluate(const ast::Expr& expression);
};

#endif //DATASTRUCTURES_INTERPRETER_H
