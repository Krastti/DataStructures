#include "Stack/stack.h"

#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

TEST(StackTests, TopOverloadsReferToTheStoredElements) {
  Stack<int> stack;
  const Stack<int>& constStack = stack;
  static_assert(std::is_same_v<decltype(stack.top()), int&>);
  static_assert(std::is_same_v<decltype(constStack.top()), const int&>);

  const int firstValue = 1;
  stack.push(firstValue);
  int& first = stack.top();
  const int secondValue = 2;
  stack.push(secondValue);
  stack.top() = 3;

  EXPECT_EQ(constStack.top(), 3);
  EXPECT_EQ(&constStack.top(), &stack.top());
  EXPECT_EQ(first, 1);

  stack.pop();
  EXPECT_EQ(&constStack.top(), &first);
}

TEST(StackTests, TopAndPopThrowOnEmptyStack) {
  Stack<int> stack;
  const Stack<int>& constStack = stack;

  EXPECT_THROW(static_cast<void>(stack.top()), std::logic_error);
  EXPECT_THROW(static_cast<void>(constStack.top()), std::logic_error);
  EXPECT_THROW(stack.pop(), std::logic_error);
}

TEST(StackTests, PushCopiesLvalueIntoTheStack) {
  Stack<std::string> stack;
  std::string value = "original";

  stack.push(value);
  stack.push(std::string("temporary"));
  EXPECT_EQ(value, "original");
  EXPECT_EQ(stack.top(), "temporary");

  stack.pop();
  value = "changed";
  EXPECT_EQ(stack.top(), "original");
}

TEST(StackTests, PushMovesOwnershipAndReleasesElementsInLifoOrder) {
  int released = 0;
  auto deleter = [&released](int* value) {
    released = released * 10 + *value;
    delete value;
  };
  using Pointer = std::unique_ptr<int, decltype(deleter)>;

  {
    Stack<Pointer> stack;
    Pointer first(new int(1), deleter);
    stack.push(std::move(first));
    EXPECT_FALSE(first);

    stack.push(Pointer(new int(2), deleter));
    const auto& constStack = stack;
    ASSERT_TRUE(constStack.top());
    EXPECT_EQ(*constStack.top(), 2);

    stack.pop();
    EXPECT_EQ(released, 2);
    ASSERT_TRUE(constStack.top());
    EXPECT_EQ(*constStack.top(), 1);
  }

  EXPECT_EQ(released, 21);
}
