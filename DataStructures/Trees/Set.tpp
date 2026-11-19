#pragma once
#include <stdexcept>
#include <utility>

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
