#ifndef DATASTRUCTURES_SET_H
#define DATASTRUCTURES_SET_H

#include "RedBlackTree.h"

#include <string>
#include <string_view>

template <typename Key>
class Set {
private:
  class Storage : public Map<Key, bool> {
    using Node = typename BinaryTree<Key, bool>::Node;

  public:
    using BinaryTree<Key, bool>::getSuccessor;
    [[nodiscard]] const Node* firstNode() const noexcept;
  };

  Storage values;

  template <typename Callable>
  static void checkCallable(const Callable& callable);

  static std::string formatKey(const Key& key);
  static Key parseKey(std::string_view text);

public:
  Set() = default;
  explicit Set(Key key);

  Set(const Set&) = delete;
  Set& operator=(const Set&) = delete;

  Set(Set&&) noexcept = default;
  Set& operator=(Set&&) noexcept = default;

  void insert(Key key);
  void remove(const Key& key);

  [[nodiscard]] bool contains(const Key& key) const;

  // Объединение
  [[nodiscard]] Set unionWith(const Set& other) const;

  // Пересечение
  [[nodiscard]] Set intersection(const Set& other) const;

  // Вычитание
  [[nodiscard]] Set difference(const Set& other) const;

  // Проверка включения
  [[nodiscard]] bool isSubsetOf(const Set& other) const;

  // Равенство множеств
  [[nodiscard]] bool operator==(const Set& other) const;

  // Преобразование
  template <typename Transform>
  [[nodiscard]] Set map(Transform&& transform) const;

  // Свёртка
  template <typename Combine, typename Accumulator>
  [[nodiscard]] Accumulator reduce(Combine&& combine, Accumulator accumulator) const;

  // Фильтрация
  template <typename Predicate>
  [[nodiscard]] Set where(Predicate&& predicate) const;

  // Сохранение в строку
  [[nodiscard]] std::string toString() const;

  // Чтение из строки
  [[nodiscard]] static Set fromString(std::string_view text);

  ~Set() = default;
};

#include "Set.tpp"

#endif // DATASTRUCTURES_SET_H
