#include "Set.h"

#include <gtest/gtest.h>

#include <compare>
#include <initializer_list>
#include <limits>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>

TEST(SetTests, SavesAndReadsStringKeys) {
  Set<std::string> values("a:b");
  values.insert("");
  values.insert("a b");

  const std::string saved = values.toString();
  EXPECT_EQ(saved, "0:3:a b3:a:b");
  EXPECT_TRUE(Set<std::string>::fromString(saved) == values);
}

TEST(SetTests, SavesAndReadsIntegerKeys) {
  Set<int> values(-12);
  values.insert(5);

  const std::string saved = values.toString();
  EXPECT_EQ(saved, "3:-121:5");
  EXPECT_TRUE(Set<int>::fromString(saved) == values);
  EXPECT_THROW(static_cast<void>(Set<int>::fromString("3:12x")), std::invalid_argument);
}

TEST(SetTests, SavesAndReadsFloatingPointKeysWithoutLosingPrecision) {
  Set<double> values(1.2345678901234567);
  values.insert(std::numeric_limits<double>::denorm_min());
  values.insert(std::numeric_limits<double>::infinity());

  EXPECT_TRUE(Set<double>::fromString(values.toString()) == values);
  EXPECT_THROW(static_cast<void>(Set<double>::fromString("3:nan")), std::invalid_argument);
}

TEST(SetTests, IgnoresDuplicatesAndRemovesKeys) {
  Set<int> values;
  for (int key : {4, 2, 6, 1, 3, 5, 7}) values.insert(key);
  values.insert(4);
  EXPECT_TRUE(values.contains(4));
  EXPECT_TRUE(values.contains(7));

  values.remove(4);
  EXPECT_FALSE(values.contains(4));
  EXPECT_TRUE(values.contains(5));
  values.remove(1);
  values.remove(7);
  values.remove(2);
  EXPECT_FALSE(values.contains(2));
}

TEST(SetTests, MissingKeysThrowOutOfRangeOnEmptyAndNonemptySet) {
  Set<int> values;
  EXPECT_FALSE(values.contains(1));
  EXPECT_THROW(values.remove(1), std::out_of_range);
  values.insert(2);
  EXPECT_THROW(values.remove(1), std::out_of_range);
  EXPECT_TRUE(values.contains(2));
  values.remove(2);
  EXPECT_THROW(values.remove(2), std::out_of_range);
}

TEST(SetTests, EquivalentCompositeKeysAreStoredOnlyOnce) {
  using Key = std::pair<std::string, int>;
  Set<Key> values;
  const Key first{"Иванов", 1980};
  const Key duplicate{"Иванов", 1980};
  const Key lookup{"Иванов", 1980};
  const Key otherYear{"Иванов", 1981};
  const Key otherName{"Петров", 1980};

  values.insert(first);
  values.insert(duplicate);
  values.insert(otherYear);
  values.insert(otherName);
  ASSERT_TRUE(values.contains(lookup));
  values.remove(lookup);
  EXPECT_FALSE(values.contains(first));
  EXPECT_THROW(values.remove(duplicate), std::out_of_range);
  EXPECT_TRUE(values.contains(otherYear));
  EXPECT_TRUE(values.contains(otherName));
}

TEST(SetTests, SingleKeyConstructorAndMixedOperationsMatchStdSet) {
  Set<int> values(10);
  std::set<int> expected{10};
  std::mt19937 random(147);
  for (int step = 0; step < 10000; ++step) {
    int key = static_cast<int>(random() % 128);
    if (random() % 2) {
      values.insert(key);
      expected.insert(key);
    } else if (expected.erase(key)) {
      values.remove(key);
    } else {
      EXPECT_THROW(values.remove(key), std::out_of_range);
    }
    for (int candidate = 0; candidate < 128; ++candidate) {
      ASSERT_EQ(values.contains(candidate), expected.contains(candidate));
    }
  }
  for (int key : expected) values.remove(key);
  EXPECT_FALSE(values.contains(10));
  EXPECT_THROW(values.remove(10), std::out_of_range);
}

TEST(SetTests, BuildsUnionIntersectionAndDirectionalDifference) {
  Set<int> left(1);
  left.insert(2);
  Set<int> right(2);
  right.insert(3);

  auto united = left.unionWith(right);
  const auto common = left.intersection(right);
  const auto leftOnly = left.difference(right);
  const auto rightOnly = right.difference(left);

  for (const int key : {1, 2, 3, 4}) {
    SCOPED_TRACE(key);
    EXPECT_EQ(united.contains(key), key <= 3);
    EXPECT_EQ(common.contains(key), key == 2);
    EXPECT_EQ(leftOnly.contains(key), key == 1);
    EXPECT_EQ(rightOnly.contains(key), key == 3);
    EXPECT_EQ(left.contains(key), key == 1 || key == 2);
    EXPECT_EQ(right.contains(key), key == 2 || key == 3);
  }

  united.remove(2);
  EXPECT_FALSE(united.contains(2));
  EXPECT_TRUE(left.contains(2));
  EXPECT_TRUE(right.contains(2));
}

TEST(SetTests, HandlesEmptyDisjointAndIdenticalOperands) {
  const Set<int> empty;
  const Set<int> left(1);
  const Set<int> right(2);

  EXPECT_TRUE(left.unionWith(empty).contains(1));
  EXPECT_TRUE(empty.unionWith(left).contains(1));
  EXPECT_FALSE(left.intersection(empty).contains(1));
  EXPECT_FALSE(empty.intersection(left).contains(1));
  EXPECT_FALSE(left.intersection(right).contains(1));
  EXPECT_FALSE(left.intersection(right).contains(2));
  EXPECT_TRUE(left.difference(empty).contains(1));
  EXPECT_FALSE(empty.difference(left).contains(1));
  EXPECT_TRUE(left.unionWith(left).contains(1));
  EXPECT_TRUE(left.intersection(left).contains(1));
  EXPECT_FALSE(left.difference(left).contains(1));
}

TEST(SetTests, SetOperationsAcceptKeysWithoutDefaultConstructionOrAssignment) {
  struct Key {
    explicit Key(int value) : value(value) {}
    Key(const Key&) = default;
    Key& operator=(const Key&) = delete;
    auto operator<=>(const Key&) const = default;

    int value;
  };

  const Set<Key> left(Key{1});
  Set<Key> right(Key{1});
  right.insert(Key{2});

  EXPECT_TRUE(left.unionWith(right).contains(Key{2}));
  EXPECT_TRUE(left.intersection(right).contains(Key{1}));
  EXPECT_TRUE(right.difference(left).contains(Key{2}));
  EXPECT_TRUE(left.isSubsetOf(right));
  EXPECT_FALSE(left == right);
}

TEST(SetTests, ChecksSubsetDirectionAndIncludesEmptySet) {
  const Set<int> empty;
  const Set<int> subset(2);
  const Set<int> missing(3);
  Set<int> superset(1);
  superset.insert(2);

  EXPECT_TRUE(empty.isSubsetOf(empty));
  EXPECT_TRUE(empty.isSubsetOf(superset));
  EXPECT_TRUE(subset.isSubsetOf(superset));
  EXPECT_TRUE(superset.isSubsetOf(superset));
  EXPECT_FALSE(superset.isSubsetOf(subset));
  EXPECT_FALSE(subset.isSubsetOf(empty));
  EXPECT_FALSE(missing.isSubsetOf(superset));
}

TEST(SetTests, ComparesElementsRegardlessOfInsertionOrder) {
  const Set<int> empty;
  const Set<int> anotherEmpty;
  Set<int> left(1);
  for (int key : {2, 3, 4}) left.insert(key);
  Set<int> right(4);
  for (int key : {3, 2, 1}) right.insert(key);
  right.insert(1);

  EXPECT_TRUE(empty == anotherEmpty);
  EXPECT_TRUE(left == right);
  EXPECT_TRUE(right == left);
  EXPECT_FALSE(left == empty);

  right.remove(4);
  right.insert(5);
  EXPECT_FALSE(left == right);
  EXPECT_FALSE(right == left);
}

TEST(SetTests, TraversesAllRemainingKeysAfterRemovingInternalNodes) {
  Set<int> values;
  Set<int> expected;
  for (int key = 1; key <= 15; ++key) {
    values.insert(key);
    if (key != 4 && key != 8 && key != 12) expected.insert(key);
  }
  for (int key : {4, 8, 12}) values.remove(key);

  const Set<int> empty;
  EXPECT_TRUE(empty.unionWith(values) == expected);
  EXPECT_TRUE(values.intersection(expected) == expected);
  EXPECT_TRUE(values.difference(empty) == expected);
  EXPECT_TRUE(values == expected);
}

namespace {

int parity(int value) {
  return value % 2;
}

bool isEven(int value) {
  return parity(value) == 0;
}

int weightedSum(int key, int accumulator) {
  return 2 * key + 3 * accumulator;
}

std::string appendKey(int key, std::string accumulator) {
  accumulator += std::to_string(key);
  return accumulator;
}

}

TEST(SetTests, TransformsAndFiltersIntoIndependentSets) {
  Set<int> source(3);
  source.insert(1);
  source.insert(2);

  Set<int> expectedMap(0);
  expectedMap.insert(1);
  auto mapped = source.map(&parity);
  EXPECT_TRUE(mapped == expectedMap);
  EXPECT_TRUE(source.where(isEven) == Set<int>(2));

  mapped.remove(1);
  EXPECT_TRUE(source.contains(1));
  EXPECT_TRUE(source.contains(3));
}

TEST(SetTests, ReducesInAscendingOrderWithKeyAsFirstArgument) {
  Set<int> source(3);
  source.insert(1);
  source.insert(2);

  EXPECT_EQ(source.reduce(weightedSum, 4), 144);
  EXPECT_EQ(source.reduce(appendKey, std::string{}), "123");
}

TEST(SetTests, TransformFilterAndReduceHandleEmptySets) {
  const Set<int> empty;

  EXPECT_TRUE(empty.map(parity) == empty);
  EXPECT_TRUE(empty.where(isEven) == empty);
  EXPECT_EQ(empty.reduce(weightedSum, 4), 4);
  EXPECT_TRUE(Set<int>(3).where(isEven) == empty);
}

TEST(SetTests, RejectsNullFunctionPointers) {
  const Set<int> source(1);
  int (*transform)(int) = nullptr;
  bool (*predicate)(int) = nullptr;
  int (*combine)(int, int) = nullptr;

  EXPECT_THROW(static_cast<void>(source.map(transform)), std::invalid_argument);
  EXPECT_THROW(static_cast<void>(source.where(predicate)), std::invalid_argument);
  EXPECT_THROW(static_cast<void>(source.reduce(combine, 0)), std::invalid_argument);
}
