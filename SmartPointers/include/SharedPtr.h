#ifndef DATASTRUCTURES_SHAREDPTR_H
#define DATASTRUCTURES_SHAREDPTR_H

#include <atomic>

#include <cstddef>
#include <type_traits>
#include <utility>

// Разные экземпляры владельцев можно копировать и уничтожать параллельно.
// Доступ к одному экземпляру SharedPtr или общему объекту требует синхронизации.

// Смена типа объекта разрешена только при виртуальном деструкторе базы.
// Добавление const не требует виртуального деструктора.
template<typename T>
class SharedPtr {
private:
  static_assert(!std::is_array_v<T>, "Для массивов используйте специализацию T[]");

  struct ControlBlock {
    T* ptr;
    std::atomic<std::size_t> owners{1};
    explicit ControlBlock(T* ptr) noexcept : ptr(ptr) {}
  };

  ControlBlock* control = nullptr;
  void releaseOwnership() noexcept;

public:
  explicit SharedPtr(std::nullptr_t = nullptr) noexcept {}

  template<typename U>
  explicit SharedPtr(U* ptr);

  SharedPtr(const SharedPtr& other) noexcept;
  SharedPtr& operator=(const SharedPtr& other) noexcept;

  SharedPtr(SharedPtr&& other) noexcept;
  SharedPtr& operator=(SharedPtr&& other) noexcept;

  T* get() const noexcept;
  T& operator*() const noexcept;
  T* operator->() const noexcept;
  [[nodiscard]] std::size_t use_count() const noexcept;
  explicit operator bool() const noexcept;
  void swap(SharedPtr& other) noexcept;
  void reset(std::nullptr_t = nullptr) noexcept;

  template<typename U>
  void reset(U* newPtr);

  bool operator==(const SharedPtr& other) const noexcept;
  bool operator==(std::nullptr_t) const noexcept;

  ~SharedPtr();
};

// Проверка совместимости массивов запрещает переход от Derived[] к Base[].
template<typename T>
class SharedPtr<T[]> {
private:
  struct ControlBlock {
    T* ptr;
    std::atomic<std::size_t> owners{1};
    explicit ControlBlock(T* ptr) noexcept : ptr(ptr) {}
  };

  ControlBlock* control = nullptr;
  void releaseOwnership() noexcept;

public:
  explicit SharedPtr(std::nullptr_t = nullptr) noexcept {}

  template<typename U>

  explicit SharedPtr(U* ptr);

  SharedPtr(const SharedPtr& other) noexcept;
  SharedPtr& operator=(const SharedPtr& other) noexcept;

  SharedPtr(SharedPtr&& other) noexcept;
  SharedPtr& operator=(SharedPtr&& other) noexcept;

  T* get() const noexcept;
  T& operator[](std::size_t index) const noexcept;
  [[nodiscard]] std::size_t use_count() const noexcept;
  explicit operator bool() const noexcept;
  void swap(SharedPtr& other) noexcept;
  void reset(std::nullptr_t = nullptr) noexcept;

  template<typename U>

  void reset(U* newPtr);

  bool operator==(const SharedPtr& other) const noexcept;
  bool operator==(std::nullptr_t) const noexcept;

  ~SharedPtr();
};

template<typename T>
SharedPtr(T*) -> SharedPtr<T>;

#include "SharedPtr.tpp"

#endif // DATASTRUCTURES_SHAREDPTR_H
