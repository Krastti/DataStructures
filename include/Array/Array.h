#ifndef DATASTRUCTURES_ARRAY_H
#define DATASTRUCTURES_ARRAY_H

#include <stddef.h>
#include <stdexcept>

template<typename T>
class Array {
private:
  T* storage = nullptr;
  size_t count = 0;
  size_t allocated = 0;

public:
  Array() noexcept = default;
  Array(const Array& other);
  Array(Array&& other) noexcept;
  ~Array();

  Array& operator=(const Array& other);
  Array& operator=(Array&& other) noexcept;

  [[nodiscard]] size_t size() const noexcept;
  T& operator[](size_t index) noexcept;
  const T& operator[](size_t index) const noexcept;

  void push_back(T value);
};

#include "Array.tpp"

#endif // DATASTRUCTURES_ARRAY_H
