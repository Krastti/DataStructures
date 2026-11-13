#ifndef DATASTRUCTURES_UNIQUEPTR_H
#define DATASTRUCTURES_UNIQUEPTR_H
#include <cstddef>
#include <type_traits>
#include <utility>

template <typename T>
class UniquePtr {
private:
  template<typename>
  friend class UniquePtr;

  using ElementType = std::remove_extent_t<T>;
  using Deleter = void (*)(void*) noexcept;

  ElementType* ptr;
  void* ownedAddress;
  Deleter deleter;

  template<typename U>
  static void deleteOwned(void* address) noexcept;

  template<typename U>
  static void* eraseAddress(U* address) noexcept;

  void destroy() noexcept;

  template<typename U>
  void resetAs(U* newPtr);

public:
  explicit UniquePtr(ElementType* ptr = nullptr) noexcept;

  template<typename U>
  requires std::is_convertible_v<U*, ElementType*> &&
           (!std::is_array_v<T> || std::is_same_v<U, ElementType>)
  explicit UniquePtr(U* ptr) noexcept;

  UniquePtr(const UniquePtr&) = delete;
  UniquePtr& operator=(const UniquePtr&) = delete;

  UniquePtr(UniquePtr&& other) noexcept;

  template<typename U>
  requires (!std::is_array_v<T> && !std::is_array_v<U> &&
            std::is_convertible_v<std::remove_extent_t<U>*, ElementType*>)
  UniquePtr(UniquePtr<U>&& other) noexcept;

  UniquePtr& operator=(UniquePtr&& other) noexcept;

  template<typename U>
  requires (!std::is_array_v<T> && !std::is_array_v<U> &&
            std::is_convertible_v<std::remove_extent_t<U>*, ElementType*>)
  UniquePtr& operator=(UniquePtr<U>&& other) noexcept;

  ElementType& operator*() const noexcept requires (!std::is_array_v<T>);
  ElementType* operator->() const noexcept requires (!std::is_array_v<T>);
  ElementType& operator[](size_t index) const noexcept requires (std::is_array_v<T>);

  ElementType* get() const noexcept;
  ElementType* release() noexcept;
  void reset(ElementType* newPtr = nullptr);

  template<typename U>
  requires std::is_convertible_v<U*, ElementType*> &&
           (!std::is_array_v<T> || std::is_same_v<U, ElementType>)
  void reset(U* newPtr);

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
