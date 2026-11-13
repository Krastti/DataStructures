#include "../../include/SmartPointers/SharedPtr.h"
#include "../../include/SmartPointers/UniquePtr.h"
#include <gtest/gtest.h>

#include <atomic>
#include <barrier>
#include <cstddef>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

struct Base {
  int value = 42;
  inline static int destroyed = 0;
  virtual ~Base() { ++destroyed; }
};

struct FirstBase {
  virtual ~FirstBase() = default;
};

struct Derived : FirstBase, Base {
  inline static int destroyed = 0;
  ~Derived() { ++destroyed; }
};

template<typename Ptr, typename Raw>
concept CanReset = requires(Ptr& pointer, Raw raw) { pointer.reset(raw); };

template<typename Ptr>
concept CanSubscript = requires(const Ptr& pointer) { pointer[0]; };

template<typename Ptr>
concept CanDereference = requires(const Ptr& pointer) { *pointer; pointer.operator->(); };

// Проверяем конструктор и reset: оба должны исключать обход ограничения через перегрузку для Base*.
static_assert(std::is_constructible_v<UniquePtr<const int[]>, int*>);
static_assert(std::is_constructible_v<SharedPtr<const int[]>, int*>);
static_assert(CanReset<UniquePtr<const int[]>, int*>);
static_assert(CanReset<SharedPtr<const int[]>, int*>);
static_assert(CanSubscript<SharedPtr<int[]>>);
static_assert(!CanSubscript<SharedPtr<int>>);
static_assert(!CanDereference<SharedPtr<int[]>>);
static_assert(CanDereference<SharedPtr<int>>);
static_assert(!std::is_copy_constructible_v<UniquePtr<int>>);
static_assert(!std::is_copy_assignable_v<UniquePtr<int>>);

struct NonVirtualBase {};
struct NonVirtualDerived : NonVirtualBase {};

// Ни одна перегрузка для сырого указателя не должна обходить требование виртуального деструктора.
static_assert(std::is_constructible_v<UniquePtr<NonVirtualBase>, NonVirtualBase*>);
static_assert(std::is_constructible_v<SharedPtr<NonVirtualBase>, NonVirtualBase*>);
static_assert(std::is_constructible_v<UniquePtr<Base>, Derived*>);
static_assert(std::is_constructible_v<SharedPtr<Base>, Derived*>);
static_assert(CanReset<UniquePtr<Base>, Derived*>);
static_assert(CanReset<SharedPtr<Base>, Derived*>);
static_assert(std::is_constructible_v<UniquePtr<Base>, UniquePtr<Derived>&&>);
static_assert(std::is_assignable_v<UniquePtr<Base>&, UniquePtr<Derived>&&>);
static_assert(!CanDereference<UniquePtr<int[]>>);
static_assert(CanDereference<UniquePtr<int>>);
static_assert(std::is_constructible_v<UniquePtr<const int>, int*>);
static_assert(std::is_constructible_v<SharedPtr<const int>, int*>);
static_assert(sizeof(UniquePtr<int>) == sizeof(int*));
static_assert(sizeof(UniquePtr<int[]>) == sizeof(int*));
static_assert(sizeof(SharedPtr<int>) == sizeof(void*));
static_assert(sizeof(SharedPtr<int[]>) == sizeof(void*));

struct UniqueNode {
  inline static int live = 0;
  UniquePtr<UniqueNode> child;
  int value;
  explicit UniqueNode(int value) : value(value) { ++live; }
  ~UniqueNode() { --live; }
};

struct SharedNode {
  inline static int live = 0;
  SharedPtr<SharedNode> child;
  int value;
  explicit SharedNode(int value) : value(value) { ++live; }
  ~SharedNode() { --live; }
};

struct ConvertingDerived;
struct ConvertingBase {
  inline static int live = 0;
  UniquePtr<ConvertingDerived> child;
  ConvertingBase() { ++live; }
  virtual ~ConvertingBase();
};
struct ConvertingDerived : ConvertingBase {
  inline static int destroyed = 0;
  ~ConvertingDerived() { ++destroyed; }
};
ConvertingBase::~ConvertingBase() { --live; }

TEST(SmartPointerSafetyTests, UniqueMoveFromOwnedChildKeepsChildAlive) {
  {
    UniquePtr<UniqueNode> root(new UniqueNode(1));
    root->child.reset(new UniqueNode(2));
    auto* child = root->child.get();
    root = std::move(root->child);
    EXPECT_EQ(root.get(), child);
    EXPECT_EQ(root->value, 2);
    EXPECT_EQ(UniqueNode::live, 1);
  }
  EXPECT_EQ(UniqueNode::live, 0);
}

TEST(SmartPointerSafetyTests, UniqueConvertingMoveFromOwnedChildUsesVirtualDestructor) {
  ConvertingDerived::destroyed = 0;
  {
    UniquePtr<ConvertingBase> root(new ConvertingBase);
    root->child.reset(new ConvertingDerived);
    auto* child = root->child.get();
    root = std::move(root->child);
    EXPECT_EQ(root.get(), static_cast<ConvertingBase*>(child));
    EXPECT_EQ(ConvertingBase::live, 1);
    EXPECT_EQ(ConvertingDerived::destroyed, 0);
  }
  EXPECT_EQ(ConvertingBase::live, 0);
  EXPECT_EQ(ConvertingDerived::destroyed, 1);
}

TEST(SmartPointerSafetyTests, SharedCopyFromOwnedChildKeepsChildAlive) {
  {
    SharedPtr<SharedNode> root(new SharedNode(1));
    root->child.reset(new SharedNode(2));
    auto* child = root->child.get();
    root = root->child;
    EXPECT_EQ(root.get(), child);
    EXPECT_EQ(root->value, 2);
    EXPECT_EQ(root.use_count(), 1);
    EXPECT_EQ(SharedNode::live, 1);
  }
  EXPECT_EQ(SharedNode::live, 0);
}

TEST(SmartPointerSafetyTests, SharedMoveFromOwnedChildKeepsChildAlive) {
  {
    SharedPtr<SharedNode> root(new SharedNode(1));
    root->child.reset(new SharedNode(2));
    auto* child = root->child.get();
    root = std::move(root->child);
    EXPECT_EQ(root.get(), child);
    EXPECT_EQ(root->value, 2);
    EXPECT_EQ(root.use_count(), 1);
    EXPECT_EQ(SharedNode::live, 1);
  }
  EXPECT_EQ(SharedNode::live, 0);
}

TEST(SmartPointerSafetyTests, SharedDeletesDerivedThroughVirtualAdjustedBase) {
  Base::destroyed = Derived::destroyed = 0;
  {
    auto* raw = new Derived;
    SharedPtr<Base> owner(raw);
    EXPECT_EQ(owner.get(), static_cast<Base*>(raw));
    {
      auto copy = owner;
      auto moved = std::move(owner);
      EXPECT_EQ(copy.use_count(), 2);
      EXPECT_EQ(moved->value, 42);
      EXPECT_FALSE(owner);
    }
    EXPECT_EQ(Derived::destroyed, 1);
  }
  EXPECT_EQ(Base::destroyed, 1);
}

TEST(SmartPointerSafetyTests, SharedResetPreservesDerivedDeletionForEveryOwner) {
  Base::destroyed = Derived::destroyed = 0;
  {
    SharedPtr<Base> owner(new Derived);
    auto copy = owner;
    owner.reset(new Derived);
    EXPECT_EQ(Derived::destroyed, 0);
    EXPECT_EQ(owner.use_count(), 1);
    EXPECT_EQ(copy.use_count(), 1);
    copy.reset();
    EXPECT_EQ(Derived::destroyed, 1);
  }
  EXPECT_EQ(Base::destroyed, 2);
  EXPECT_EQ(Derived::destroyed, 2);
}

TEST(SmartPointerSafetyTests, SharedArrayCopiesDelayDeletionAndAllowSubscript) {
  Base::destroyed = 0;
  {
    SharedPtr<Base[]> values(new Base[3]);
    auto copy = values;
    values[1].value = 7;
    values.reset();
    EXPECT_EQ(Base::destroyed, 0);
    EXPECT_EQ(copy[1].value, 7);
    EXPECT_EQ(copy.use_count(), 1);
  }
  EXPECT_EQ(Base::destroyed, 3);
}

TEST(SmartPointerSafetyTests, ConstArraysCanTakeMutableArrays) {
  UniquePtr<const int[]> unique(new int[2]{1, 2});
  SharedPtr<const int[]> shared(new int[2]{3, 4});
  EXPECT_EQ(unique[1], 2);
  EXPECT_EQ(shared[1], 4);
  unique.reset(new int[1]{5});
  shared.reset(new int[1]{6});
  EXPECT_EQ(unique[0], 5);
  EXPECT_EQ(shared[0], 6);
}

TEST(SmartPointerSafetyTests, ResetToSamePointerPreservesOwnersAndCounts) {
  UniquePtr<UniqueNode> unique(new UniqueNode(1));
  unique.reset(unique.get());
  EXPECT_EQ(unique->value, 1);
  EXPECT_EQ(UniqueNode::live, 1);

  SharedPtr<SharedNode> shared(new SharedNode(2));
  shared.reset(shared.get());
  EXPECT_EQ(shared->value, 2);
  EXPECT_EQ(shared.use_count(), 1);
  auto copy = shared;
  shared.reset(shared.get());
  EXPECT_EQ(shared, copy);
  EXPECT_EQ(shared.use_count(), 2);
  EXPECT_EQ(SharedNode::live, 1);
}

TEST(SmartPointerSafetyTests, EmptyTypedNullAndMovedFromPointersCanBeReset) {
  SharedPtr<int> shared(static_cast<int*>(nullptr));
  UniquePtr<int> unique(nullptr);
  EXPECT_FALSE(shared);
  EXPECT_EQ(shared.use_count(), 0);
  EXPECT_FALSE(unique);
  shared.reset(new int(1));
  unique.reset(new int(2));
  SharedPtr<int> movedShared(std::move(shared));
  UniquePtr<int> movedUnique(std::move(unique));
  EXPECT_FALSE(shared);
  EXPECT_FALSE(unique);
  shared.reset(new int(3));
  unique.reset(new int(4));
  EXPECT_EQ(*movedShared, 1);
  EXPECT_EQ(*movedUnique, 2);
  EXPECT_EQ(*shared, 3);
  EXPECT_EQ(*unique, 4);
  shared.reset(nullptr);
  unique.reset(nullptr);
  EXPECT_EQ(shared, nullptr);
  EXPECT_EQ(unique, nullptr);
}

TEST(SmartPointerSafetyTests, UniqueArrayReleaseTransfersArrayWithoutDeletingIt) {
  Base::destroyed = 0;
  UniquePtr<Base[]> owner(new Base[2]);
  auto* raw = owner.release();
  EXPECT_FALSE(owner);
  EXPECT_EQ(Base::destroyed, 0);
  delete[] raw;
  EXPECT_EQ(Base::destroyed, 2);
}

TEST(SmartPointerSafetyTests, AssigningSharedOwnersOfSameResourceKeepsCounts) {
  SharedPtr<int> first(new int(42));
  auto second = first;
  first = second;
  EXPECT_EQ(first.use_count(), 2);
  first = std::move(second);
  EXPECT_FALSE(second);
  EXPECT_EQ(first.use_count(), 1);
  EXPECT_EQ(*first, 42);
  first = std::move(first);
  EXPECT_EQ(first.use_count(), 1);
  EXPECT_EQ(*first, 42);
}

struct ConcurrentObject {
  std::atomic<int>& destroyed;
  explicit ConcurrentObject(std::atomic<int>& destroyed) : destroyed(destroyed) {}
  ~ConcurrentObject() { destroyed.fetch_add(1); }
};

TEST(SmartPointerSafetyTests, SeparateSharedOwnersCanCopyAndDestroyConcurrently) {
  std::atomic<int> destroyed{0};
  SharedPtr<ConcurrentObject> owner(new ConcurrentObject(destroyed));
  constexpr int workers = 8;
  std::barrier start(workers + 1);
  std::vector<std::jthread> threads;
  for (int i = 0; i < workers; ++i) {
    threads.emplace_back([local = owner, &start] {
      start.arrive_and_wait();
      for (int j = 0; j < 10'000; ++j) {
        auto copy = local;
        auto another = copy;
        copy.reset();
        another = local;
      }
    });
  }
  owner.reset();
  EXPECT_EQ(destroyed.load(), 0);
  start.arrive_and_wait();
  threads.clear();
  EXPECT_EQ(destroyed.load(), 1);
}

TEST(SmartPointerSafetyTests, ReleasedPolymorphicObjectCanBeDeletedThroughBase) {
  Base::destroyed = Derived::destroyed = 0;
  UniquePtr<Base> owner(new Derived);
  Base* raw = owner.release();
  EXPECT_FALSE(owner);
  EXPECT_EQ(Derived::destroyed, 0);
  delete raw;
  EXPECT_EQ(Derived::destroyed, 1);
  EXPECT_EQ(Base::destroyed, 1);
}

TEST(SmartPointerSafetyTests, UniqueArraysMoveSwapAndResetWithoutDoubleDeletion) {
  Base::destroyed = 0;
  {
    UniquePtr<Base[]> first(new Base[2]);
    first[1].value = 7;
    auto* original = first.get();
    UniquePtr<Base[]> second(std::move(first));
    EXPECT_FALSE(first);
    EXPECT_EQ(second.get(), original);
    second.reset(second.get());
    second = std::move(second);
    EXPECT_EQ(second[1].value, 7);
    UniquePtr<Base[]> third(new Base[3]);
    third = std::move(second);
    EXPECT_FALSE(second);
    EXPECT_EQ(Base::destroyed, 3);
    first.swap(third);
    EXPECT_FALSE(third);
    first.reset(nullptr);
    EXPECT_FALSE(first);
    EXPECT_EQ(Base::destroyed, 5);
  }
  EXPECT_EQ(Base::destroyed, 5);
}

TEST(SmartPointerSafetyTests, SharedArraysMoveSwapAndResetPreserveSharedOwnership) {
  Base::destroyed = 0;
  {
    SharedPtr<Base[]> first(new Base[2]);
    first[1].value = 7;
    auto copy = first;
    auto* original = first.get();
    SharedPtr<Base[]> second(std::move(first));
    EXPECT_FALSE(first);
    EXPECT_EQ(second.get(), original);
    second.reset(second.get());
    second = std::move(second);
    EXPECT_EQ(second.use_count(), 2);
    SharedPtr<Base[]> third(new Base[3]);
    third = std::move(second);
    EXPECT_FALSE(second);
    EXPECT_EQ(Base::destroyed, 3);
    first.swap(third);
    EXPECT_FALSE(third);
    first.reset();
    EXPECT_EQ(copy.use_count(), 1);
    EXPECT_EQ(copy[1].value, 7);
    EXPECT_EQ(Base::destroyed, 3);
    copy.reset(nullptr);
    EXPECT_EQ(Base::destroyed, 5);
  }
  EXPECT_EQ(Base::destroyed, 5);
}

TEST(SmartPointerSafetyTests, ScalarConstOwnersAndReversedComparisonsWork) {
  UniquePtr<const int> unique(new int(1));
  SharedPtr<const int> shared(new int(2));
  EXPECT_EQ(*unique, 1);
  EXPECT_EQ(*shared, 2);
  EXPECT_TRUE(nullptr != unique);
  EXPECT_TRUE(nullptr != shared);
  unique.reset();
  shared.reset();
  EXPECT_TRUE(nullptr == unique);
  EXPECT_TRUE(nullptr == shared);
  EXPECT_FALSE(unique != nullptr);
  EXPECT_FALSE(shared != nullptr);
}

} // Безымянное пространство имён.
