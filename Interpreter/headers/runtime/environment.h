// ReSharper disable CppParameterMayBeConst
#ifndef DATASTRUCTURES_MINILANG_ENVIRONMENT_H
#define DATASTRUCTURES_MINILANG_ENVIRONMENT_H

#include "Array/Array.h"
#include "RedBlackTree.h"
#include "Stack/stack.h"

#include <cstddef>
#include <string>
#include <variant>

namespace minilang {

// Окружение
class Environment {
public:
  // Значение MiniLang при выполнении: число, логическое значение или строка.
  using Value = std::variant<double, bool, std::string>;
  using Variables = Map<std::string, Value>;

  Environment() = default;
  Environment(const Environment&) = delete;
  Environment& operator=(const Environment&) = delete;

  void reset();
  [[nodiscard]] const Value& lookup(const std::string& name) const;
  void assign(const std::string& name, Value value);

  // Ссылка на счётчик действует только во время вызова run.
  template<typename Run>
  void withLoopVariable(const std::string& name, Run&& run);

  // Параметры доступны только во время вызова run.
  template<typename Run>
  Value withFunctionScope(const Array<std::string>& parameters, const Array<Value>& arguments, Run&& run);

private:
  Variables variables;
  Array<Variables*> localScopes;

  struct CallFrame {
    Variables locals;
    // С этой позиции начинаются области циклов текущей функции.
    std::size_t firstLoopScope;
    std::size_t depth;

    CallFrame(std::size_t firstLoopScope, std::size_t depth)
      : firstLoopScope(firstLoopScope), depth(depth) {}

    CallFrame(const CallFrame&) = delete;
    CallFrame(CallFrame&&) = default;
  };

  Stack<CallFrame> callFrames;

  [[nodiscard]] Variables* findLocalScope(const std::string& name) const;
};

}

#include "environment.tpp"

#endif //DATASTRUCTURES_MINILANG_ENVIRONMENT_H
