#ifndef DATASTRUCTURES_OPTIONAL_H
#define DATASTRUCTURES_OPTIONAL_H

#include <stdexcept>
#include <type_traits>
#include <utility>

// Хранит значение типа T или обозначает его отсутствие.
// T должен поддерживать создание значения по умолчанию и присваивание.
template<typename T>
class Optional {
private:
  T storedValue{};
  bool containsValue = false;

public:
  // Конструкторы.
  Optional() = default;
  explicit Optional(const T& value);
  explicit Optional(T&& value);
  Optional& operator=(T value);

  [[nodiscard]] bool has_value() const noexcept;

  T& value();
  const T& value() const;

  T& operator*();
  const T& operator*() const;

  void reset() noexcept;

  /**
   * Преобразует хранящееся значение, если оно есть.
   * Для пустого Optional функция не вызывается, иначе её результат сохраняется в новом Optional.
   */
  template<typename Function>
  auto map(Function function) const;

  /**
   * Продолжает вычисление функцией, которая сама возвращает Optional.
   * Для пустого Optional функция не вызывается, иначе её результат возвращается напрямую, без вложенного Optional.
   */
  template<typename Function>
  auto andThen(Function function) const;
};

#include "Optional.tpp"

#endif // DATASTRUCTURES_OPTIONAL_H
