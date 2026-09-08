#ifndef DATASTRUCTURES_SHAREDPTR_H
#define DATASTRUCTURES_SHAREDPTR_H

#include <cstddef>
#include <type_traits>

template <typename T>
class SharedPtr {
private:
  using ElementType = std::remove_extent_t<T>;

  ElementType* ptr;
  size_t* refCount;

public:
  SharedPtr();
  explicit SharedPtr(ElementType* ptr);

  SharedPtr(const SharedPtr &other);
  SharedPtr(SharedPtr &&other) noexcept;

  SharedPtr& operator=(const SharedPtr &other);
  SharedPtr& operator=(SharedPtr &&other) noexcept;

  ElementType* get() const;

  ElementType& operator*() const;

  ElementType* operator->() const;

  [[nodiscard]] int use_count() const;

  void reset(ElementType* newPtr = nullptr);

  bool operator==(const SharedPtr &other) const noexcept;
  bool operator!=(const SharedPtr &other) const noexcept;

  bool operator==(std::nullptr_t) const noexcept;
  bool operator!=(std::nullptr_t) const noexcept;

  ~SharedPtr();
};

template <typename T>
bool operator==(std::nullptr_t, const SharedPtr<T> &ptr) noexcept;

template <typename T>
bool operator!=(std::nullptr_t, const SharedPtr<T> &ptr) noexcept;

#include "SharedPtr.tpp"

#endif // DATASTRUCTURES_SHAREDPTR_H
