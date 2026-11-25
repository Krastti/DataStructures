#pragma once

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

}
