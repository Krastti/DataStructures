#ifndef DATASTRUCTURES_SET_H
#define DATASTRUCTURES_SET_H

#include "RedBlackTree.h"

template <typename Key>
class Set {
private:
  Map<Key, bool> values;

public:
  Set() = default;
  explicit Set(Key key);
  Set(const Set&) = delete;
  Set& operator=(const Set&) = delete;

  void insert(Key key);
  void remove(const Key& key);
  [[nodiscard]] bool contains(const Key& key) const;

  ~Set() = default;
};

#include "Set.tpp"

#endif // DATASTRUCTURES_SET_H
