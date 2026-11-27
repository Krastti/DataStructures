#include "../../DataStructures/tests/AllocationFailure.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include <gtest/gtest.h>

#include <cstddef>
#include <new>
#include <stdexcept>
#include <utility>

namespace {

TEST(ExpressionTreeAllocationTests, AllocationFailureKeepsOperandTrees) {
  auto left = ast::ExpressionTree::makeLeaf(UniquePtr<ast::Expr>(new ast::NumberLiteral("1")));
  auto right = ast::ExpressionTree::makeLeaf(UniquePtr<ast::Expr>(new ast::NumberLiteral("2")));
  const auto* leftRoot = left.getRoot();
  const auto* rightRoot = right.getRoot();
  bool failed = false;

  {
    test::RejectAllocations guard;
    try { ast::ExpressionTree::makeBinary(TokenType::Plus, std::move(left), std::move(right)); }
    catch (const std::bad_alloc&) { failed = true; }
  }

  EXPECT_TRUE(failed);
  EXPECT_EQ(left.getRoot(), leftRoot);
  EXPECT_EQ(right.getRoot(), rightRoot);
  EXPECT_EQ(left.size(), 1u);
  EXPECT_EQ(right.size(), 1u);
}

TEST(ExpressionTreeAllocationTests, ParseErrorReleasesPartialProgram) {
  const auto tokens = Lexer::tokenize("remember longIdentifier = 1 + 2\nprint (3 * 4 + 5");
  std::ptrdiff_t balance = 0;
  bool failed = false;
  {
    // Токены созданы заранее. Считаем только ресурсы, выделенные и освобождённые при разборе.
    test::TrackAllocations guard(balance);
    try { Parser{}.parse(tokens); }
    catch (const std::invalid_argument&) { failed = true; }
  }
  EXPECT_TRUE(failed);
  EXPECT_EQ(balance, 0);
}

}

TEST(ExpressionTreeAllocationTests, ParseErrorReleasesSignedLiteralLeaves) {
  const auto tokens = Lexer::tokenize("remember x = -2\nprint (-3 *");
  std::ptrdiff_t balance = 0;
  bool failed = false;
  {
    test::TrackAllocations guard(balance);
    try { Parser::parse(tokens); }
    catch (const std::invalid_argument&) { failed = true; }
  }
  EXPECT_TRUE(failed);
  EXPECT_EQ(balance, 0);
}
