#include "../headers/environment.h"

#include <cstddef>
#include <stdexcept>
#include <utility>

namespace minilang {

void Environment::reset() {
  variables = Variables{};
}

const Environment::Value& Environment::lookup(const std::string& name) const {
  if (const auto* local = findLocalScope(name)) return local->at(name);
  if (!variables.containsKey(name)) {
    throw std::invalid_argument("Ошибка выполнения: неизвестная переменная: " + name);
  }
  return variables.at(name);
}

void Environment::assign(const std::string& name, Value value) {
  if (auto* local = findLocalScope(name)) {
    local->at(name) = std::move(value);
    return;
  }
  if (variables.containsKey(name)) variables.replace(name, std::move(value));
  else variables.insert(name, std::move(value));
}

Environment::Variables* Environment::findLocalScope(const std::string& name) const {
  for (std::size_t i = localScopes.size(); i > 0; --i) {
    auto* scope = localScopes[i - 1];
    if (scope->containsKey(name)) return scope;
  }
  return nullptr;
}

}
