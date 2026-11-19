// ReSharper disable CppDFAUnusedValue
#include "../include/UniquePtr.h"
#include "../../DataStructures/tests/Timer.h"
#include <gtest/gtest.h>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

namespace {

struct TrackedObject {
  inline static int liveCount = 0;
  bool isDestroyed = false;
  int value = 67;

  TrackedObject() { liveCount++; }

  ~TrackedObject() {
    EXPECT_FALSE(isDestroyed) << "Двойное освобождение!";
    if (isDestroyed) {
      std::cerr << "FATAL: Двойное освобождение! Тест не пройден." << std::endl;
      std::abort();
    }
    isDestroyed = true;
    liveCount--;
  }
};

struct VirtualBase {
  inline static int destroyedCount = 0;
  virtual ~VirtualBase() { ++destroyedCount; }
};

struct FirstPolymorphicBase {
  virtual ~FirstPolymorphicBase() = default;
};

struct DerivedTrackedObject : FirstPolymorphicBase, VirtualBase {
  inline static int destroyedCount = 0;
  ~DerivedTrackedObject() { ++destroyedCount; }
};

template<typename Ptr>
concept SupportsSubscript = requires(const Ptr& pointer) { pointer[0]; };

static_assert(SupportsSubscript<UniquePtr<TrackedObject[]>>);
static_assert(!SupportsSubscript<UniquePtr<TrackedObject>>);

class UniquePtrTest : public ::testing::Test {
protected:
  void SetUp() override {
    TrackedObject::liveCount = 0;
  }

  void TearDown() override {
    EXPECT_EQ(TrackedObject::liveCount, 0)
      << "Обнаружена утечка памяти";
  }
};

TEST_F(UniquePtrTest, UniquePtr_Creation_RawPointerAndDefault) {
    {
        UniquePtr<TrackedObject> emptyPtr;
        EXPECT_EQ(emptyPtr.get(), nullptr);

        UniquePtr rawPtr(new TrackedObject());
        EXPECT_NE(rawPtr.get(), nullptr);
        EXPECT_EQ(TrackedObject::liveCount, 1);
    }
    EXPECT_EQ(TrackedObject::liveCount, 0);
}

TEST_F(UniquePtrTest, UniquePtr_Dereferencing_OperatorStarAndArrow) {
    UniquePtr ptr(new TrackedObject());
    ptr->value = 100;
    EXPECT_EQ((*ptr).value, 100);
    EXPECT_EQ(ptr->value, 100);
}

TEST_F(UniquePtrTest, UniquePtr_Get_DoesNotTransferOwnership) {
  {
    TrackedObject* raw = nullptr;
    UniquePtr ptr(new TrackedObject());
        raw = ptr.get();
        EXPECT_NE(raw, nullptr);
        EXPECT_EQ(TrackedObject::liveCount, 1);
    }
    EXPECT_EQ(TrackedObject::liveCount, 0);
}

TEST_F(UniquePtrTest, UniquePtr_Release_TransfersOwnership) {
    TrackedObject* raw = nullptr;
    {
        UniquePtr ptr(new TrackedObject());
        raw = ptr.release();

        EXPECT_EQ(ptr.get(), nullptr);
        EXPECT_EQ(TrackedObject::liveCount, 1);
    }
    EXPECT_EQ(TrackedObject::liveCount, 1);

    delete raw;
    EXPECT_EQ(TrackedObject::liveCount, 0);
}

TEST_F(UniquePtrTest, UniquePtr_ResetWithNewPointer_ReleasesOldAndTakesNew) {
    UniquePtr ptr(new TrackedObject());
    EXPECT_EQ(TrackedObject::liveCount, 1);

    auto secondObj = new TrackedObject();
    ptr.reset(secondObj);

    EXPECT_EQ(TrackedObject::liveCount, 1);
    EXPECT_EQ(ptr.get(), secondObj);
}

TEST_F(UniquePtrTest, UniquePtr_ResetWithoutArgs_ReleasesCurrent) {
    UniquePtr ptr(new TrackedObject());
    EXPECT_EQ(TrackedObject::liveCount, 1);

    ptr.reset();

    EXPECT_EQ(ptr.get(), nullptr);
    EXPECT_EQ(TrackedObject::liveCount, 0);
}

TEST_F(UniquePtrTest, UniquePtr_MoveConstructor_TransfersOwnership) {
    UniquePtr ptr1(new TrackedObject());
    TrackedObject* obj = ptr1.get();

    UniquePtr ptr2(std::move(ptr1));

    EXPECT_EQ(ptr1.get(), nullptr);
    EXPECT_EQ(ptr2.get(), obj);
    EXPECT_EQ(TrackedObject::liveCount, 1);
}

TEST_F(UniquePtrTest, UniquePtr_MoveAssignment_ReleasesPreviouslyOwnedResource) {
    UniquePtr ptr1(new TrackedObject());
    // ReSharper disable once CppEntityAssignedButNoRead
    UniquePtr ptr2(new TrackedObject());
    EXPECT_EQ(TrackedObject::liveCount, 2);

    ptr2 = std::move(ptr1);

    EXPECT_EQ(ptr1.get(), nullptr);
    EXPECT_EQ(TrackedObject::liveCount, 1);
}

TEST_F(UniquePtrTest, UniquePtr_SelfMoveAssignment_DoesNotCrashOrPrematurelyDelete) {
    UniquePtr ptr(new TrackedObject());
    TrackedObject* obj = ptr.get();

    ptr = std::move(ptr);

    EXPECT_EQ(ptr.get(), obj);
    EXPECT_EQ(TrackedObject::liveCount, 1);
}

TEST_F(UniquePtrTest, UniquePtr_Destructor_ReleasesObject) {
    {
        // ReSharper disable once CppDFAMemoryLeak
        UniquePtr ptr(new TrackedObject());
        EXPECT_EQ(TrackedObject::liveCount, 1);
    }
    EXPECT_EQ(TrackedObject::liveCount, 0);
}

TEST_F(UniquePtrTest, UniquePtr_ArraySubscriptAndDeleteArray) {
  {
    UniquePtr<TrackedObject[]> values(new TrackedObject[3]);
    ASSERT_EQ(TrackedObject::liveCount, 3);
    values[1].value = 42;
    EXPECT_EQ(values[1].value, 42);
    EXPECT_EQ(values.get()[2].value, 67);
  }
  EXPECT_EQ(TrackedObject::liveCount, 0);
}

TEST_F(UniquePtrTest, UniquePtr_ConvertingMovePreservesDerivedDeletion) {
  VirtualBase::destroyedCount = 0;
  DerivedTrackedObject::destroyedCount = 0;
  auto* raw = new DerivedTrackedObject();
  {
    UniquePtr<DerivedTrackedObject> derived(raw);
    UniquePtr<VirtualBase> base(std::move(derived));
    EXPECT_EQ(derived, nullptr);
    EXPECT_EQ(base.get(), static_cast<VirtualBase*>(raw));
    EXPECT_EQ(DerivedTrackedObject::destroyedCount, 0);
  }
  EXPECT_EQ(DerivedTrackedObject::destroyedCount, 1);
  EXPECT_EQ(VirtualBase::destroyedCount, 1);
}

TEST_F(UniquePtrTest, UniquePtr_ConvertingMoveAssignmentReleasesOldAndDerivedObjects) {
  VirtualBase::destroyedCount = 0;
  DerivedTrackedObject::destroyedCount = 0;
  {
    UniquePtr<VirtualBase> destination(new VirtualBase());
    UniquePtr<DerivedTrackedObject> source(new DerivedTrackedObject());
    destination = std::move(source);
    EXPECT_EQ(source, nullptr);
    EXPECT_EQ(DerivedTrackedObject::destroyedCount, 0);
    EXPECT_EQ(VirtualBase::destroyedCount, 1);
  }
  EXPECT_EQ(DerivedTrackedObject::destroyedCount, 1);
  EXPECT_EQ(VirtualBase::destroyedCount, 2);
}

TEST_F(UniquePtrTest, UniquePtr_DirectBaseOwnerAndResetUseVirtualDestructor) {
  VirtualBase::destroyedCount = 0;
  DerivedTrackedObject::destroyedCount = 0;
  {
    UniquePtr<VirtualBase> owner(new DerivedTrackedObject());
    owner.reset(new DerivedTrackedObject());
    EXPECT_EQ(DerivedTrackedObject::destroyedCount, 1);
  }
  EXPECT_EQ(DerivedTrackedObject::destroyedCount, 2);
  EXPECT_EQ(VirtualBase::destroyedCount, 2);
}

namespace {
    constexpr int kUniqueIterations = 500'000;
}

TEST_F(UniquePtrTest, StdUniquePtr_CreateDestroy) {
    for (int k = 0; k < 1; k++) {
        Timer timer("std::unique_ptr create/destroy x" + std::to_string(kUniqueIterations));
        for (int i = 0; i < kUniqueIterations; ++i) {
            auto p = std::make_unique<TrackedObject>();
        }
        timer.stop();
    }
}

// [Замер] Создание и уничтожение std::unique_ptr 500000 раз: 48.3059 мс.
// [Замер] Создание и уничтожение std::unique_ptr 500000 раз: 50.8849 мс.
// [Замер] Создание и уничтожение std::unique_ptr 500000 раз: 46.6353 мс.
// [Замер] Создание и уничтожение std::unique_ptr 500000 раз: 53.238 мс.
// [Замер] Создание и уничтожение std::unique_ptr 500000 раз: 56.2345 мс.
// [Замер] Создание и уничтожение std::unique_ptr 500000 раз: 54.5628 мс.
// [Замер] Создание и уничтожение std::unique_ptr 500000 раз: 53.292 мс.
// [Замер] Создание и уничтожение std::unique_ptr 500000 раз: 51.7812 мс.
// [Замер] Создание и уничтожение std::unique_ptr 500000 раз: 51.8659 мс.
// [Замер] Создание и уничтожение std::unique_ptr 500000 раз: 56.8523 мс.
// [Замер] Создание и уничтожение std::unique_ptr 500000 раз: 52.4052 мс.
// [Замер] Создание и уничтожение std::unique_ptr 500000 раз: 52.1122 мс.
// [Замер] Создание и уничтожение std::unique_ptr 500000 раз: 50.9419 мс.

TEST_F(UniquePtrTest, CustomUniquePtr_CreateDestroy) {
    for (int k =0; k < 1; k++) {
        Timer timer("Custom UniquePtr create/destroy x" + std::to_string(kUniqueIterations));
        for (int i = 0; i < kUniqueIterations; ++i) {
            UniquePtr p(new TrackedObject());
        }
        timer.stop();
    }
}

// [Замер] Создание и уничтожение собственного UniquePtr 500000 раз: 33.8248 мс.
// [Замер] Создание и уничтожение собственного UniquePtr 500000 раз: 35.0644 мс.
// [Замер] Создание и уничтожение собственного UniquePtr 500000 раз: 32.7523 мс.
// [Замер] Создание и уничтожение собственного UniquePtr 500000 раз: 34.3824 мс.
// [Замер] Создание и уничтожение собственного UniquePtr 500000 раз: 34.7001 мс.
// [Замер] Создание и уничтожение собственного UniquePtr 500000 раз: 36.3489 мс.
// [Замер] Создание и уничтожение собственного UniquePtr 500000 раз: 37.0113 мс.
// [Замер] Создание и уничтожение собственного UniquePtr 500000 раз: 36.1236 мс.
// [Замер] Создание и уничтожение собственного UniquePtr 500000 раз: 34.8457 мс.
// [Замер] Создание и уничтожение собственного UniquePtr 500000 раз: 35.8224 мс.
// [Замер] Создание и уничтожение собственного UniquePtr 500000 раз: 34.8561 мс.
// [Замер] Создание и уничтожение собственного UniquePtr 500000 раз: 35.2651 мс.
// [Замер] Создание и уничтожение собственного UniquePtr 500000 раз: 35.0988 мс.

} // Безымянное пространство имён.
