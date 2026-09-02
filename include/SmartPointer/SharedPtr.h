#ifndef DATASTRUCTURES_SHAREDPTR_H
#define DATASTRUCTURES_SHAREDPTR_H

#include <cstddef>

template <typename T>
class SharedPtr {
private:
  T* ptr;
  size_t* refCount;

public:
  SharedPtr();
  explicit SharedPtr(T* ptr);

  SharedPtr(const SharedPtr &other);
  SharedPtr(SharedPtr &&other) noexcept;

  SharedPtr& operator=(const SharedPtr &other);
  SharedPtr& operator=(SharedPtr &&other) noexcept;

  ~SharedPtr();
};

#include "SharedPtr.tpp"

#endif // DATASTRUCTURES_SHAREDPTR_H
