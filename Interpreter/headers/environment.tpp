#pragma once

#include <stdexcept>
#include <utility>

namespace minilang {

// Run выводится из вызываемого объекта, например лямбды, которая выполняет цикл.
template<typename Run>
void Environment::withLoopVariable(const std::string& name, Run&& run) {
  Variables local;
  local.insert(name, Value{0.0});

  // Счётчик цикла
  auto& counter = local.at(name);
  localScopes.push_back(&local);

  try {
    std::forward<Run>(run)(counter);
  } catch (...) {
    localScopes.pop_back();
    throw;
  }
  localScopes.pop_back();
}

template<typename Run>
Environment::Value Environment::withFunctionScope(
    const Array<std::string>& parameters, const Array<Value>& arguments, Run&& run) {
  if (parameters.size() != arguments.size()) {
    throw std::invalid_argument("Ошибка выполнения: число аргументов не совпадает с числом параметров");
  }
  constexpr std::size_t maxDepth = 128;

  const std::size_t depth = callFrames.empty() ? 1 : callFrames.top().depth + 1;
  if (depth > maxDepth) {
    throw std::invalid_argument("Ошибка выполнения: превышена глубина вызовов функций");
  }

  CallFrame frame(localScopes.size(), depth);
  for (std::size_t i = 0; i < parameters.size(); ++i) {
    frame.locals.insert(parameters[i], arguments[i]);
  }
  callFrames.push(std::move(frame));

  struct PopFrame {
    Stack<CallFrame>& frames;
    ~PopFrame() { frames.pop(); }
  } guard{callFrames};

  return std::forward<Run>(run)();
}

}
