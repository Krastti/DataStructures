#include "../../include/SmartPointers/SharedPtr.h"
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdlib>
#include <new>
#include <utility>

namespace {
thread_local bool failNextAllocation = false;
}

// Отдельный исполняемый файл изолирует отказ выделения памяти от остальных тестов.
void* operator new(std::size_t size) {
  if (std::exchange(failNextAllocation, false)) throw std::bad_alloc();
  if (void* address = std::malloc(size == 0 ? 1 : size)) return address;
  throw std::bad_alloc();
}

void operator delete(void* address) noexcept { std::free(address); }
void operator delete(void* address, std::size_t) noexcept { std::free(address); }

void* operator new(std::size_t size, const std::nothrow_t&) noexcept {
  try {
    return ::operator new(size);
  } catch (...) {
    return nullptr;
  }
}

void operator delete(void* address, const std::nothrow_t&) noexcept {
  ::operator delete(address);
}

namespace {

struct Tracked {
  inline static int live = 0;
  Tracked() { ++live; }
  ~Tracked() { --live; }
};

TEST(SharedPtrAllocationTests, FailedConstructorDeletesIncomingObject) {
  auto* raw = new Tracked;
  bool threw = false;
  try {
    failNextAllocation = true;
    SharedPtr<Tracked> owner(raw);
  } catch (const std::bad_alloc&) {
    threw = true;
  }
  failNextAllocation = false;
  EXPECT_TRUE(threw);
  EXPECT_EQ(Tracked::live, 0);
}

TEST(SharedPtrAllocationTests, FailedArrayConstructorDeletesEveryElement) {
  auto* raw = new Tracked[3];
  bool threw = false;
  try {
    failNextAllocation = true;
    SharedPtr<Tracked[]> owner(raw);
  } catch (const std::bad_alloc&) {
    threw = true;
  }
  failNextAllocation = false;
  EXPECT_TRUE(threw);
  EXPECT_EQ(Tracked::live, 0);
}

TEST(SharedPtrAllocationTests, FailedResetPreservesOldOwnershipAndDeletesIncomingObject) {
  {
    SharedPtr<Tracked> owner(new Tracked);
    auto copy = owner;
    auto* original = owner.get();
    auto* replacement = new Tracked;
    bool threw = false;
    try {
      failNextAllocation = true;
      owner.reset(replacement);
    } catch (const std::bad_alloc&) {
      threw = true;
    }
    failNextAllocation = false;
    EXPECT_TRUE(threw);
    EXPECT_EQ(owner.get(), original);
    EXPECT_EQ(copy.get(), original);
    EXPECT_EQ(owner.use_count(), 2);
    EXPECT_EQ(Tracked::live, 1);
  }
  EXPECT_EQ(Tracked::live, 0);
}

TEST(SharedPtrAllocationTests, FailedArrayResetPreservesOldArray) {
  {
    SharedPtr<Tracked[]> owner(new Tracked[2]);
    auto* original = owner.get();
    auto* replacement = new Tracked[3];
    bool threw = false;
    try {
      failNextAllocation = true;
      owner.reset(replacement);
    } catch (const std::bad_alloc&) {
      threw = true;
    }
    failNextAllocation = false;
    EXPECT_TRUE(threw);
    EXPECT_EQ(owner.get(), original);
    EXPECT_EQ(owner.use_count(), 1);
    EXPECT_EQ(Tracked::live, 2);
  }
  EXPECT_EQ(Tracked::live, 0);
}

struct VirtualBase {
  inline static int destroyed = 0;
  virtual ~VirtualBase() { ++destroyed; }
};

struct Derived : VirtualBase {
  inline static int destroyed = 0;
  ~Derived() override { ++destroyed; }
};

TEST(SharedPtrAllocationTests, FailedPolymorphicConstructorDeletesDerived) {
  VirtualBase::destroyed = Derived::destroyed = 0;
  auto* raw = new Derived;
  bool threw = false;
  try {
    failNextAllocation = true;
    SharedPtr<VirtualBase> owner(raw);
  } catch (const std::bad_alloc&) {
    threw = true;
  }
  failNextAllocation = false;
  EXPECT_TRUE(threw);
  EXPECT_EQ(Derived::destroyed, 1);
  EXPECT_EQ(VirtualBase::destroyed, 1);
}

} // Безымянное пространство имён.
