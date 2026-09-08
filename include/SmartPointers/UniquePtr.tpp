#pragma once

template <typename T>
UniquePtr<T>::UniquePtr(ElementType *ptr) : ptr(ptr) { }

template <typename T>
UniquePtr<T>::UniquePtr(UniquePtr &&other) noexcept : ptr(other.ptr) {
  other.ptr = nullptr;
}

template <typename T>
UniquePtr<T>& UniquePtr<T>::operator=(UniquePtr<T> &&other) noexcept {
  if (this != &other) {
    if constexpr (std::is_array_v<T>) {
      delete[] ptr;
    } else {
      delete ptr;
    }

    ptr = other.ptr;
    other.ptr = nullptr;
  }

  return *this;
}
template <typename T>
typename UniquePtr<T>::ElementType& UniquePtr<T>::operator*() const noexcept {
  return *ptr;
}

template <typename T>
typename UniquePtr<T>::ElementType* UniquePtr<T>::operator->() const noexcept {
  return ptr;
}
template <typename T>
typename UniquePtr<T>::ElementType* UniquePtr<T>::get() const noexcept {
  return ptr;
}
template <typename T>
typename UniquePtr<T>::ElementType* UniquePtr<T>::release() noexcept {
  ElementType* temp = ptr;
  ptr = nullptr;
  return temp;
}

template <typename T>
void UniquePtr<T>::reset(ElementType* newPtr) {
  if (this->ptr != newPtr) {
    if constexpr (std::is_array_v<T>) {
      delete[] this->ptr;
    } else {
      delete this->ptr;
    }
  }
  this->ptr = newPtr;
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
  if constexpr (std::is_array_v<T>) {
    delete[] ptr;
  } else {
    delete ptr;
  }
}
