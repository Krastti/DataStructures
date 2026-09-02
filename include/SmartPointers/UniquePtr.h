#ifndef DATASTRUCTURES_UNIQUEPTR_H
#define DATASTRUCTURES_UNIQUEPTR_H

template <typename T>
class UniquePtr {
private:
  T* ptr;

public:
  explicit UniquePtr(T* ptr = nullptr);

  UniquePtr(const UniquePtr&) = delete;
  UniquePtr& operator=(const UniquePtr&) = delete;

  UniquePtr(UniquePtr &&other) noexcept;
  UniquePtr& operator=(UniquePtr &&other) noexcept;

  T& operator*() const noexcept;
  T *operator->() const noexcept;

  T* get() const noexcept;
  T* release() noexcept;
  void reset(T* newPtr = nullptr);

  ~UniquePtr();
};

#include "UniquePtr.tpp"

#endif // DATASTRUCTURES_UNIQUEPTR_H
