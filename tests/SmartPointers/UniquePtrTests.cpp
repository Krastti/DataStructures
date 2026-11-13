// ReSharper disable CppDFAUnusedValue
#include "../../include/SmartPointers/UniquePtr.h"
#include "../Timer.h"
#include <gtest/gtest.h>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

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

struct NonVirtualBase {
  inline static int destroyedCount = 0;
  ~NonVirtualBase() { ++destroyedCount; }
};

struct FirstPolymorphicBase {
  virtual ~FirstPolymorphicBase() = default;
};

struct DerivedTrackedObject : FirstPolymorphicBase, NonVirtualBase {
  inline static int destroyedCount = 0;
  ~DerivedTrackedObject() { ++destroyedCount; }
};

template<typename Ptr>
concept SupportsSubscript = requires(const Ptr& pointer) { pointer[0]; };

static_assert(SupportsSubscript<UniquePtr<TrackedObject[]>>);
static_assert(!SupportsSubscript<UniquePtr<TrackedObject>>);

class SmartPtrTest : public ::testing::Test {
protected:
  void SetUp() override {
    TrackedObject::liveCount = 0;
  }

  void TearDown() override {
    EXPECT_EQ(TrackedObject::liveCount, 0)
      << "Обнаружена утечка памяти";
  }
};

TEST_F(SmartPtrTest, UniquePtr_Creation_RawPointerAndDefault) {
    {
        UniquePtr<TrackedObject> emptyPtr;
        EXPECT_EQ(emptyPtr.get(), nullptr);

        UniquePtr rawPtr(new TrackedObject());
        EXPECT_NE(rawPtr.get(), nullptr);
        EXPECT_EQ(TrackedObject::liveCount, 1);
    }
    EXPECT_EQ(TrackedObject::liveCount, 0);
}

TEST_F(SmartPtrTest, UniquePtr_Dereferencing_OperatorStarAndArrow) {
    UniquePtr ptr(new TrackedObject());
    ptr->value = 100;
    EXPECT_EQ((*ptr).value, 100);
    EXPECT_EQ(ptr->value, 100);
}

TEST_F(SmartPtrTest, UniquePtr_Get_DoesNotTransferOwnership) {
  {
    TrackedObject* raw = nullptr;
    UniquePtr ptr(new TrackedObject());
        raw = ptr.get();
        EXPECT_NE(raw, nullptr);
        EXPECT_EQ(TrackedObject::liveCount, 1);
    }
    EXPECT_EQ(TrackedObject::liveCount, 0);
}

TEST_F(SmartPtrTest, UniquePtr_Release_TransfersOwnership) {
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

TEST_F(SmartPtrTest, UniquePtr_ResetWithNewPointer_ReleasesOldAndTakesNew) {
    UniquePtr ptr(new TrackedObject());
    EXPECT_EQ(TrackedObject::liveCount, 1);

    auto secondObj = new TrackedObject();
    ptr.reset(secondObj);

    EXPECT_EQ(TrackedObject::liveCount, 1);
    EXPECT_EQ(ptr.get(), secondObj);
}

TEST_F(SmartPtrTest, UniquePtr_ResetWithoutArgs_ReleasesCurrent) {
    UniquePtr ptr(new TrackedObject());
    EXPECT_EQ(TrackedObject::liveCount, 1);

    ptr.reset();

    EXPECT_EQ(ptr.get(), nullptr);
    EXPECT_EQ(TrackedObject::liveCount, 0);
}

TEST_F(SmartPtrTest, UniquePtr_MoveConstructor_TransfersOwnership) {
    UniquePtr ptr1(new TrackedObject());
    TrackedObject* obj = ptr1.get();

    UniquePtr ptr2(std::move(ptr1));

    EXPECT_EQ(ptr1.get(), nullptr);
    EXPECT_EQ(ptr2.get(), obj);
    EXPECT_EQ(TrackedObject::liveCount, 1);
}

TEST_F(SmartPtrTest, UniquePtr_MoveAssignment_ReleasesPreviouslyOwnedResource) {
    UniquePtr ptr1(new TrackedObject());
    // ReSharper disable once CppEntityAssignedButNoRead
    UniquePtr ptr2(new TrackedObject());
    EXPECT_EQ(TrackedObject::liveCount, 2);

    ptr2 = std::move(ptr1);

    EXPECT_EQ(ptr1.get(), nullptr);
    EXPECT_EQ(TrackedObject::liveCount, 1);
}

TEST_F(SmartPtrTest, UniquePtr_SelfMoveAssignment_DoesNotCrashOrPrematurelyDelete) {
    UniquePtr ptr(new TrackedObject());
    TrackedObject* obj = ptr.get();

    ptr = std::move(ptr);

    EXPECT_EQ(ptr.get(), obj);
    EXPECT_EQ(TrackedObject::liveCount, 1);
}

TEST_F(SmartPtrTest, UniquePtr_Destructor_ReleasesObject) {
    {
        // ReSharper disable once CppDFAMemoryLeak
        UniquePtr ptr(new TrackedObject());
        EXPECT_EQ(TrackedObject::liveCount, 1);
    }
    EXPECT_EQ(TrackedObject::liveCount, 0);
}

TEST_F(SmartPtrTest, UniquePtr_ArraySubscriptAndDeleteArray) {
  {
    UniquePtr<TrackedObject[]> values(new TrackedObject[3]);
    ASSERT_EQ(TrackedObject::liveCount, 3);
    values[1].value = 42;
    EXPECT_EQ(values[1].value, 42);
    EXPECT_EQ(values.get()[2].value, 67);
  }
  EXPECT_EQ(TrackedObject::liveCount, 0);
}

TEST_F(SmartPtrTest, UniquePtr_ConvertingMovePreservesDerivedDeletion) {
  NonVirtualBase::destroyedCount = 0;
  DerivedTrackedObject::destroyedCount = 0;
  auto* raw = new DerivedTrackedObject();
  {
    UniquePtr<DerivedTrackedObject> derived(raw);
    UniquePtr<NonVirtualBase> base(std::move(derived));
    EXPECT_EQ(derived, nullptr);
    EXPECT_EQ(base.get(), static_cast<NonVirtualBase*>(raw));
    EXPECT_EQ(DerivedTrackedObject::destroyedCount, 0);
  }
  EXPECT_EQ(DerivedTrackedObject::destroyedCount, 1);
  EXPECT_EQ(NonVirtualBase::destroyedCount, 1);
}

TEST_F(SmartPtrTest, UniquePtr_ConvertingMoveAssignmentReleasesOldAndDerivedObjects) {
  NonVirtualBase::destroyedCount = 0;
  DerivedTrackedObject::destroyedCount = 0;
  {
    UniquePtr<NonVirtualBase> destination(new NonVirtualBase());
    UniquePtr<DerivedTrackedObject> source(new DerivedTrackedObject());
    destination = std::move(source);
    EXPECT_EQ(source, nullptr);
    EXPECT_EQ(DerivedTrackedObject::destroyedCount, 0);
    EXPECT_EQ(NonVirtualBase::destroyedCount, 1);
  }
  EXPECT_EQ(DerivedTrackedObject::destroyedCount, 1);
  EXPECT_EQ(NonVirtualBase::destroyedCount, 2);
}

TEST_F(SmartPtrTest, UniquePtr_DirectBaseOwnerAndResetRetainDynamicType) {
  NonVirtualBase::destroyedCount = 0;
  DerivedTrackedObject::destroyedCount = 0;
  {
    UniquePtr<NonVirtualBase> owner(new DerivedTrackedObject());
    owner.reset(new DerivedTrackedObject());
    EXPECT_EQ(DerivedTrackedObject::destroyedCount, 1);
  }
  EXPECT_EQ(DerivedTrackedObject::destroyedCount, 2);
  EXPECT_EQ(NonVirtualBase::destroyedCount, 2);
}

namespace {
    constexpr int kUniqueIterations = 500'000;
}

TEST_F(SmartPtrTest, StdUniquePtr_CreateDestroy) {
    for (int k = 0; k < 1; k++) {
        Timer timer("std::unique_ptr create/destroy x" + std::to_string(kUniqueIterations));
        for (int i = 0; i < kUniqueIterations; ++i) {
            auto p = std::make_unique<TrackedObject>();
        }
        timer.stop();
    }
}

// [Timer] std::unique_ptr create/destroy x500000: 48.3059 ms
// [Timer] std::unique_ptr create/destroy x500000: 50.8849 ms
// [Timer] std::unique_ptr create/destroy x500000: 46.6353 ms
// [Timer] std::unique_ptr create/destroy x500000: 53.238 ms
// [Timer] std::unique_ptr create/destroy x500000: 56.2345 ms
// [Timer] std::unique_ptr create/destroy x500000: 54.5628 ms
// [Timer] std::unique_ptr create/destroy x500000: 53.292 ms
// [Timer] std::unique_ptr create/destroy x500000: 51.7812 ms
// [Timer] std::unique_ptr create/destroy x500000: 51.8659 ms
// [Timer] std::unique_ptr create/destroy x500000: 56.8523 ms
// [Timer] std::unique_ptr create/destroy x500000: 52.4052 ms
// [Timer] std::unique_ptr create/destroy x500000: 52.1122 ms
// [Timer] std::unique_ptr create/destroy x500000: 50.9419 ms

TEST_F(SmartPtrTest, CustomUniquePtr_CreateDestroy) {
    for (int k =0; k < 1; k++) {
        Timer timer("Custom UniquePtr create/destroy x" + std::to_string(kUniqueIterations));
        for (int i = 0; i < kUniqueIterations; ++i) {
            UniquePtr p(new TrackedObject());
        }
        timer.stop();
    }
}

// [Timer] Custom UniquePtr create/destroy x500000: 33.8248 ms
// [Timer] Custom UniquePtr create/destroy x500000: 35.0644 ms
// [Timer] Custom UniquePtr create/destroy x500000: 32.7523 ms
// [Timer] Custom UniquePtr create/destroy x500000: 34.3824 ms
// [Timer] Custom UniquePtr create/destroy x500000: 34.7001 ms
// [Timer] Custom UniquePtr create/destroy x500000: 36.3489 ms
// [Timer] Custom UniquePtr create/destroy x500000: 37.0113 ms
// [Timer] Custom UniquePtr create/destroy x500000: 36.1236 ms
// [Timer] Custom UniquePtr create/destroy x500000: 34.8457 ms
// [Timer] Custom UniquePtr create/destroy x500000: 35.8224 ms
// [Timer] Custom UniquePtr create/destroy x500000: 34.8561 ms
// [Timer] Custom UniquePtr create/destroy x500000: 35.2651 ms
// [Timer] Custom UniquePtr create/destroy x500000: 35.0988 ms
