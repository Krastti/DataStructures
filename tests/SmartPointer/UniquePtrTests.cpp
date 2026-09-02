// ReSharper disable CppDFAUnusedValue
#include <gtest/gtest.h>
#include "../../include/SmartPointer/UniquePtr.h"

#include <iostream>
#include <utility>
#include <cstdlib>

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

    TrackedObject* secondObj = new TrackedObject();
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
