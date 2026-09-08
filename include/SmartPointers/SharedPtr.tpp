#pragma once

template <typename T>
SharedPtr<T>::SharedPtr() : ptr(nullptr), refCount(nullptr) {}

template <typename T>
SharedPtr<T>::SharedPtr(ElementType *ptr) : ptr(ptr), refCount(new size_t(1)) {}

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
      if constexpr (std::is_array_v<T>) {
        delete[] ptr;
      } else {
        delete ptr;
      }
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
      if constexpr (std::is_array_v<T>) {
        delete[] ptr;
      } else {
        delete ptr;
      }
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
SharedPtr<T>::ElementType* SharedPtr<T>::get() const {
  return ptr;
}
template <typename T>
SharedPtr<T>::ElementType& SharedPtr<T>::operator*() const {
  return *ptr;
}
template <typename T>
SharedPtr<T>::ElementType* SharedPtr<T>::operator->() const {
  return ptr;
}

template <typename T>
int SharedPtr<T>::use_count() const {
  if (refCount == nullptr) {
    return 0;
  }
  return *refCount;
}

template <typename T>
void SharedPtr<T>::reset(ElementType *newPtr) {
  if (ptr == newPtr && refCount != nullptr) {
    (*refCount)--;
    if (*refCount == 0) {
      delete refCount;
      if constexpr (std::is_array_v<T>) {
        delete[] ptr;
      } else {
        delete ptr;
      }
    }
    ptr = nullptr;
    refCount = nullptr;
    return;
  }

  if (refCount != nullptr) {
    (*refCount)--;
    if (*refCount == 0) {
      delete refCount;
      if constexpr (std::is_array_v<T>) {
        delete[] ptr;
      } else {
        delete ptr;
      }
    }
    refCount = nullptr;
    ptr = nullptr;
  }

  ptr = newPtr;

  if (newPtr != nullptr) {
    refCount = new size_t(1);
  }
}

template <typename T>
bool SharedPtr<T>::operator==(const SharedPtr &other) const noexcept {
  return ptr == other.ptr;
}

template <typename T>
bool SharedPtr<T>::operator!=(const SharedPtr &other) const noexcept {
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
bool operator==(std::nullptr_t, const SharedPtr<T> &ptr) noexcept {
  return ptr == nullptr;
}

template <typename T>
bool operator!=(std::nullptr_t, const SharedPtr<T> &ptr) noexcept {
  return ptr != nullptr;
}

template <typename T>
SharedPtr<T>::~SharedPtr() {
  if (refCount != nullptr) {
    (*refCount)--;
    if (*refCount == 0) {
      if constexpr (std::is_array_v<T>) {
        delete[] ptr;
      } else {
        delete ptr;
      }
      delete refCount;
    }
  }
}
