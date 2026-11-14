#pragma once

#include <chrono>

// Время включает формирование результата, но не обновление виджетов.
template<typename Action>
auto measured(Action action, double& milliseconds) {
  const auto start = std::chrono::steady_clock::now();
  auto result = action();
  milliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
  return result;
}

