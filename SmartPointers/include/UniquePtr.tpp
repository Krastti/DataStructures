#pragma once

template<typename T>
template<typename U>

UniquePtr<T>::UniquePtr(U* ptr) noexcept {
  static_assert(!std::is_array_v<U>, "Нельзя передать массив владельцу одиночного объекта");
  static_assert(std::is_convertible_v<U*, T*>, "Несовместимые типы указателей");
  if constexpr (!std::is_same_v<std::remove_cv_t<U>, std::remove_cv_t<T>>) {
    static_assert(std::has_virtual_destructor_v<T>, "Базовый тип должен иметь виртуальный деструктор");
  }
  this->ptr = ptr;
}

template<typename T>
UniquePtr<T>::UniquePtr(UniquePtr&& other) noexcept {
  ptr = other.release();
}

template<typename T>
UniquePtr<T>& UniquePtr<T>::operator=(UniquePtr&& other) noexcept {
  UniquePtr replacement(std::move(other));
  swap(replacement);
  return *this;
}

template<typename T>
template<typename U>

UniquePtr<T>::UniquePtr(UniquePtr<U>&& other) noexcept {
  static_assert(!std::is_array_v<U>, "Нельзя передать массив владельцу одиночного объекта");
  static_assert(std::is_convertible_v<U*, T*>, "Несовместимые типы указателей");
  if constexpr (!std::is_same_v<std::remove_cv_t<U>, std::remove_cv_t<T>>) {
    static_assert(std::has_virtual_destructor_v<T>, "Базовый тип должен иметь виртуальный деструктор");
  }
  ptr = other.release();
}

template<typename T>
template<typename U>

UniquePtr<T>& UniquePtr<T>::operator=(UniquePtr<U>&& other) noexcept {
  static_assert(!std::is_array_v<U>, "Нельзя передать массив владельцу одиночного объекта");
  static_assert(std::is_convertible_v<U*, T*>, "Несовместимые типы указателей");
  if constexpr (!std::is_same_v<std::remove_cv_t<U>, std::remove_cv_t<T>>) {
    static_assert(std::has_virtual_destructor_v<T>, "Базовый тип должен иметь виртуальный деструктор");
  }
  UniquePtr replacement(std::move(other));
  swap(replacement);
  return *this;
}

template<typename T>
T* UniquePtr<T>::get() const noexcept {
  return ptr;
}

template<typename T>
T& UniquePtr<T>::operator*() const noexcept {
  return *get();
}

template<typename T>
T* UniquePtr<T>::operator->() const noexcept {
  return get();
}

template<typename T>
T* UniquePtr<T>::release() noexcept {
  return std::exchange(ptr, nullptr);
}

template<typename T>
UniquePtr<T>::operator bool() const noexcept {
  return get() != nullptr;
}

template<typename T>
void UniquePtr<T>::swap(UniquePtr& other) noexcept {
  std::swap(ptr, other.ptr);
}

template<typename T>
void UniquePtr<T>::reset(std::nullptr_t) noexcept {
  UniquePtr replacement;
  swap(replacement);
}

template<typename T>
template<typename U>

void UniquePtr<T>::reset(U* newPtr) noexcept {
  static_assert(!std::is_array_v<U>, "Нельзя передать массив владельцу одиночного объекта");
  static_assert(std::is_convertible_v<U*, T*>, "Несовместимые типы указателей");
  if constexpr (!std::is_same_v<std::remove_cv_t<U>, std::remove_cv_t<T>>) {
    static_assert(std::has_virtual_destructor_v<T>, "Базовый тип должен иметь виртуальный деструктор");
  }
  if (get() == newPtr) return;
  UniquePtr replacement(newPtr);
  swap(replacement);
}

template<typename T>
bool UniquePtr<T>::operator==(const UniquePtr& other) const noexcept {
  return get() == other.get();
}

template<typename T>
bool UniquePtr<T>::operator==(std::nullptr_t) const noexcept {
  return get() == nullptr;
}

template<typename T>
UniquePtr<T>::~UniquePtr() {
  delete ptr;
}

template<typename T>
template<typename U>

UniquePtr<T[]>::UniquePtr(U* ptr) noexcept {
  static_assert(std::is_convertible_v<U(*)[], T(*)[]>, "Несовместимые типы массивов");
  this->ptr = ptr;
}

template<typename T>
UniquePtr<T[]>::UniquePtr(UniquePtr&& other) noexcept {
  ptr = other.release();
}

template<typename T>
UniquePtr<T[]>& UniquePtr<T[]>::operator=(UniquePtr&& other) noexcept {
  UniquePtr replacement(std::move(other));
  swap(replacement);
  return *this;
}

template<typename T>
T* UniquePtr<T[]>::get() const noexcept {
  return ptr;
}

template<typename T>
T& UniquePtr<T[]>::operator[](std::size_t index) const noexcept {
  return get()[index];
}

template<typename T>
T* UniquePtr<T[]>::release() noexcept {
  return std::exchange(ptr, nullptr);
}

template<typename T>
UniquePtr<T[]>::operator bool() const noexcept {
  return get() != nullptr;
}

template<typename T>
void UniquePtr<T[]>::swap(UniquePtr& other) noexcept {
  std::swap(ptr, other.ptr);
}

template<typename T>
void UniquePtr<T[]>::reset(std::nullptr_t) noexcept {
  UniquePtr replacement;
  swap(replacement);
}

template<typename T>
template<typename U>

void UniquePtr<T[]>::reset(U* newPtr) noexcept {
  static_assert(std::is_convertible_v<U(*)[], T(*)[]>, "Несовместимые типы массивов");
  if (get() == newPtr) return;
  UniquePtr replacement(newPtr);
  swap(replacement);
}

template<typename T>
bool UniquePtr<T[]>::operator==(const UniquePtr& other) const noexcept {
  return get() == other.get();
}

template<typename T>
bool UniquePtr<T[]>::operator==(std::nullptr_t) const noexcept {
  return get() == nullptr;
}

template<typename T>
UniquePtr<T[]>::~UniquePtr() {
  delete[] ptr;
}
