#include <gtest/gtest.h>
#include "../../include/SmartPointer/SharedPtr.h"

#include <iostream>
#include <utility>
#include <cstdlib>

struct TrackedObject {
    inline static int liveCount = 0;
    bool isDestroyed = false;
    int value = 67;

    TrackedObject() { liveCount++; }

    ~TrackedObject() {
        EXPECT_FALSE(isDestroyed);
        if (isDestroyed) {
            std::cerr << "FATAL: double free detected." << std::endl;
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
        EXPECT_EQ(TrackedObject::liveCount, 0);
    }
};

TEST_F(SmartPtrTest, SharedPtr_Creation_RawPointer) {
    SharedPtr<TrackedObject> ptr(new TrackedObject());
    EXPECT_EQ(ptr.use_count(), 1);
    EXPECT_NE(ptr.get(), nullptr);
    EXPECT_EQ(TrackedObject::liveCount, 1);
}

TEST_F(SmartPtrTest, SharedPtr_Copying_IncrementsUseCountAndDelaysDeletion) {
    SharedPtr<TrackedObject> ptr1(new TrackedObject());
    {
        SharedPtr<TrackedObject> ptr2(ptr1);
        EXPECT_EQ(ptr1.use_count(), 2);
        EXPECT_EQ(ptr2.use_count(), 2);
        EXPECT_EQ(TrackedObject::liveCount, 1);
    }
    EXPECT_EQ(ptr1.use_count(), 1);
    EXPECT_EQ(TrackedObject::liveCount, 1);
}

TEST_F(SmartPtrTest, SharedPtr_CopyAssignment_ReleasesPreviouslyOwnedResource) {
    SharedPtr<TrackedObject> ptr1(new TrackedObject());
    SharedPtr<TrackedObject> ptr2(new TrackedObject());
    EXPECT_EQ(TrackedObject::liveCount, 2);

    ptr2 = ptr1;

    EXPECT_EQ(ptr1.use_count(), 2);
    EXPECT_EQ(ptr2.use_count(), 2);
    EXPECT_EQ(TrackedObject::liveCount, 1);
}

TEST_F(SmartPtrTest, SharedPtr_SelfCopyAssignment_DoesNotCrashAndKeepsUseCount) {
    SharedPtr<TrackedObject> ptr(new TrackedObject());
    ptr = ptr;

    EXPECT_EQ(ptr.use_count(), 1);
    EXPECT_EQ(TrackedObject::liveCount, 1);
}

TEST_F(SmartPtrTest, SharedPtr_MoveConstructorAndAssignment_DoNotIncreaseUseCount) {
    SharedPtr<TrackedObject> ptr1(new TrackedObject());
    SharedPtr<TrackedObject> ptr2(std::move(ptr1));

    EXPECT_EQ(ptr2.use_count(), 1);
    EXPECT_EQ(TrackedObject::liveCount, 1);

    SharedPtr<TrackedObject> ptr3(new TrackedObject());
    EXPECT_EQ(TrackedObject::liveCount, 2);

    ptr3 = std::move(ptr2);
    EXPECT_EQ(ptr3.use_count(), 1);
    EXPECT_EQ(TrackedObject::liveCount, 1);
}

TEST_F(SmartPtrTest, SharedPtr_SelfMoveAssignment_DoesNotCrash) {
    SharedPtr<TrackedObject> ptr(new TrackedObject());
    ptr = std::move(ptr);

    SUCCEED();
}

TEST_F(SmartPtrTest, SharedPtr_UseCount_ZeroForDefaultConstructedAndMovedFrom) {
    SharedPtr<TrackedObject> emptyPtr;
    EXPECT_EQ(emptyPtr.use_count(), 0);

    SharedPtr<TrackedObject> ptr(new TrackedObject());
    SharedPtr<TrackedObject> movedFrom(std::move(ptr));

    EXPECT_EQ(ptr.use_count(), 0);
    EXPECT_EQ(TrackedObject::liveCount, 1);
}

void CheckConstSharedPtr(const SharedPtr<TrackedObject>& ptr) {
    EXPECT_EQ((*ptr).value, 42);
    EXPECT_EQ(ptr->value, 42);
}

TEST_F(SmartPtrTest, SharedPtr_ConstReference_DereferencingWorks) {
    SharedPtr<TrackedObject> ptr(new TrackedObject());
    ptr->value = 42;
    CheckConstSharedPtr(ptr);
}

TEST_F(SmartPtrTest, SharedPtr_ComplexChain_SingleDeletionAtTheEnd) {
    SharedPtr<TrackedObject> p1(new TrackedObject());
    EXPECT_EQ(TrackedObject::liveCount, 1);

    SharedPtr<TrackedObject> p2 = p1;
    SharedPtr<TrackedObject> p3 = std::move(p2);
    SharedPtr<TrackedObject> p4(p1);
    p4 = p3;

    EXPECT_EQ(TrackedObject::liveCount, 1);

    {
        SharedPtr<TrackedObject> p5 = p1;
        SharedPtr<TrackedObject> p6 = std::move(p5);
        EXPECT_EQ(TrackedObject::liveCount, 1);
    }

    p1 = SharedPtr<TrackedObject>();
    p3 = SharedPtr<TrackedObject>();
    p4 = SharedPtr<TrackedObject>();

    EXPECT_EQ(TrackedObject::liveCount, 0);
}
