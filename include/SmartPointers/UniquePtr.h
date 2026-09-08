#ifndef DATASTRUCTURES_UNIQUEPTR_H
#define DATASTRUCTURES_UNIQUEPTR_H
#include <cstddef>
#include <type_traits>

template <typename T>
class UniquePtr {
private:
  using ElementType = std::remove_extent_t<T>;

  ElementType* ptr;

public:
  explicit UniquePtr(ElementType* ptr = nullptr);

  UniquePtr(const UniquePtr&) = delete;
  UniquePtr& operator=(const UniquePtr&) = delete;

  UniquePtr(UniquePtr &&other) noexcept;
  UniquePtr& operator=(UniquePtr &&other) noexcept;

  ElementType& operator*() const noexcept;
  ElementType* operator->() const noexcept;

  ElementType* get() const noexcept;
  ElementType* release() noexcept;
  void reset(ElementType* newPtr = nullptr);

  bool operator==(const UniquePtr &other) const noexcept;
  bool operator!=(const UniquePtr &other) const noexcept;

  bool operator==(std::nullptr_t) const noexcept;
  bool operator!=(std::nullptr_t) const noexcept;

  ~UniquePtr();
};

template <typename T>
UniquePtr(T*) -> UniquePtr<T>;

template <typename T>
bool operator==(std::nullptr_t, const UniquePtr<T> &ptr) noexcept;

template <typename T>
bool operator!=(std::nullptr_t, const UniquePtr<T> &ptr) noexcept;

#include "UniquePtr.tpp"

#endif // DATASTRUCTURES_UNIQUEPTR_H
