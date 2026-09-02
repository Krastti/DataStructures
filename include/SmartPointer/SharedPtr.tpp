#pragma once

template <typename T>
SharedPtr<T>::SharedPtr() : ptr(nullptr), refCount(nullptr) {}

template <typename T>
SharedPtr<T>::SharedPtr(T *ptr) : ptr(ptr), refCount(new size_t(1)) {}

template <typename T>
SharedPtr<T>::SharedPtr(const SharedPtr &other) : ptr(other.ptr), refCount(other.refCount) {
  if (refCount != nullptr) {
    (*refCount)++;
  }
}

template <typename T>
SharedPtr<T>::SharedPtr(SharedPtr &&other) noexcept {
  refCount = other.refCount;
  ptr = other.ptr;

  other.ptr = nullptr;
  other.refCount = nullptr;
}

template <typename T>
SharedPtr<T>& SharedPtr<T>::operator=(const SharedPtr &other) {
  if (this == &other) {
    return *this;
  }

  if (refCount != nullptr) {
    (*refCount)--;
    if (*refCount == 0) {
      delete refCount;
      delete ptr;
    }
  }

  ptr = other.ptr;
  refCount = other.refCount;

  if (refCount != nullptr) {
    (*refCount)++;
  }

  return *this;
}

template <typename T>
SharedPtr<T>& SharedPtr<T>::operator=(SharedPtr &&other) noexcept {
  if (this == &other) {
    return *this;
  }

  if (refCount != nullptr) {
    (*refCount)--;
    if (*refCount == 0) {
      delete ptr;
      delete refCount;
    }
  }

  ptr = other.ptr;
  refCount = other.refCount;

  other.ptr = nullptr;
  other.refCount = nullptr;
  return *this;
}

template <typename T>
SharedPtr<T>::~SharedPtr() {
  if (refCount != nullptr) {
    (*refCount)--;
    if (*refCount == 0) {
      delete ptr;
      delete refCount;
    }
  }
}

