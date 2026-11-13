#pragma once

template <typename T>
template <typename U>
void SharedPtr<T>::deleteOwned(void* address) noexcept {
  if constexpr (std::is_array_v<T>) delete[] static_cast<U*>(address);
  else delete static_cast<U*>(address);
}

template <typename T>
SharedPtr<T>::SharedPtr(std::nullptr_t) noexcept : ptr(nullptr), control(nullptr) {}

template <typename T>
template <typename U>
requires std::is_convertible_v<U*, typename SharedPtr<T>::ElementType*> &&
         (!std::is_array_v<T> || std::is_convertible_v<U(*)[], typename SharedPtr<T>::ElementType(*)[]>)
SharedPtr<T>::SharedPtr(U* pointer) : ptr(pointer), control(nullptr) {
  if (pointer == nullptr) return;
  void* address = const_cast<void*>(static_cast<const volatile void*>(pointer));
  try {
    control = new ControlBlock(address, &deleteOwned<U>);
  } catch (...) {
    deleteOwned<U>(address);
    throw;
  }
}

template <typename T>
SharedPtr<T>::SharedPtr(const SharedPtr& other) noexcept
  : ptr(other.ptr), control(other.control) {
  if (control != nullptr) control->refCount.fetch_add(1, std::memory_order_relaxed);
}

template <typename T>
SharedPtr<T>::SharedPtr(SharedPtr&& other) noexcept
  : ptr(std::exchange(other.ptr, nullptr)), control(std::exchange(other.control, nullptr)) {}

template <typename T>
SharedPtr<T>& SharedPtr<T>::operator=(const SharedPtr& other) noexcept {
  SharedPtr replacement(other);
  swap(replacement);
  return *this;
}

template <typename T>
SharedPtr<T>& SharedPtr<T>::operator=(SharedPtr&& other) noexcept {
  SharedPtr replacement(std::move(other));
  swap(replacement);
  return *this;
}

template <typename T>
void SharedPtr<T>::destroy() noexcept {
  ControlBlock* oldControl = std::exchange(control, nullptr);
  ptr = nullptr;
  if (oldControl != nullptr &&
      oldControl->refCount.fetch_sub(1, std::memory_order_acq_rel) == 1) {
    oldControl->deleter(oldControl->ownedAddress);
    delete oldControl;
  }
}

template <typename T>
typename SharedPtr<T>::ElementType* SharedPtr<T>::get() const noexcept {
  return ptr;
}

template <typename T>
typename SharedPtr<T>::ElementType& SharedPtr<T>::operator*() const noexcept
  requires (!std::is_array_v<T>) {
  return *ptr;
}

template <typename T>
typename SharedPtr<T>::ElementType* SharedPtr<T>::operator->() const noexcept
  requires (!std::is_array_v<T>) {
  return ptr;
}

template <typename T>
typename SharedPtr<T>::ElementType& SharedPtr<T>::operator[](std::size_t index) const noexcept
  requires (std::is_array_v<T>) {
  return ptr[index];
}

template <typename T>
std::size_t SharedPtr<T>::use_count() const noexcept {
  return control == nullptr ? 0 : control->refCount.load(std::memory_order_relaxed);
}

template <typename T>
SharedPtr<T>::operator bool() const noexcept {
  return ptr != nullptr;
}

template <typename T>
void SharedPtr<T>::swap(SharedPtr& other) noexcept {
  std::swap(ptr, other.ptr);
  std::swap(control, other.control);
}

template <typename T>
void SharedPtr<T>::reset(std::nullptr_t) noexcept {
  SharedPtr replacement;
  swap(replacement);
}

template <typename T>
template <typename U>
requires std::is_convertible_v<U*, typename SharedPtr<T>::ElementType*> &&
         (!std::is_array_v<T> || std::is_convertible_v<U(*)[], typename SharedPtr<T>::ElementType(*)[]>)
void SharedPtr<T>::reset(U* newPtr) {
  if (ptr == newPtr) return;
  SharedPtr replacement(newPtr);
  swap(replacement);
}

template <typename T>
bool SharedPtr<T>::operator==(const SharedPtr& other) const noexcept {
  return ptr == other.ptr;
}

template <typename T>
bool SharedPtr<T>::operator!=(const SharedPtr& other) const noexcept {
  return !(*this == other);
}

template <typename T>
bool SharedPtr<T>::operator==(std::nullptr_t) const noexcept {
  return ptr == nullptr;
}

template <typename T>
bool SharedPtr<T>::operator!=(std::nullptr_t) const noexcept {
  return ptr != nullptr;
}

template <typename T>
bool operator==(std::nullptr_t, const SharedPtr<T>& ptr) noexcept {
  return ptr == nullptr;
}

template <typename T>
bool operator!=(std::nullptr_t, const SharedPtr<T>& ptr) noexcept {
  return ptr != nullptr;
}

template <typename T>
SharedPtr<T>::~SharedPtr() {
  destroy();
}
