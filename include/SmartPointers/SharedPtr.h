#ifndef DATASTRUCTURES_SHAREDPTR_H
#define DATASTRUCTURES_SHAREDPTR_H

#include <atomic>
#include <cstddef>
#include <type_traits>
#include <utility>

// Separate owners can be copied/destroyed concurrently. Mutating the same
// SharedPtr instance or the managed object still requires synchronization.
template <typename T>
class SharedPtr {
private:
  using ElementType = std::remove_extent_t<T>;
  using Deleter = void (*)(void*) noexcept;

  struct ControlBlock {
    std::atomic<std::size_t> refCount{1};
    void* ownedAddress;
    Deleter deleter;

    ControlBlock(void* address, Deleter destroy) noexcept
      : ownedAddress(address), deleter(destroy) {}
  };

  ElementType* ptr;
  ControlBlock* control;

  template<typename U>
  static void deleteOwned(void* address) noexcept;

  void destroy() noexcept;

public:
  SharedPtr(std::nullptr_t = nullptr) noexcept;

  template<typename U>
  requires std::is_convertible_v<U*, ElementType*> &&
           (!std::is_array_v<T> || std::is_convertible_v<U(*)[], ElementType(*)[]>)
  explicit SharedPtr(U* ptr);

  SharedPtr(const SharedPtr& other) noexcept;
  SharedPtr(SharedPtr&& other) noexcept;

  SharedPtr& operator=(const SharedPtr& other) noexcept;
  SharedPtr& operator=(SharedPtr&& other) noexcept;

  ElementType* get() const noexcept;
  ElementType& operator*() const noexcept requires (!std::is_array_v<T>);
  ElementType* operator->() const noexcept requires (!std::is_array_v<T>);
  ElementType& operator[](std::size_t index) const noexcept requires (std::is_array_v<T>);

  [[nodiscard]] std::size_t use_count() const noexcept;
  explicit operator bool() const noexcept;
  void swap(SharedPtr& other) noexcept;
  void reset(std::nullptr_t = nullptr) noexcept;

  template<typename U>
  requires std::is_convertible_v<U*, ElementType*> &&
           (!std::is_array_v<T> || std::is_convertible_v<U(*)[], ElementType(*)[]>)
  void reset(U* newPtr);

  bool operator==(const SharedPtr& other) const noexcept;
  bool operator!=(const SharedPtr& other) const noexcept;
  bool operator==(std::nullptr_t) const noexcept;
  bool operator!=(std::nullptr_t) const noexcept;

  ~SharedPtr();
};

template <typename T>
SharedPtr(T*) -> SharedPtr<T>;

template <typename T>
bool operator==(std::nullptr_t, const SharedPtr<T>& ptr) noexcept;

template <typename T>
bool operator!=(std::nullptr_t, const SharedPtr<T>& ptr) noexcept;

#include "SharedPtr.tpp"

#endif // DATASTRUCTURES_SHAREDPTR_H
