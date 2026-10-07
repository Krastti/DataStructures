#pragma once

template<typename T>
Optional<T>::Optional(const T& value) : storedValue(value), containsValue(true) {}

template<typename T>
Optional<T>::Optional(T&& value) : storedValue(std::move(value)), containsValue(true) {}

template<typename T>
Optional<T>& Optional<T>::operator=(T value) {
  storedValue = std::move(value);
  containsValue = true;
  return *this;
}

template<typename T>
bool Optional<T>::has_value() const noexcept {
  return containsValue;
}

template<typename T>
T& Optional<T>::value() {
  if (!containsValue) throw std::logic_error("В Optional нет значения");
  return storedValue;
}

template<typename T>
const T& Optional<T>::value() const {
  if (!containsValue) throw std::logic_error("В Optional нет значения");
  return storedValue;
}

template<typename T>
T& Optional<T>::operator*() {
  return value();
}

template<typename T>
const T& Optional<T>::operator*() const {
  return value();
}

template<typename T>
void Optional<T>::reset() noexcept {
  containsValue = false;
}

template<typename T>
template<typename Function>
auto Optional<T>::map(Function function) const {
  using Result = std::decay_t<decltype(function(storedValue))>;
  if (!containsValue) return Optional<Result>{};
  return Optional<Result>(function(storedValue));
}

template<typename T>
template<typename Function>
auto Optional<T>::andThen(Function function) const {
  using Result = decltype(function(storedValue));
  if (!containsValue) return Result{};
  return function(storedValue);
}
