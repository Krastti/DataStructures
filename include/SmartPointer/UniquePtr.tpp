#pragma once

template <typename T>
UniquePtr<T>::UniquePtr(T *ptr) : ptr(ptr) { }

template <typename T>
UniquePtr<T>::UniquePtr(UniquePtr &&other) noexcept : ptr(other.ptr) {
  other.ptr = nullptr;
}

template <typename T>
UniquePtr<T>& UniquePtr<T>::operator=(UniquePtr<T> &&other) noexcept {
  if (this != &other) {
    delete ptr;

    ptr = other.ptr;
    other.ptr = nullptr;
  }

  return *this;
}
template <typename T>
T& UniquePtr<T>::operator*() const noexcept {
  return *ptr;
}

template <typename T>
T* UniquePtr<T>::operator->() const noexcept {
  return ptr;
}
template <typename T>
T* UniquePtr<T>::get() const noexcept {
  return ptr;
}
template <typename T>
T* UniquePtr<T>::release() noexcept {
  T* temp = ptr;
  ptr = nullptr;
  return temp;

}

template <typename T>
void UniquePtr<T>::reset(T* newPtr) {
  if (this->ptr != newPtr) {
    delete this->ptr;
  }

  this->ptr = newPtr;
}

template <typename T>
UniquePtr<T>::~UniquePtr() {
  delete ptr;
}

