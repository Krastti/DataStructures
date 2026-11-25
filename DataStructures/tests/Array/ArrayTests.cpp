#include "ArrayTestHelpers.h"
#include <gtest/gtest.h>

#include <utility>
#include <vector>
#include <cstdint>

namespace {
struct CopyOnlyValue {
  CopyOnlyValue() = default;
  explicit CopyOnlyValue(int value) : value(value) {}
  CopyOnlyValue(const CopyOnlyValue&) = default;
  CopyOnlyValue(CopyOnlyValue&&) = delete;
  CopyOnlyValue& operator=(const CopyOnlyValue&) = default;
  int value;
};

struct ThrowOnAssignment {
  static inline int live = 0;
  static inline int assignmentsBeforeThrow = -1;

  ThrowOnAssignment() { ++live; }
  explicit ThrowOnAssignment(int value) : value(value) { ++live; }
  ThrowOnAssignment(const ThrowOnAssignment& other) : value(other.value) { ++live; }
  ThrowOnAssignment(ThrowOnAssignment&& other) noexcept : value(other.value) { ++live; }
  ThrowOnAssignment& operator=(const ThrowOnAssignment& other) {
    if (assignmentsBeforeThrow == 0) throw 7;
    if (assignmentsBeforeThrow > 0) --assignmentsBeforeThrow;
    value = other.value;
    return *this;
  }
  ~ThrowOnAssignment() { --live; }
  int value;
};

struct alignas(64) AlignedValue {
  int value = 0;
};
}

TEST(ArrayTests, DoublesBeforeAddingPastSeventyFivePercent) {
  Array<int> values;
  EXPECT_EQ(values.size(), 0);

  values.push_back(0);
  int* firstBlock = &values[0];
  values.push_back(1);
  values.push_back(2);
  EXPECT_EQ(&values[0], firstBlock);

  values.push_back(3);
  EXPECT_NE(&values[0], firstBlock);
  int* secondBlock = &values[0];
  values.push_back(4);
  values.push_back(5);
  EXPECT_EQ(&values[0], secondBlock);
  values.push_back(6);
  EXPECT_NE(&values[0], secondBlock);

  for (int value = 7; value < 20; ++value) values.push_back(value);
  ASSERT_EQ(values.size(), 20);
  for (size_t i = 0; i < values.size(); ++i) EXPECT_EQ(values[i], static_cast<int>(i));
}

TEST(ArrayTests, CopiesAndMovesElements) {
  Array<int> values;
  for (int value : {1, 2, 3}) values.push_back(value);
  Array<int> copied(values);
  copied[1] = 8;
  EXPECT_EQ(toVector(values), (std::vector<int>{1, 2, 3}));
  EXPECT_EQ(toVector(copied), (std::vector<int>{1, 8, 3}));

  Array<int> moved(std::move(copied));
  EXPECT_EQ(copied.size(), 0);
  EXPECT_EQ(toVector(moved), (std::vector<int>{1, 8, 3}));

  Array<int> assigned;
  assigned = moved;
  EXPECT_EQ(toVector(assigned), toVector(moved));
  assigned = std::move(moved);
  EXPECT_EQ(moved.size(), 0);
  EXPECT_EQ(toVector(assigned), (std::vector<int>{1, 8, 3}));
}

TEST(ArrayTests, CanAppendAnElementFromTheSameArrayDuringGrowth) {
  Array<int> values;
  values.push_back(10);
  values.push_back(20);
  values.push_back(30);
  values.push_back(values[0]);
  EXPECT_EQ(toVector(values), (std::vector<int>{10, 20, 30, 10}));
}

TEST(ArrayTests, PopBackRemovesLastElementAndRejectsEmptyArray) {
  Array<int> values;
  EXPECT_THROW(values.pop_back(), std::out_of_range);
  values.push_back(10);
  values.push_back(20);
  values.pop_back();
  EXPECT_EQ(toVector(values), (std::vector<int>{10}));
  values.push_back(30);
  EXPECT_EQ(toVector(values), (std::vector<int>{10, 30}));
  values.pop_back();
  values.pop_back();
  EXPECT_EQ(values.size(), 0u);
  EXPECT_THROW(values.pop_back(), std::out_of_range);
}

TEST(ArrayTests, SupportsDefaultConstructibleCopyAssignableElements) {
  Array<CopyOnlyValue> copyOnly;
  for (int value = 0; value < 5; ++value) {
    CopyOnlyValue item(value);
    copyOnly.push_back(item);
  }
  for (size_t i = 0; i < copyOnly.size(); ++i) EXPECT_EQ(copyOnly[i].value, static_cast<int>(i));

}

TEST(ArrayTests, ReleasesAllocatedElementsAfterFailedGrowthAndCopy) {
  ThrowOnAssignment::live = 0;
  ThrowOnAssignment::assignmentsBeforeThrow = -1;
  {
    Array<ThrowOnAssignment> values;
    for (int value = 1; value <= 3; ++value) values.push_back(ThrowOnAssignment(value));
    ThrowOnAssignment extra(4);
    ASSERT_EQ(ThrowOnAssignment::live, 5);

    ThrowOnAssignment::assignmentsBeforeThrow = 1;
    EXPECT_THROW(values.push_back(extra), int);
    EXPECT_EQ(values.size(), 3);
    EXPECT_EQ(ThrowOnAssignment::live, 5);
    for (size_t i = 0; i < values.size(); ++i) EXPECT_EQ(values[i].value, static_cast<int>(i + 1));

    ThrowOnAssignment::assignmentsBeforeThrow = 1;
    EXPECT_THROW((Array<ThrowOnAssignment>(values)), int);
    EXPECT_EQ(ThrowOnAssignment::live, 5);
  }
  EXPECT_EQ(ThrowOnAssignment::live, 0);
}

TEST(ArrayTests, AlignsOverAlignedElements) {
  Array<AlignedValue> values;
  values.push_back(AlignedValue{0});
  EXPECT_EQ(reinterpret_cast<std::uintptr_t>(&values[0]) % alignof(AlignedValue), 0u);
  for (int value = 1; value < 5; ++value) values.push_back(AlignedValue{value});
  EXPECT_EQ(reinterpret_cast<std::uintptr_t>(&values[0]) % alignof(AlignedValue), 0u);
  for (size_t i = 0; i < values.size(); ++i) EXPECT_EQ(values[i].value, static_cast<int>(i));
}

namespace {
struct MoveOnlyValue {
  static inline int resources = 0;
  int* value = nullptr;
  MoveOnlyValue() = default;
  explicit MoveOnlyValue(int number) : value(new int(number)) { ++resources; }
  MoveOnlyValue(const MoveOnlyValue&) = delete;
  MoveOnlyValue& operator=(const MoveOnlyValue&) = delete;
  MoveOnlyValue(MoveOnlyValue&& other) noexcept : value(std::exchange(other.value, nullptr)) {}
  MoveOnlyValue& operator=(MoveOnlyValue&& other) noexcept {
    if (this != &other) {
      if (value) { delete value; --resources; }
      value = std::exchange(other.value, nullptr);
    }
    return *this;
  }
  ~MoveOnlyValue() { if (value) { delete value; --resources; } }
};
}

TEST(ArrayTests, GrowsWithMoveOnlyOwnersAndReleasesResourcesOnce) {
  ASSERT_EQ(MoveOnlyValue::resources, 0);
  {
    Array<MoveOnlyValue> values;
    for (int i = 0; i < 100; ++i) values.push_back(MoveOnlyValue(i));
    ASSERT_EQ(values.size(), 100u);
    EXPECT_EQ(MoveOnlyValue::resources, 100);
    for (std::size_t i = 0; i < values.size(); ++i) {
      ASSERT_NE(values[i].value, nullptr);
      EXPECT_EQ(*values[i].value, static_cast<int>(i));
    }
    Array<MoveOnlyValue> moved(std::move(values));
    EXPECT_EQ(values.size(), 0u);
    EXPECT_EQ(moved.size(), 100u);
  }
  EXPECT_EQ(MoveOnlyValue::resources, 0);
}

TEST(ArrayTests, PopBackReleasesRemovedMoveOnlyOwnerImmediately) {
  ASSERT_EQ(MoveOnlyValue::resources, 0);
  {
    Array<MoveOnlyValue> values;
    values.push_back(MoveOnlyValue(1));
    values.push_back(MoveOnlyValue(2));
    values.pop_back();
    ASSERT_EQ(values.size(), 1u);
    EXPECT_EQ(MoveOnlyValue::resources, 1);
    EXPECT_EQ(*values[0].value, 1);
    values.pop_back();
    EXPECT_EQ(values.size(), 0u);
    EXPECT_EQ(MoveOnlyValue::resources, 0);
  }
  EXPECT_EQ(MoveOnlyValue::resources, 0);
}
