#ifndef DATASTRUCTURES_UNIQUEPTR_H
#define DATASTRUCTURES_UNIQUEPTR_H

#include <cstddef>
#include <type_traits>
#include <utility>

// Смена типа объекта разрешена только при виртуальном деструкторе базы.
template<typename T>
class UniquePtr {
private:
  static_assert(!std::is_array_v<T>, "Для массивов используйте специализацию T[]");

  T* ptr = nullptr;

public:
  explicit UniquePtr(std::nullptr_t = nullptr) noexcept {}

  template<typename U>
  explicit UniquePtr(U* ptr) noexcept;

  UniquePtr(const UniquePtr& other) = delete;
  UniquePtr& operator=(const UniquePtr& other) = delete;
  UniquePtr(UniquePtr&& other) noexcept;
  UniquePtr& operator=(UniquePtr&& other) noexcept;

  template<typename U>
  explicit UniquePtr(UniquePtr<U>&& other) noexcept;

  template<typename U>
  UniquePtr& operator=(UniquePtr<U>&& other) noexcept;

  T* get() const noexcept;
  T& operator*() const noexcept;
  T* operator->() const noexcept;
  [[nodiscard]] T* release() noexcept;
  explicit operator bool() const noexcept;
  void swap(UniquePtr& other) noexcept;
  void reset(std::nullptr_t = nullptr) noexcept;

  template<typename U>
  void reset(U* newPtr) noexcept;

  bool operator==(const UniquePtr& other) const noexcept;
  bool operator==(std::nullptr_t) const noexcept;

  ~UniquePtr();
};

// Проверка совместимости массивов запрещает переход от Derived[] к Base[].
template<typename T>
class UniquePtr<T[]> {
private:
  T* ptr = nullptr;

public:
  explicit UniquePtr(std::nullptr_t = nullptr) noexcept {}

  template<typename U>
  explicit UniquePtr(U* ptr) noexcept;

  UniquePtr(const UniquePtr& other) = delete;
  UniquePtr& operator=(const UniquePtr& other) = delete;

  UniquePtr(UniquePtr&& other) noexcept;
  UniquePtr& operator=(UniquePtr&& other) noexcept;

  T* get() const noexcept;
  T& operator[](std::size_t index) const noexcept;
  [[nodiscard]] T* release() noexcept;
  explicit operator bool() const noexcept;
  void swap(UniquePtr& other) noexcept;
  void reset(std::nullptr_t = nullptr) noexcept;

  template<typename U>

  void reset(U* newPtr) noexcept;

  bool operator==(const UniquePtr& other) const noexcept;
  bool operator==(std::nullptr_t) const noexcept;

  ~UniquePtr();
};

template<typename T>
UniquePtr(T*) -> UniquePtr<T>;

#include "UniquePtr.tpp"

#endif // DATASTRUCTURES_UNIQUEPTR_H
