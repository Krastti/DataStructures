#pragma once

template<typename T>
template<typename U>

SharedPtr<T>::SharedPtr(U* pointer) {
  static_assert(!std::is_array_v<U>, "Нельзя передать массив владельцу одиночного объекта");
  static_assert(std::is_convertible_v<U*, T*>, "Несовместимые типы указателей");
  if constexpr (!std::is_same_v<std::remove_cv_t<U>, std::remove_cv_t<T>>) {
    static_assert(std::has_virtual_destructor_v<T>, "Базовый тип должен иметь виртуальный деструктор");
  }
  if (pointer == nullptr) return;
  try {
    control = new ControlBlock(pointer);
  } catch (...) {
    delete pointer;
    throw;
  }
}

template<typename T>
SharedPtr<T>::SharedPtr(const SharedPtr& other) noexcept {
  control = other.control;
  if (control != nullptr) ++control->owners;
}

template<typename T>
SharedPtr<T>& SharedPtr<T>::operator=(const SharedPtr& other) noexcept {
  SharedPtr replacement(other);
  swap(replacement);
  return *this;
}

template<typename T>
SharedPtr<T>::SharedPtr(SharedPtr&& other) noexcept {
  control = std::exchange(other.control, nullptr);
}

template<typename T>
SharedPtr<T>& SharedPtr<T>::operator=(SharedPtr&& other) noexcept {
  SharedPtr replacement(std::move(other));
  swap(replacement);
  return *this;
}

template<typename T>
void SharedPtr<T>::releaseOwnership() noexcept {
  ControlBlock* old = std::exchange(control, nullptr);
  if (old != nullptr && --old->owners == 0) {
    delete old->ptr;
    delete old;
  }
}

template<typename T>
T* SharedPtr<T>::get() const noexcept {
  return control == nullptr ? nullptr : control->ptr;
}

template<typename T>
T& SharedPtr<T>::operator*() const noexcept {
  return *get();
}

template<typename T>
T* SharedPtr<T>::operator->() const noexcept {
  return get();
}

template<typename T>
std::size_t SharedPtr<T>::use_count() const noexcept {
  return control == nullptr ? 0 : control->owners.load();
}

template<typename T>
SharedPtr<T>::operator bool() const noexcept {
  return get() != nullptr;
}

template<typename T>
void SharedPtr<T>::swap(SharedPtr& other) noexcept {
  std::swap(control, other.control);
}

template<typename T>
void SharedPtr<T>::reset(std::nullptr_t) noexcept {
  SharedPtr replacement;
  swap(replacement);
}

template<typename T>
template<typename U>

void SharedPtr<T>::reset(U* newPtr) {
  static_assert(!std::is_array_v<U>, "Нельзя передать массив владельцу одиночного объекта");
  static_assert(std::is_convertible_v<U*, T*>, "Несовместимые типы указателей");
  if constexpr (!std::is_same_v<std::remove_cv_t<U>, std::remove_cv_t<T>>) {
    static_assert(std::has_virtual_destructor_v<T>, "Базовый тип должен иметь виртуальный деструктор");
  }
  if (get() == newPtr) return;
  SharedPtr replacement(newPtr);
  swap(replacement);
}

template<typename T>
bool SharedPtr<T>::operator==(const SharedPtr& other) const noexcept {
  return get() == other.get();
}

template<typename T>
bool SharedPtr<T>::operator==(std::nullptr_t) const noexcept {
  return get() == nullptr;
}

template<typename T>
SharedPtr<T>::~SharedPtr() {
  releaseOwnership();
}

template<typename T>
template<typename U>

SharedPtr<T[]>::SharedPtr(U* pointer) {
  static_assert(std::is_convertible_v<U(*)[], T(*)[]>, "Несовместимые типы массивов");
  if (pointer == nullptr) return;
  try {
    control = new ControlBlock(pointer);
  } catch (...) {
    delete[] pointer;
    throw;
  }
}

template<typename T>
SharedPtr<T[]>::SharedPtr(const SharedPtr& other) noexcept {
  control = other.control;
  if (control != nullptr) ++control->owners;
}

template<typename T>
SharedPtr<T[]>& SharedPtr<T[]>::operator=(const SharedPtr& other) noexcept {
  SharedPtr replacement(other);
  swap(replacement);
  return *this;
}

template<typename T>
SharedPtr<T[]>::SharedPtr(SharedPtr&& other) noexcept {
  control = std::exchange(other.control, nullptr);
}

template<typename T>
SharedPtr<T[]>& SharedPtr<T[]>::operator=(SharedPtr&& other) noexcept {
  SharedPtr replacement(std::move(other));
  swap(replacement);
  return *this;
}

template<typename T>
void SharedPtr<T[]>::releaseOwnership() noexcept {
  ControlBlock* old = std::exchange(control, nullptr);
  if (old != nullptr && --old->owners == 0) {
    delete[] old->ptr;
    delete old;
  }
}

template<typename T>
T* SharedPtr<T[]>::get() const noexcept {
  return control == nullptr ? nullptr : control->ptr;
}

template<typename T>
T& SharedPtr<T[]>::operator[](std::size_t index) const noexcept {
  return get()[index];
}

template<typename T>
std::size_t SharedPtr<T[]>::use_count() const noexcept {
  return control == nullptr ? 0 : control->owners.load();
}

template<typename T>
SharedPtr<T[]>::operator bool() const noexcept {
  return get() != nullptr;
}

template<typename T>
void SharedPtr<T[]>::swap(SharedPtr& other) noexcept {
  std::swap(control, other.control);
}

template<typename T>
void SharedPtr<T[]>::reset(std::nullptr_t) noexcept {
  SharedPtr replacement;
  swap(replacement);
}

template<typename T>
template<typename U>

void SharedPtr<T[]>::reset(U* newPtr) {
  static_assert(std::is_convertible_v<U(*)[], T(*)[]>, "Несовместимые типы массивов");
  if (get() == newPtr) return;
  SharedPtr replacement(newPtr);
  swap(replacement);
}

template<typename T>
bool SharedPtr<T[]>::operator==(const SharedPtr& other) const noexcept {
  return get() == other.get();
}

template<typename T>
bool SharedPtr<T[]>::operator==(std::nullptr_t) const noexcept {
  return get() == nullptr;
}

template<typename T>
SharedPtr<T[]>::~SharedPtr() {
  releaseOwnership();
}
