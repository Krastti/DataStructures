#pragma once
#include <charconv>
#include <cmath>
#include <cstddef>
#include <functional>
#include <limits>
#include <stdexcept>
#include <system_error>
#include <type_traits>
#include <utility>

template <typename Key>
auto Set<Key>::Storage::firstNode() const noexcept -> const Node* {
  const Node* node = this->root;
  while (node != nullptr && node->left != nullptr) node = node->left;
  return node;
}

template <typename Key>
template <typename Callable>
void Set<Key>::checkCallable(const Callable& callable) {
  if constexpr (std::is_pointer_v<Callable> || std::is_member_pointer_v<Callable>) {
    if (callable == nullptr) throw std::invalid_argument("Callable must not be null");
  }
}

template <typename Key>
Set<Key>::Set(Key key) {
  insert(std::move(key));
}

template <typename Key>
void Set<Key>::insert(Key key) {
  values.insert(std::move(key), true);
}

template <typename Key>
bool Set<Key>::contains(const Key& key) const {
  return values.containsKey(key);
}

template <typename Key>
void Set<Key>::remove(const Key& key) {
  if (values.size() == 0) throw std::out_of_range("Key does not exist");
  values.remove(key);
}

template <typename Key>
Set<Key> Set<Key>::unionWith(const Set& other) const {
  Set result;
  result.values = values;
  for (const auto* node = other.values.firstNode(); node != nullptr; node = other.values.getSuccessor(node)) {
    result.insert(node->key);
  }
  return result;
}

template <typename Key>
Set<Key> Set<Key>::intersection(const Set& other) const {
  Set result;
  for (const auto* node = values.firstNode(); node != nullptr; node = values.getSuccessor(node)) {
    if (other.contains(node->key)) result.insert(node->key);
  }
  return result;
}

template <typename Key>
Set<Key> Set<Key>::difference(const Set& other) const {
  Set result;
  for (const auto* node = values.firstNode(); node != nullptr; node = values.getSuccessor(node)) {
    if (!other.contains(node->key)) result.insert(node->key);
  }
  return result;
}

template <typename Key>
bool Set<Key>::isSubsetOf(const Set& other) const {
  if (values.size() > other.values.size()) return false;

  for (const auto* node = values.firstNode(); node != nullptr; node = values.getSuccessor(node)) {
    if (!other.contains(node->key)) return false;
  }
  return true;
}

template <typename Key>
bool Set<Key>::operator==(const Set& other) const {
  return values.size() == other.values.size() && isSubsetOf(other);
}

template <typename Key>
template <typename Transform>
Set<Key> Set<Key>::map(Transform&& transform) const {
  checkCallable(transform);
  Set result;
  for (const auto* node = values.firstNode(); node != nullptr; node = values.getSuccessor(node)) {
    result.insert(std::invoke(transform, node->key));
  }
  return result;
}

template <typename Key>
template <typename Combine, typename Accumulator>
Accumulator Set<Key>::reduce(Combine&& combine, Accumulator accumulator) const {
  checkCallable(combine);
  for (const auto* node = values.firstNode(); node != nullptr; node = values.getSuccessor(node)) {
    accumulator = std::invoke(combine, node->key, std::move(accumulator));
  }
  return accumulator;
}

template <typename Key>
template <typename Predicate>
Set<Key> Set<Key>::where(Predicate&& predicate) const {
  checkCallable(predicate);
  Set result;
  for (const auto* node = values.firstNode(); node != nullptr; node = values.getSuccessor(node)) {
    if (std::invoke(predicate, node->key)) result.insert(node->key);
  }
  return result;
}

template <typename Key>
std::string Set<Key>::formatKey(const Key& key) {
  if constexpr (std::is_same_v<Key, std::string>) {
    return key;
  } else if constexpr (std::is_integral_v<Key>) {
    return std::to_string(key);
  } else {
    static_assert(std::is_floating_point_v<Key>, "Set serialization requires string or numeric keys");
    if (std::isnan(key)) throw std::invalid_argument("NaN is not an ordered key");

    // Оставляем место для знака, точки и десятичного показателя степени.
    char buffer[std::numeric_limits<Key>::max_digits10 + 16];
    const auto written = std::to_chars(buffer, buffer + sizeof(buffer), key, std::chars_format::general);
    if (written.ec != std::errc{}) throw std::invalid_argument("Cannot write set key");
    return std::string(buffer, written.ptr);
  }
}

template <typename Key>
Key Set<Key>::parseKey(std::string_view text) {
  if constexpr (std::is_same_v<Key, std::string>) {
    return std::string(text);
  } else {
    static_assert(std::is_arithmetic_v<Key>, "Set serialization requires string or numeric keys");
    // Продвигаем узкие целые типы и bool до типа, который принимает from_chars.
    using Number = decltype(+Key{});
    Number number{};
    const char* end = text.data() + text.size();
    const auto parsed = std::from_chars(text.data(), end, number);
    if (parsed.ec != std::errc{} || parsed.ptr != end) {
      throw std::invalid_argument("Invalid set key");
    }

    if constexpr (std::is_floating_point_v<Key>) {
      if (std::isnan(number)) throw std::invalid_argument("NaN is not an ordered key");
    } else if constexpr (!std::is_same_v<Number, Key>) {
      if (number < std::numeric_limits<Key>::lowest() || number > std::numeric_limits<Key>::max()) {
        throw std::invalid_argument("Set key out of range");
      }
    }
    return static_cast<Key>(number);
  }
}

template <typename Key>
std::string Set<Key>::toString() const {
  std::string result;
  for (const auto* node = values.firstNode(); node != nullptr; node = values.getSuccessor(node)) {
    const std::string key = formatKey(node->key);
    result += std::to_string(key.size());
    result += ':';
    result += key;
  }
  return result;
}

template <typename Key>
Set<Key> Set<Key>::fromString(std::string_view text) {
  Set result;
  while (!text.empty()) {
    const std::size_t separator = text.find(':');
    if (separator == std::string_view::npos) throw std::invalid_argument("Missing set key length");

    std::size_t length = 0;
    const char* end = text.data() + separator;
    const auto parsed = std::from_chars(text.data(), end, length);
    if (parsed.ec != std::errc{} || parsed.ptr != end) {
      throw std::invalid_argument("Invalid set key length");
    }
    text.remove_prefix(separator + 1);
    if (length > text.size()) throw std::invalid_argument("Incomplete set key");

    result.insert(parseKey(text.substr(0, length)));
    text.remove_prefix(length);
  }
  return result;
}
