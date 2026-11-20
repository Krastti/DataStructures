#include "AllocationFailure.h"

#include <cstdlib>
#include <new>

namespace {
thread_local std::ptrdiff_t allocationBudget = -1;
thread_local std::ptrdiff_t* allocationBalance = nullptr;
}

// Подмена действует только в тестовых бинарниках, куда подключён этот файл.
// Включаем отказ только на время проверяемой операции.
void* operator new(std::size_t size) {
  if (allocationBudget == 0) throw std::bad_alloc();
  if (void* memory = std::malloc(size == 0 ? 1 : size)) {
    if (allocationBudget > 0) --allocationBudget;
    if (allocationBalance) ++*allocationBalance;
    return memory;
  }
  throw std::bad_alloc();
}

void operator delete(void* memory) noexcept {
  if (memory && allocationBalance) --*allocationBalance;
  std::free(memory);
}
void operator delete(void* memory, std::size_t) noexcept { ::operator delete(memory); }
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete[](void* memory) noexcept { ::operator delete(memory); }
void operator delete[](void* memory, std::size_t) noexcept { ::operator delete(memory); }
void* operator new(std::size_t size, const std::nothrow_t&) noexcept {
  try { return ::operator new(size); }
  catch (...) { return nullptr; }
}
void* operator new[](std::size_t size, const std::nothrow_t&) noexcept {
  return ::operator new(size, std::nothrow);
}
void operator delete(void* memory, const std::nothrow_t&) noexcept {
  ::operator delete(memory);
}
void operator delete[](void* memory, const std::nothrow_t&) noexcept {
  ::operator delete(memory);
}

namespace test {

RejectAllocations::RejectAllocations(std::ptrdiff_t allowedAllocations)
    : previousBudget(allocationBudget) {
  allocationBudget = allowedAllocations;
}

RejectAllocations::~RejectAllocations() { allocationBudget = previousBudget; }

TrackAllocations::TrackAllocations(std::ptrdiff_t& balance) { allocationBalance = &balance; }
TrackAllocations::~TrackAllocations() { allocationBalance = nullptr; }

}
