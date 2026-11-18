#include <gtest/gtest.h>

#include "../../interpreter/headers/lexer.h"
#include "../../interpreter/headers/parser.h"
#include "../../interpreter/headers/visitors.h"

#include <stdexcept>
#include <string>
#include <utility>

namespace {
auto parse(const std::string& source) {
  Parser parser;
  return parser.parse(Lexer::tokenize(source));
}
}

TEST(ParserTests, ParsesEmptyProgramAndBlankLines) {
  EXPECT_EQ(parse("").size(), 0u);
  EXPECT_EQ(parse("\n\r\n\n").size(), 0u);
}

TEST(ParserTests, ParsesRememberAndPrintInOrder) {
  const auto program = parse("remember x = 42\r\n\nprint x");
  ASSERT_EQ(program.size(), 2u);
  const auto* remember = dynamic_cast<const ast::Remember*>(program[0].get());
  ASSERT_NE(remember, nullptr);
  EXPECT_EQ(remember->variable, "x");
  const auto* number = dynamic_cast<const ast::NumberLiteral*>(remember->right.getRoot());
  ASSERT_NE(number, nullptr);
  EXPECT_EQ(number->lexeme, "42");
  const auto* print = dynamic_cast<const ast::Print*>(program[1].get());
  ASSERT_NE(print, nullptr);
  const auto* name = dynamic_cast<const ast::Identifier*>(print->forOutput.getRoot());
  ASSERT_NE(name, nullptr);
  EXPECT_EQ(name->name, "x");
}

TEST(ParserTests, MultiplicationBindsMoreTightlyThanAddition) {
  const auto program = parse("print 2 + 3 * 4");
  const auto* print = dynamic_cast<const ast::Print*>(program[0].get());
  ASSERT_NE(print, nullptr);
  const auto* sum = dynamic_cast<const ast::Binary*>(print->forOutput.getRoot());
  ASSERT_NE(sum, nullptr);
  EXPECT_EQ(sum->operation, TokenType::Plus);
  const auto* left = dynamic_cast<const ast::NumberLiteral*>(sum->leftOperand());
  ASSERT_NE(left, nullptr);
  EXPECT_EQ(left->lexeme, "2");
  const auto* product = dynamic_cast<const ast::Binary*>(sum->rightOperand());
  ASSERT_NE(product, nullptr);
  EXPECT_EQ(product->operation, TokenType::Star);
}

TEST(ParserTests, ParenthesesOverridePrecedence) {
  const auto program = parse("print (2 + 3) * 4\n");
  const auto* print = dynamic_cast<const ast::Print*>(program[0].get());
  ASSERT_NE(print, nullptr);
  const auto* product = dynamic_cast<const ast::Binary*>(print->forOutput.getRoot());
  ASSERT_NE(product, nullptr);
  EXPECT_EQ(product->operation, TokenType::Star);
  const auto* sum = dynamic_cast<const ast::Binary*>(product->leftOperand());
  ASSERT_NE(sum, nullptr);
  EXPECT_EQ(sum->operation, TokenType::Plus);
}

TEST(ParserTests, OperatorsAtEachLevelAssociateToTheLeft) {
  for (const auto* source : {"print 10 - 3 - 2", "print 20 / 5 / 2",
                              "print 1 < 2 < 3", "print 1 == 2 != 3"}) {
    const auto program = parse(source);
    const auto* print = dynamic_cast<const ast::Print*>(program[0].get());
    ASSERT_NE(print, nullptr);
    const auto* binary = dynamic_cast<const ast::Binary*>(print->forOutput.getRoot());
    ASSERT_NE(binary, nullptr);
    EXPECT_NE(dynamic_cast<const ast::Binary*>(binary->leftOperand()), nullptr);
    EXPECT_NE(dynamic_cast<const ast::NumberLiteral*>(binary->rightOperand()), nullptr);
  }
}

TEST(ParserTests, EqualityBindsLessTightlyThanComparisonAndArithmetic) {
  const auto program = parse("print 1 + 2 < 4 == 5 >= 6");
  const auto* print = dynamic_cast<const ast::Print*>(program[0].get());
  ASSERT_NE(print, nullptr);
  const auto* equality = dynamic_cast<const ast::Binary*>(print->forOutput.getRoot());
  ASSERT_NE(equality, nullptr);
  EXPECT_EQ(equality->operation, TokenType::EqualEqual);
  const auto* less = dynamic_cast<const ast::Binary*>(equality->leftOperand());
  const auto* greater = dynamic_cast<const ast::Binary*>(equality->rightOperand());
  ASSERT_NE(less, nullptr);
  ASSERT_NE(greater, nullptr);
  EXPECT_EQ(less->operation, TokenType::Less);
  EXPECT_EQ(greater->operation, TokenType::GreaterEqual);
  const auto* sum = dynamic_cast<const ast::Binary*>(less->leftOperand());
  ASSERT_NE(sum, nullptr);
  EXPECT_EQ(sum->operation, TokenType::Plus);
}

TEST(ParserTests, PreservesQuotedStringLexemes) {
  const auto program = parse("remember text = \"hello world\"\nprint (\"\")");
  ASSERT_EQ(program.size(), 2u);
  const auto* remember = dynamic_cast<const ast::Remember*>(program[0].get());
  ASSERT_NE(remember, nullptr);
  const auto* text = dynamic_cast<const ast::StringLiteral*>(remember->right.getRoot());
  ASSERT_NE(text, nullptr);
  EXPECT_EQ(text->lexeme, "\"hello world\"");
  const auto* print = dynamic_cast<const ast::Print*>(program[1].get());
  ASSERT_NE(print, nullptr);
  const auto* empty = dynamic_cast<const ast::StringLiteral*>(print->forOutput.getRoot());
  ASSERT_NE(empty, nullptr);
  EXPECT_EQ(empty->lexeme, "\"\"");
}

TEST(ParserTests, GrowsAndMovesProgramWithoutLosingStatements) {
  std::string source;
  for (int i = 0; i < 100; ++i) source += "print " + std::to_string(i) + "\n";
  auto program = parse(source);
  auto moved = std::move(program);
  EXPECT_EQ(program.size(), 0u);
  ASSERT_EQ(moved.size(), 100u);
  for (std::size_t i = 0; i < moved.size(); ++i) {
    const auto* print = dynamic_cast<const ast::Print*>(moved[i].get());
    ASSERT_NE(print, nullptr);
    const auto* number = dynamic_cast<const ast::NumberLiteral*>(print->forOutput.getRoot());
    ASSERT_NE(number, nullptr);
    EXPECT_EQ(number->lexeme, std::to_string(i));
  }
}

TEST(ParserTests, RejectsMalformedStatementsAndExpressions) {
  for (const auto* source : {"remember", "remember = 1", "remember x 1", "remember x =",
      "print", "print ()", "print (1 + 2", "print 1 +", "print 1 2", "print 1 print 2",
      "1 + 2", "print -1", "print 1)", "print 1 + * 2", "print 1\nremember x = ("}) {
    EXPECT_THROW(parse(source), std::invalid_argument) << source;
  }
}

TEST(ParserTests, RejectsMissingOrPrematureEndOfFile) {
  Parser parser;
  Array<Token> empty;
  EXPECT_THROW(parser.parse(empty), std::invalid_argument);
  Array<Token> missing;
  missing.push_back({TokenType::Print, "print"});
  EXPECT_THROW(parser.parse(missing), std::invalid_argument);
  auto premature = Lexer::tokenize("");
  premature.push_back({TokenType::EndOfFile, ""});
  EXPECT_THROW(parser.parse(premature), std::invalid_argument);
}

TEST(ParserTests, CanBeReusedAfterParseError) {
  Parser parser;
  EXPECT_THROW(parser.parse(Lexer::tokenize("print 1\nprint (")), std::invalid_argument);
  const auto program = parser.parse(Lexer::tokenize("print 2"));
  EXPECT_EQ(program.size(), 1u);
}

namespace {
class RecordingVisitor : public ast::ExprVisitor, public ast::StatementVisitor {
public:
  Array<std::string> events;
  bool visitChildren = false;
  const ast::Expr* lastExpression = nullptr;
  const ast::Statement* lastStatement = nullptr;

  void visit(const ast::NumberLiteral& node) override {
    lastExpression = &node;
    events.push_back("number:" + node.lexeme);
  }

  void visit(const ast::Identifier& node) override {
    lastExpression = &node;
    events.push_back("identifier:" + node.name);
  }

  void visit(const ast::StringLiteral& node) override {
    lastExpression = &node;
    events.push_back("string:" + node.lexeme);
  }

  void visit(const ast::Binary& node) override {
    lastExpression = &node;
    events.push_back("binary");
    if (visitChildren) {
      node.leftOperand()->accept(*this);
      node.rightOperand()->accept(*this);
    }
  }

  void visit(const ast::Remember& node) override {
    lastStatement = &node;
    events.push_back("remember:" + node.variable);
    if (visitChildren) node.right.getRoot()->accept(*this);
  }

  void visit(const ast::Print& node) override {
    lastStatement = &node;
    events.push_back("print");
    if (visitChildren) node.forOutput.getRoot()->accept(*this);
  }
};
}

TEST(ParserTests, AcceptDispatchesEachExpressionThroughConstBaseReference) {
  const ast::NumberLiteral number("12");
  const ast::Identifier identifier("x");
  const ast::StringLiteral string("\"hello\"");
  const auto binary = ast::ExpressionTree::makeBinary(TokenType::Plus,
      ast::ExpressionTree::makeLeaf(UniquePtr<ast::Expr>(new ast::NumberLiteral("1"))),
      ast::ExpressionTree::makeLeaf(UniquePtr<ast::Expr>(new ast::NumberLiteral("2"))));
  const ast::Expr* nodes[] = {&number, &identifier, &string, binary.getRoot()};
  const std::string expected[] = {"number:12", "identifier:x", "string:\"hello\"", "binary"};
  RecordingVisitor visitor;
  ast::ExprVisitor& expressionVisitor = visitor;

  for (std::size_t i = 0; i < 4; ++i) {
    nodes[i]->accept(expressionVisitor);
    EXPECT_EQ(visitor.lastExpression, nodes[i]);
    ASSERT_EQ(visitor.events.size(), i + 1);
    EXPECT_EQ(visitor.events[i], expected[i]);
  }
}

TEST(ParserTests, AcceptDispatchesStatementsWithoutVisitingChildrenAutomatically) {
  const auto program = parse("remember x = 1\nprint x");
  RecordingVisitor visitor;
  ast::StatementVisitor& statementVisitor = visitor;
  for (std::size_t i = 0; i < program.size(); ++i) {
    const ast::Statement& statement = *program[i];
    statement.accept(statementVisitor);
    EXPECT_EQ(visitor.lastStatement, &statement);
  }
  ASSERT_EQ(visitor.events.size(), 2u);
  EXPECT_EQ(visitor.events[0], "remember:x");
  EXPECT_EQ(visitor.events[1], "print");
  EXPECT_EQ(visitor.lastExpression, nullptr);
}

TEST(ParserTests, VisitorCanTraverseParsedTreeInChosenOrder) {
  const auto program = parse("remember x = 2 + 3 * 4\nprint x\nprint \"hello\"");
  RecordingVisitor visitor;
  visitor.visitChildren = true;
  for (std::size_t i = 0; i < program.size(); ++i) program[i]->accept(visitor);

  const std::string expected[] = {
    "remember:x", "binary", "number:2", "binary", "number:3", "number:4",
    "print", "identifier:x", "print", "string:\"hello\""
  };
  ASSERT_EQ(visitor.events.size(), 10u);
  for (std::size_t i = 0; i < visitor.events.size(); ++i) {
    EXPECT_EQ(visitor.events[i], expected[i]);
  }
}
