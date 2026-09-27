#ifndef DATASTRUCTURES_ARRAYSEQUENCE_H
#define DATASTRUCTURES_ARRAYSEQUENCE_H

#include "Sequence.h"

template <typename T>
class ArraySequence : public Sequence<T> {
private:
  T* data;
  size_t size;
  size_t capacity;

  void reallocate(size_t newCapacity);

public:

  //
  // Конструкторы
  //

  ArraySequence();

  explicit ArraySequence(size_t initialCapacity);

  ArraySequence(const ArraySequence &other);

  ArraySequence(ArraySequence &&other) noexcept;


  //
  //  Методы
  //

  T& get(size_t index) override;

  const T& get(size_t index) const override;

  T& getFirst() override;

  T& getLast() override;

  [[nodiscard]] size_t getLength() const override;

  void append(T item) override;

  void prepend(T item) override;

  void insertAt(size_t index, T item) override;

  void removeAt(size_t index) override;

  //
  // Операторы
  //
  
  ArraySequence& operator=(const ArraySequence &other);

  ArraySequence& operator=(ArraySequence &&other) noexcept;

};

#include "ArraySequence.tpp"

#endif // DATASTRUCTURES_ARRAYSEQUENCE_H
