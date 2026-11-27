#include <gtest/gtest.h>

#include "ast/expressionTree.h"
#include "runtime/interpreter.h"

#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace {
class CountedExpr final : public ast::Expr {
private:
  int& liveCount;

public:
  explicit CountedExpr(int& liveCount) : liveCount(liveCount) {
    ++this->liveCount;
  }

  ~CountedExpr() override {
    --liveCount;
  }

  void accept(ast::ExprVisitor&) const override {}
};

ast::ExpressionTree trackedLeaf(int& live) {
  return ast::ExpressionTree::makeLeaf(UniquePtr<ast::Expr>(new CountedExpr(live)));
}
}

static_assert(!std::is_copy_constructible_v<ast::ExpressionTree>);
static_assert(std::is_nothrow_move_constructible_v<ast::ExpressionTree>);

TEST(ExpressionTreeTests, PreservesNodeAddressesWhenCombiningAndMovingTrees) {
  int live = 0;
  {
    auto left = trackedLeaf(live);
    auto right = trackedLeaf(live);
    const auto* leftRoot = left.getRoot();
    const auto* rightRoot = right.getRoot();
    auto tree = ast::ExpressionTree::makeBinary(TokenType::Plus, std::move(left), std::move(right));

    EXPECT_EQ(left.getRoot(), nullptr);
    EXPECT_EQ(right.getRoot(), nullptr);
    EXPECT_EQ(left.size(), 0u);
    EXPECT_EQ(right.size(), 0u);
    EXPECT_EQ(tree.size(), 3u);
    const auto* root = dynamic_cast<const ast::Binary*>(tree.getRoot());
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->operation, TokenType::Plus);
    EXPECT_EQ(root->leftOperand(), leftRoot);
    EXPECT_EQ(root->rightOperand(), rightRoot);

    auto replacement = trackedLeaf(live);
    EXPECT_EQ(live, 3);
    replacement = std::move(tree);
    EXPECT_EQ(live, 2);
    EXPECT_EQ(replacement.getRoot(), root);
    EXPECT_EQ(tree.getRoot(), nullptr);
  }
  EXPECT_EQ(live, 0);
}

TEST(ExpressionTreeTests, FailedCombinationPreservesExistingOperands) {
  int live = 0;
  {
    auto leaf = trackedLeaf(live);
    const auto* original = leaf.getRoot();
    ast::ExpressionTree empty;
    EXPECT_THROW(ast::ExpressionTree::makeBinary(TokenType::Plus, std::move(leaf), std::move(empty)),
                 std::invalid_argument);
    EXPECT_THROW(ast::ExpressionTree::makeBinary(TokenType::Plus, std::move(leaf), std::move(leaf)),
                 std::invalid_argument);
    EXPECT_THROW(ast::ExpressionTree::makeLeaf(UniquePtr<ast::Expr>{}), std::invalid_argument);
    EXPECT_EQ(leaf.getRoot(), original);
    EXPECT_EQ(leaf.size(), 1u);
    EXPECT_EQ(live, 1);
  }
  EXPECT_EQ(live, 0);
}

TEST(ExpressionTreeTests, DestroysDeepTreeWithoutRecursion) {
  int live = 0;
  {
    auto tree = trackedLeaf(live);
    for (int i = 0; i < 100000; ++i) {
      tree = ast::ExpressionTree::makeBinary(TokenType::Plus, std::move(tree), trackedLeaf(live));
    }
    EXPECT_EQ(tree.size(), 200001u);
    EXPECT_EQ(live, 100001);
  }
  EXPECT_EQ(live, 0);
}

TEST(ExpressionTreeTests, ProgramOwnsTreeAfterExecutionError) {
  int live = 0;
  {
    Array<UniquePtr<ast::Statement>> program;
    program.push_back(UniquePtr<ast::Statement>(new ast::Print(trackedLeaf(live))));

    Interpreter interpreter;
    std::ostringstream output;
    EXPECT_THROW(interpreter.execute(program, output), std::logic_error);
    EXPECT_EQ(live, 1);
    EXPECT_TRUE(output.str().empty());
  }
  EXPECT_EQ(live, 0);
}

TEST(ExpressionTreeTests, StoresSignedLiteralInSingleMovableLeaf) {
  auto tree = ast::ExpressionTree::makeLeaf(UniquePtr<ast::Expr>(new ast::NumberLiteral("-0.5")));
  const auto* number = dynamic_cast<const ast::NumberLiteral*>(tree.getRoot());
  ASSERT_NE(number, nullptr);
  EXPECT_EQ(number->lexeme, "-0.5");
  EXPECT_EQ(tree.size(), 1u);
  auto moved = std::move(tree);
  EXPECT_EQ(moved.getRoot(), number);
  EXPECT_EQ(moved.size(), 1u);
  EXPECT_EQ(tree.getRoot(), nullptr);
  EXPECT_EQ(tree.size(), 0u);
}
