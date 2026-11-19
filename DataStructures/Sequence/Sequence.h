#ifndef DATASTRUCTURES_SEQUENCE_H
#define DATASTRUCTURES_SEQUENCE_H

#include <cstddef>

template <typename T>
class Sequence {
public:
  virtual T& get(size_t index) = 0;

  virtual const T& get(size_t index) const = 0;

  virtual T& getFirst() = 0;

  virtual T& getLast() = 0;

  virtual size_t getLength() const = 0;

  virtual void append(T item) = 0;

  virtual void prepend(T item) = 0;

  virtual void insertAt(T item, size_t index) = 0;

  virtual void removeAt(size_t index) = 0;

  virtual ~Sequence() = default;
};

#endif // DATASTRUCTURES_SEQUENCE_H
