#pragma once

template <typename T>
template <typename U>
void UniquePtr<T>::deleteOwned(void* address) noexcept {
  if constexpr (std::is_array_v<T>) delete[] static_cast<U*>(address);
  else delete static_cast<U*>(address);
}

template <typename T>
template <typename U>
void* UniquePtr<T>::eraseAddress(U* address) noexcept {
  return const_cast<void*>(static_cast<const volatile void*>(address));
}

template <typename T>
void UniquePtr<T>::destroy() noexcept {
  void* address = std::exchange(ownedAddress, nullptr);
  ptr = nullptr;
  if (address != nullptr) deleter(address);
}

template <typename T>
template <typename U>
void UniquePtr<T>::resetAs(U* newPtr) noexcept {
  ElementType* convertedPtr = newPtr;
  if (ptr == convertedPtr) return;
  UniquePtr replacement(newPtr);
  swap(replacement);
}

template <typename T>
UniquePtr<T>::UniquePtr(std::nullptr_t) noexcept
  : ptr(nullptr), ownedAddress(nullptr), deleter(&deleteOwned<ElementType>) {}

template <typename T>
template <typename U>
requires std::is_convertible_v<U*, typename UniquePtr<T>::ElementType*> &&
         (!std::is_array_v<T> || std::is_convertible_v<U(*)[], typename UniquePtr<T>::ElementType(*)[]>)
UniquePtr<T>::UniquePtr(U* ptr) noexcept
  : ptr(ptr), ownedAddress(eraseAddress(ptr)), deleter(&deleteOwned<U>) {}

template <typename T>
UniquePtr<T>::UniquePtr(UniquePtr&& other) noexcept
  : ptr(other.ptr), ownedAddress(other.ownedAddress), deleter(other.deleter) {
  other.ptr = nullptr;
  other.ownedAddress = nullptr;
  other.deleter = &UniquePtr::template deleteOwned<ElementType>;
}

template <typename T>
template <typename U>
requires (!std::is_array_v<T> && !std::is_array_v<U> &&
          std::is_convertible_v<std::remove_extent_t<U>*, typename UniquePtr<T>::ElementType*>)
UniquePtr<T>::UniquePtr(UniquePtr<U>&& other) noexcept
  : ptr(other.ptr), ownedAddress(other.ownedAddress), deleter(other.deleter) {
  other.ptr = nullptr;
  other.ownedAddress = nullptr;
  other.deleter = &UniquePtr<U>::template deleteOwned<typename UniquePtr<U>::ElementType>;
}

template <typename T>
UniquePtr<T>& UniquePtr<T>::operator=(UniquePtr&& other) noexcept {
  UniquePtr replacement(std::move(other));
  swap(replacement);
  return *this;
}

template <typename T>
template <typename U>
requires (!std::is_array_v<T> && !std::is_array_v<U> &&
          std::is_convertible_v<std::remove_extent_t<U>*, typename UniquePtr<T>::ElementType*>)
UniquePtr<T>& UniquePtr<T>::operator=(UniquePtr<U>&& other) noexcept {
  UniquePtr replacement(std::move(other));
  swap(replacement);
  return *this;
}
template <typename T>
typename UniquePtr<T>::ElementType& UniquePtr<T>::operator*() const noexcept
  requires (!std::is_array_v<T>) {
  return *ptr;
}

template <typename T>
typename UniquePtr<T>::ElementType* UniquePtr<T>::operator->() const noexcept
  requires (!std::is_array_v<T>) {
  return ptr;
}

template <typename T>
typename UniquePtr<T>::ElementType& UniquePtr<T>::operator[](size_t index) const noexcept
  requires (std::is_array_v<T>) {
  return ptr[index];
}
template <typename T>
typename UniquePtr<T>::ElementType* UniquePtr<T>::get() const noexcept {
  return ptr;
}
template <typename T>
typename UniquePtr<T>::ElementType* UniquePtr<T>::release() noexcept {
  ElementType* temp = ptr;
  ptr = nullptr;
  ownedAddress = nullptr;
  deleter = &deleteOwned<ElementType>;
  return temp;
}

template <typename T>
void UniquePtr<T>::swap(UniquePtr& other) noexcept {
  std::swap(ptr, other.ptr);
  std::swap(ownedAddress, other.ownedAddress);
  std::swap(deleter, other.deleter);
}

template <typename T>
UniquePtr<T>::operator bool() const noexcept {
  return ptr != nullptr;
}

template <typename T>
void UniquePtr<T>::reset(std::nullptr_t) noexcept {
  UniquePtr replacement;
  swap(replacement);
}

template <typename T>
template <typename U>
requires std::is_convertible_v<U*, typename UniquePtr<T>::ElementType*> &&
         (!std::is_array_v<T> || std::is_convertible_v<U(*)[], typename UniquePtr<T>::ElementType(*)[]>)
void UniquePtr<T>::reset(U* newPtr) noexcept {
  resetAs(newPtr);
}

template <typename T>
bool UniquePtr<T>::operator==(const UniquePtr &other) const noexcept {
  return ptr == other.ptr;
}

template <typename T>
bool UniquePtr<T>::operator!=(const UniquePtr &other) const noexcept {
  return !(*this == other);
}

template <typename T>
bool UniquePtr<T>::operator==(std::nullptr_t) const noexcept {
  return ptr == nullptr;
}

template <typename T>
bool UniquePtr<T>::operator!=(std::nullptr_t) const noexcept {
  return ptr != nullptr;
}

template <typename T>
bool operator==(std::nullptr_t, const UniquePtr<T> &ptr) noexcept {
  return ptr == nullptr;
}

template <typename T>
bool operator!=(std::nullptr_t, const UniquePtr<T> &ptr) noexcept {
  return ptr != nullptr;
}

template <typename T>
UniquePtr<T>::~UniquePtr() {
  destroy();
}
