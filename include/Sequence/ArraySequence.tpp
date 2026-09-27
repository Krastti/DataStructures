#pragma once

template <typename T>
void ArraySequence<T>::reallocate(const size_t newCapacity) { capacity = newCapacity; }

template <typename T>
ArraySequence<T>::ArraySequence() : size(0), capacity(0) { data = new T[capacity]; }

template <typename T>
ArraySequence<T>::ArraySequence(size_t initialCapacity) : size(initialCapacity), capacity(initialCapacity) { data = new T[capacity]; }

template <typename T>
ArraySequence<T>::ArraySequence(const ArraySequence &other) {}

template <typename T>
ArraySequence<T>::ArraySequence(ArraySequence &&other) noexcept {}

template <typename T>
T &ArraySequence<T>::get(size_t index) { return data[index]; }

template <typename T>
const T &ArraySequence<T>::get(size_t index) const { return data[index]; }

template <typename T>
T &ArraySequence<T>::getFirst() { return data[0]; }

template <typename T>
T &ArraySequence<T>::getLast() { return data[size - 1]; }

template <typename T>
size_t ArraySequence<T>::getLength() const { return size; }

template <typename T>
void ArraySequence<T>::append(T item) { data[size++] = item; }

template <typename T>
void ArraySequence<T>::prepend(T item) {
  for (int i = 1; i < size; i++) {
    data[i] = data[i - 1];
  }
  data[0] = item;
}

template <typename T>
void ArraySequence<T>::insertAt(size_t index, T item) { data[index] = item; }

template <typename T>
void ArraySequence<T>::removeAt(size_t index) { }

template <typename T>
ArraySequence<T> &ArraySequence<T>::operator=(const ArraySequence &other) {}

template <typename T>
ArraySequence<T> &ArraySequence<T>::operator=(ArraySequence &&other) noexcept {}







