#pragma once

template<typename T>
Array<T>::Array(const Array& other) {
  if (other.allocated == 0) return;

  storage = new T[other.allocated];
  allocated = other.allocated;

  try {
    for (; count < other.count; ++count) storage[count] = other.storage[count];
  } catch (...) {
    delete[] storage;
    storage = nullptr;
    allocated = 0;
    throw;
  }
}

template<typename T>
Array<T>::Array(Array&& other) noexcept : storage(other.storage), count(other.count), allocated(other.allocated) {
  other.storage = nullptr;
  other.count = 0;
  other.allocated = 0;
}

template<typename T>
Array<T>::~Array() {
  delete[] storage;
}

template<typename T>
Array<T>& Array<T>::operator=(const Array& other) {
  if (this == &other) return *this;

  Array copy(other);
  *this = static_cast<Array&&>(copy);
  return *this;
}

template<typename T>
Array<T>& Array<T>::operator=(Array&& other) noexcept {
  if (this == &other) return *this;

  delete[] storage;
  storage = other.storage;
  count = other.count;
  allocated = other.allocated;
  other.storage = nullptr;
  other.count = 0;
  other.allocated = 0;
  return *this;
}

template<typename T>
size_t Array<T>::size() const noexcept {
  return count;
}

template<typename T>
T& Array<T>::operator[](size_t index) noexcept {
  return storage[index];
}

template<typename T>
const T& Array<T>::operator[](size_t index) const noexcept {
  return storage[index];
}

template<typename T>
void Array<T>::push_back(T value) {
  if (allocated != 0 && count < allocated - allocated / 4) {
    storage[count] = value;
    ++count;
    return;
  }

  constexpr size_t maxSize = static_cast<size_t>(-1);
  if (allocated > maxSize / 2) throw std::length_error("Array capacity overflow");

  const size_t newCapacity = allocated == 0 ? 4 : allocated * 2;
  if (newCapacity > maxSize / sizeof(T)) throw std::length_error("Array capacity overflow");

  T* newStorage = new T[newCapacity];
  try {
    for (size_t i = 0; i < count; ++i) newStorage[i] = storage[i];
    newStorage[count] = value;
  } catch (...) {
    delete[] newStorage;
    throw;
  }

  delete[] storage;
  storage = newStorage;
  allocated = newCapacity;
  ++count;
}
