#ifndef DATASTRUCTURES_INTERPRETER_H
#define DATASTRUCTURES_INTERPRETER_H

#include "Array/Array.h"
#include "Optional/Optional.h"
#include "UniquePtr.h"
#include "environment.h"
#include "statements.h"
#include "visitors.h"

#include <iosfwd>

class Interpreter : public ast::ExprVisitor, public ast::StatementVisitor {
public:
  using Value = minilang::Environment::Value;
  using Variables = minilang::Environment::Variables;

  struct Observer {
    virtual ~Observer() = default;
    virtual void enterStatement(const ast::Statement& statement) = 0;
    virtual void enterExpression(const ast::Expr& expression) = 0;
    // Значение передаётся только при успешном завершении evaluate.
    virtual void leaveCall(const Value* value) = 0;
  };

  // Каждый запуск начинает с пустой таблицы переменных. Поток должен жить до конца вызова.
  void execute(const Array<UniquePtr<ast::Statement>>& statements, std::ostream& output,
               Observer* trace = nullptr);

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
  minilang::Environment environment;

  // visit возвращает void, поэтому результат выражения сохраняем отдельно.
  Optional<Value> lastValue;
  std::ostream* output = nullptr;
  Observer* observer = nullptr;

  // Забирает результат после accept, до перехода к следующему выражению.
  Value evaluate(const ast::Expr& expression);
  bool evaluateCondition(const ast::ExpressionTree& condition, const char* instruction);
  void executeStatements(const Array<UniquePtr<ast::Statement>>& statements);
  void executeStatement(const ast::Statement& statement);
};

#endif //DATASTRUCTURES_INTERPRETER_H
