#ifndef DATASTRUCTURES_MINILANG_ENVIRONMENT_H
#define DATASTRUCTURES_MINILANG_ENVIRONMENT_H

#include "Array/Array.h"
#include "RedBlackTree.h"

#include <string>
#include <variant>

namespace minilang {

// Окружение
class Environment {
public:
  // Значение MiniLang при выполнении: число, логическое значение или строка.
  using Value = std::variant<double, bool, std::string>;
  using Variables = Map<std::string, Value>;

  void reset();
  [[nodiscard]] const Value& lookup(const std::string& name) const;
  void assign(const std::string& name, Value value);

  // Ссылка на счётчик действует только во время вызова run.
  template<typename Run>
  void withLoopVariable(const std::string& name, Run&& run);

private:
  Variables variables;
  Array<Variables*> localScopes;

  [[nodiscard]] Variables* findLocalScope(const std::string& name) const;
};

}

#include "environment.tpp"

#endif //DATASTRUCTURES_MINILANG_ENVIRONMENT_H
