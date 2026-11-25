#include <gtest/gtest.h>

#include "../headers/lexer.h"
#include "../headers/parser.h"
#include "../headers/visitors.h"

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

TEST(ParserTests, BindsNumericUpdatesBeforeMultiplication) {
  const auto program = parse("print ++x * y--");
  ASSERT_EQ(program.size(), 1u);
  const auto* print = dynamic_cast<const ast::Print*>(program[0].get());
  ASSERT_NE(print, nullptr);
  const auto* product = dynamic_cast<const ast::Binary*>(print->forOutput.getRoot());
  ASSERT_NE(product, nullptr);
  EXPECT_EQ(product->operation, TokenType::Star);
  const auto* prefix = dynamic_cast<const ast::Update*>(product->leftOperand());
  const auto* postfix = dynamic_cast<const ast::Update*>(product->rightOperand());
  ASSERT_NE(prefix, nullptr);
  ASSERT_NE(postfix, nullptr);
  EXPECT_EQ(prefix->variable, "x");
  EXPECT_EQ(prefix->operation, TokenType::Increment);
  EXPECT_FALSE(prefix->postfix);
  EXPECT_EQ(postfix->variable, "y");
  EXPECT_EQ(postfix->operation, TokenType::Decrement);
  EXPECT_TRUE(postfix->postfix);
}

TEST(ParserTests, RequiresVariableNamesForUpdates) {
  for (const char* source : {"print ++1", "print 1--", "print (x)++", "print f()--",
                             "x++ + 1", "x++--", "x +=", "print x += 1", "1 -= 2"}) {
    EXPECT_THROW(parse(source), std::invalid_argument) << source;
  }
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

TEST(ParserTests, DistinguishesEmptyCallFromIdentifier) {
  const auto program = parse("print f()\nprint f");
  ASSERT_EQ(program.size(), 2u);

  const auto* callPrint = dynamic_cast<const ast::Print*>(program[0].get());
  ASSERT_NE(callPrint, nullptr);
  const auto* call = dynamic_cast<const ast::Call*>(callPrint->forOutput.getRoot());
  ASSERT_NE(call, nullptr);
  EXPECT_EQ(call->name, "f");
  EXPECT_EQ(call->arguments.size(), 0u);

  const auto* namePrint = dynamic_cast<const ast::Print*>(program[1].get());
  ASSERT_NE(namePrint, nullptr);
  const auto* name = dynamic_cast<const ast::Identifier*>(namePrint->forOutput.getRoot());
  ASSERT_NE(name, nullptr);
  EXPECT_EQ(name->name, "f");
}

TEST(ParserTests, ContinuesParsingExpressionAfterEmptyCall) {
  const auto program = parse("print f() + 2");
  ASSERT_EQ(program.size(), 1u);
  const auto* print = dynamic_cast<const ast::Print*>(program[0].get());
  ASSERT_NE(print, nullptr);
  const auto* sum = dynamic_cast<const ast::Binary*>(print->forOutput.getRoot());
  ASSERT_NE(sum, nullptr);
  EXPECT_EQ(sum->operation, TokenType::Plus);
  EXPECT_NE(dynamic_cast<const ast::Call*>(sum->leftOperand()), nullptr);
  EXPECT_NE(dynamic_cast<const ast::NumberLiteral*>(sum->rightOperand()), nullptr);
}

TEST(ParserTests, RequiresClosingParenthesisForCall) {
  EXPECT_THROW(parse("print f("), std::invalid_argument);
  EXPECT_THROW(parse("print f(1"), std::invalid_argument);
}

TEST(ParserTests, ParsesCallArgumentsAsExpressionsInOrder) {
  const auto program = parse("print 2 * f(3 + 4, g(5))");
  ASSERT_EQ(program.size(), 1u);
  const auto* print = dynamic_cast<const ast::Print*>(program[0].get());
  ASSERT_NE(print, nullptr);
  const auto* product = dynamic_cast<const ast::Binary*>(print->forOutput.getRoot());
  ASSERT_NE(product, nullptr);
  EXPECT_EQ(product->operation, TokenType::Star);
  const auto* call = dynamic_cast<const ast::Call*>(product->rightOperand());
  ASSERT_NE(call, nullptr);
  EXPECT_EQ(call->name, "f");
  ASSERT_EQ(call->arguments.size(), 2u);

  const auto* sum = dynamic_cast<const ast::Binary*>(call->arguments[0].getRoot());
  ASSERT_NE(sum, nullptr);
  EXPECT_EQ(sum->operation, TokenType::Plus);
  const auto* nested = dynamic_cast<const ast::Call*>(call->arguments[1].getRoot());
  ASSERT_NE(nested, nullptr);
  EXPECT_EQ(nested->name, "g");
  ASSERT_EQ(nested->arguments.size(), 1u);
  const auto* number = dynamic_cast<const ast::NumberLiteral*>(nested->arguments[0].getRoot());
  ASSERT_NE(number, nullptr);
  EXPECT_EQ(number->lexeme, "5");
}

TEST(ParserTests, RejectsMalformedCallArgumentLists) {
  for (const char* source : {"print f(,1)", "print f(1,)", "print f(1,,2)", "print f(1 2)"}) {
    EXPECT_THROW(parse(source), std::invalid_argument) << source;
  }
}

TEST(ParserTests, ParsesParameterlessFunctionDeclarationAndFollowingStatement) {
  const auto program = parse("function f() -> g() end\nprint f()");
  ASSERT_EQ(program.size(), 2u);
  const auto* function = dynamic_cast<const ast::Function*>(program[0].get());
  ASSERT_NE(function, nullptr);
  EXPECT_EQ(function->name, "f");
  EXPECT_EQ(function->parameters.size(), 0u);
  const auto* call = dynamic_cast<const ast::Call*>(function->result.getRoot());
  ASSERT_NE(call, nullptr);
  EXPECT_EQ(call->name, "g");
  EXPECT_NE(dynamic_cast<const ast::Print*>(program[1].get()), nullptr);
}

TEST(ParserTests, AcceptsFunctionEndImmediatelyBeforeEof) {
  EXPECT_EQ(parse("function f() -> 1 end").size(), 1u);
}

TEST(ParserTests, SeparatesMultilineFunctionBodyFromFinalReturnExpression) {
  const auto program = parse(
      "function f(x) ->\r\n"
      "  remember y = x + 1\r\n"
      "  when y > 1\r\n"
      "    remember y = y * 2\r\n"
      "  end\r\n"
      "  return y\r\n"
      "\r\n"
      "end");
  ASSERT_EQ(program.size(), 1u);
  const auto* function = dynamic_cast<const ast::Function*>(program[0].get());
  ASSERT_NE(function, nullptr);
  ASSERT_EQ(function->body.size(), 2u);
  EXPECT_NE(dynamic_cast<const ast::Remember*>(function->body[0].get()), nullptr);
  EXPECT_NE(dynamic_cast<const ast::When*>(function->body[1].get()), nullptr);
  const auto* result = dynamic_cast<const ast::Identifier*>(function->result.getRoot());
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->name, "y");
}

TEST(ParserTests, RejectsMissingMisplacedOrRepeatedFunctionReturn) {
  for (const char* source : {
      "return 1",
      "function f() ->\nremember x = 1\nend",
      "function f() ->\nwhen 1 < 2\nreturn 1\nend\nreturn 0\nend",
      "function f() ->\nfor i in range(1)\nreturn i\nend\nreturn 0\nend",
      "function f() ->\nreturn\nend",
      "function f() ->\nreturn 1 end",
      "function f() ->\nreturn 1\nreturn 2\nend",
      "function f() ->\nreturn 1\nprint 2\nend",
      "function f() ->\nreturn 1\n"}) {
    EXPECT_THROW(parse(source), std::invalid_argument) << source;
  }
}

TEST(ParserTests, ParsesFunctionParametersInDeclarationOrder) {
  const auto program = parse("function add(a, b) -> a + b end");
  ASSERT_EQ(program.size(), 1u);
  const auto* function = dynamic_cast<const ast::Function*>(program[0].get());
  ASSERT_NE(function, nullptr);
  EXPECT_EQ(function->name, "add");
  ASSERT_EQ(function->parameters.size(), 2u);
  EXPECT_EQ(function->parameters[0], "a");
  EXPECT_EQ(function->parameters[1], "b");
  const auto* sum = dynamic_cast<const ast::Binary*>(function->result.getRoot());
  ASSERT_NE(sum, nullptr);
  EXPECT_EQ(sum->operation, TokenType::Plus);
}

TEST(ParserTests, RejectsRepeatedFunctionParameterNames) {
  EXPECT_THROW(parse("function f(a, b, a) -> a end"), std::invalid_argument);
}

TEST(ParserTests, RejectsMalformedFunctionParameterLists) {
  for (const char* source : {"function f(,a) -> a end", "function f(a,) -> a end",
                             "function f(a,,b) -> a end", "function f(a b) -> a end",
                             "function f(a + 1) -> a end", "function f(1) -> 1 end",
                             "function f(print) -> 1 end"}) {
    EXPECT_THROW(parse(source), std::invalid_argument) << source;
  }
}

TEST(ParserTests, RejectsMalformedFunctionDeclarations) {
  for (const char* source : {"function () -> 1 end", "function f( -> 1 end", "function f() 1 end",
                             "function f() -> end", "function f() -> 1", "function f() -> 1 end print 2"}) {
    EXPECT_THROW(parse(source), std::invalid_argument) << source;
  }
}

TEST(ParserTests, RejectsNestedFunctionDeclaration) {
  EXPECT_THROW(parse("when 1 == 1\nfunction f() -> 1 end\nend"), std::invalid_argument);
}

TEST(ParserTests, ParsesNestedWhenAndKeepsEachBranchAtItsOwnLevel) {
  const auto program = parse(
      "when (x + 1) < 3\n"
      "  when x == 1\n"
      "    print 1\n"
      "  otherwise\n"
      "    remember x = 2\n"
      "  end\n"
      "  print x\n"
      "otherwise\n"
      "  remember x = 3\n"
      "end\n"
      "print x");
  ASSERT_EQ(program.size(), 2u);
  const auto* outer = dynamic_cast<const ast::When*>(program[0].get());
  ASSERT_NE(outer, nullptr);
  const auto* condition = dynamic_cast<const ast::Binary*>(outer->condition.getRoot());
  ASSERT_NE(condition, nullptr);
  EXPECT_EQ(condition->operation, TokenType::Less);
  ASSERT_EQ(outer->thenBranch.size(), 2u);
  ASSERT_EQ(outer->otherwiseBranch.size(), 1u);

  const auto* inner = dynamic_cast<const ast::When*>(outer->thenBranch[0].get());
  ASSERT_NE(inner, nullptr);
  ASSERT_EQ(inner->thenBranch.size(), 1u);
  ASSERT_EQ(inner->otherwiseBranch.size(), 1u);
  EXPECT_NE(dynamic_cast<const ast::Print*>(inner->thenBranch[0].get()), nullptr);
  EXPECT_NE(dynamic_cast<const ast::Remember*>(inner->otherwiseBranch[0].get()), nullptr);
  EXPECT_NE(dynamic_cast<const ast::Print*>(outer->thenBranch[1].get()), nullptr);
  EXPECT_NE(dynamic_cast<const ast::Remember*>(outer->otherwiseBranch[0].get()), nullptr);
  EXPECT_NE(dynamic_cast<const ast::Print*>(program[1].get()), nullptr);
}

TEST(ParserTests, ParsesEmptyWhenBranchesAndEndBeforeEof) {
  const auto program = parse("when 1 < 2\r\n\r\nend\r\nwhen flag\notherwise\n\nend");
  ASSERT_EQ(program.size(), 2u);
  for (std::size_t i = 0; i < program.size(); ++i) {
    const auto* when = dynamic_cast<const ast::When*>(program[i].get());
    ASSERT_NE(when, nullptr);
    EXPECT_NE(when->condition.getRoot(), nullptr);
    EXPECT_EQ(when->thenBranch.size(), 0u);
    EXPECT_EQ(when->otherwiseBranch.size(), 0u);
  }
}

TEST(ParserTests, ParsesWhenInsideRepeatWhile) {
  const auto program = parse(
      "repeat while n < 3\n"
      "  when n == 1\n"
      "    print 1\n"
      "  otherwise\n"
      "    print n\n"
      "  end\n"
      "  remember n = n + 1\n"
      "end\n"
      "print n");
  ASSERT_EQ(program.size(), 2u);
  const auto* loop = dynamic_cast<const ast::RepeatWhile*>(program[0].get());
  ASSERT_NE(loop, nullptr);
  const auto* condition = dynamic_cast<const ast::Binary*>(loop->condition.getRoot());
  ASSERT_NE(condition, nullptr);
  EXPECT_EQ(condition->operation, TokenType::Less);
  ASSERT_EQ(loop->body.size(), 2u);

  const auto* when = dynamic_cast<const ast::When*>(loop->body[0].get());
  ASSERT_NE(when, nullptr);
  ASSERT_EQ(when->thenBranch.size(), 1u);
  ASSERT_EQ(when->otherwiseBranch.size(), 1u);
  EXPECT_NE(dynamic_cast<const ast::Print*>(when->thenBranch[0].get()), nullptr);
  EXPECT_NE(dynamic_cast<const ast::Print*>(when->otherwiseBranch[0].get()), nullptr);
  EXPECT_NE(dynamic_cast<const ast::Remember*>(loop->body[1].get()), nullptr);
  EXPECT_NE(dynamic_cast<const ast::Print*>(program[1].get()), nullptr);
}

TEST(ParserTests, ParsesRepeatWhileInsideWhen) {
  const auto program = parse(
      "when flag\n"
      "  repeat while n < 3\n"
      "    remember n = n + 1\n"
      "  end\n"
      "  print n\n"
      "otherwise\n"
      "  repeat while other\n"
      "\n"
      "  end\n"
      "end");
  ASSERT_EQ(program.size(), 1u);
  const auto* when = dynamic_cast<const ast::When*>(program[0].get());
  ASSERT_NE(when, nullptr);
  ASSERT_EQ(when->thenBranch.size(), 2u);
  ASSERT_EQ(when->otherwiseBranch.size(), 1u);

  const auto* loop = dynamic_cast<const ast::RepeatWhile*>(when->thenBranch[0].get());
  ASSERT_NE(loop, nullptr);
  ASSERT_EQ(loop->body.size(), 1u);
  EXPECT_NE(dynamic_cast<const ast::Remember*>(loop->body[0].get()), nullptr);
  EXPECT_NE(dynamic_cast<const ast::Print*>(when->thenBranch[1].get()), nullptr);

  const auto* emptyLoop = dynamic_cast<const ast::RepeatWhile*>(when->otherwiseBranch[0].get());
  ASSERT_NE(emptyLoop, nullptr);
  EXPECT_EQ(emptyLoop->body.size(), 0u);
}

TEST(ParserTests, ParsesForRangeVariableStopAndBody) {
  const auto program = parse("for index in range((1 + 2))\nprint index\nend\nprint index");
  ASSERT_EQ(program.size(), 2u);
  const auto* loop = dynamic_cast<const ast::For*>(program[0].get());
  ASSERT_NE(loop, nullptr);
  EXPECT_EQ(loop->variable, "index");
  const auto* stop = dynamic_cast<const ast::Binary*>(loop->stop.getRoot());
  ASSERT_NE(stop, nullptr);
  EXPECT_EQ(stop->operation, TokenType::Plus);
  ASSERT_EQ(loop->body.size(), 1u);
  EXPECT_NE(dynamic_cast<const ast::Print*>(loop->body[0].get()), nullptr);
  EXPECT_NE(dynamic_cast<const ast::Print*>(program[1].get()), nullptr);
}

TEST(ParserTests, RejectsMalformedForRangeHeadersAndBlocks) {
  for (const auto* source : {"for in range(3)\nend", "for i range(3)\nend", "for i in (3)\nend",
                             "for i in range()\nend", "for i in range(3\nend",
                             "for i in range(3) print i\nend", "for i in range(3)\nprint i"}) {
    EXPECT_THROW(parse(source), std::invalid_argument) << source;
  }
}

TEST(ParserTests, RejectsMalformedRepeatWhileWithBlockDiagnostics) {
  const std::pair<const char*, const char*> cases[] = {
    {"repeat", "Ожидался while после repeat"},
    {"repeat n < 3\nend", "Ожидался while после repeat"},
    {"repeat while\nend", "Ожидалось условие после repeat while"},
    {"repeat while flag end", "Ожидался перевод строки после условия repeat while"},
    {"repeat while flag\nprint 1", "Ожидался end для блока repeat while"},
    {"repeat while flag\nend print 1", "Ожидался перевод строки или конец файла"},
    {"when flag\nrepeat while other\notherwise\nend", "Неожиданный otherwise в блоке repeat while"}
  };
  for (const auto& [source, message] : cases) {
    try {
      parse(source);
      ADD_FAILURE() << source;
    } catch (const std::invalid_argument& error) {
      EXPECT_STREQ(error.what(), message) << source;
    }
  }
}

TEST(ParserTests, RejectsMalformedWhenWithBlockDiagnostics) {
  const std::pair<const char*, const char*> cases[] = {
    {"when\nend", "Ожидалось условие после when"},
    {"when flag print 1\nend", "Ожидался перевод строки после условия when"},
    {"when flag\notherwise print 1\nend", "Ожидался перевод строки после otherwise"},
    {"when flag\nprint 1 end", "Ожидался перевод строки после инструкции в блоке"},
    {"when flag\nprint 1", "Ожидался end для блока when"},
    {"when flag\notherwise\notherwise\nend", "Неожиданный otherwise в блоке when"},
    {"when flag\nwhen other\nend", "Ожидался end для блока when"},
    {"when flag\nend print 1", "Ожидался перевод строки или конец файла"},
    {"otherwise\n", "Неожиданный otherwise вне блока when"},
    {"end", "Неожиданный end вне блока"}
  };
  for (const auto& [source, message] : cases) {
    try {
      parse(source);
      ADD_FAILURE() << source;
    } catch (const std::invalid_argument& error) {
      EXPECT_STREQ(error.what(), message) << source;
    }
  }
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

  void visit(const ast::Call& node) override {
    lastExpression = &node;
    events.push_back("call:" + node.name);
    if (visitChildren) {
      for (std::size_t i = 0; i < node.arguments.size(); ++i) {
        node.arguments[i].getRoot()->accept(*this);
      }
    }
  }

  void visit(const ast::Binary& node) override {
    lastExpression = &node;
    events.push_back("binary");
    if (visitChildren) {
      node.leftOperand()->accept(*this);
      node.rightOperand()->accept(*this);
    }
  }

  void visit(const ast::Update& node) override {
    lastExpression = &node;
    events.push_back("update:" + node.variable);
  }

  void visit(const ast::Remember& node) override {
    lastStatement = &node;
    events.push_back("remember:" + node.variable);
    if (visitChildren) node.right.getRoot()->accept(*this);
  }

  void visit(const ast::UpdateStatement& node) override {
    lastStatement = &node;
    events.push_back("update statement");
    if (visitChildren) node.expression.getRoot()->accept(*this);
  }

  void visit(const ast::Print& node) override {
    lastStatement = &node;
    events.push_back("print");
    if (visitChildren) node.forOutput.getRoot()->accept(*this);
  }

  void visit(const ast::When& node) override {
    lastStatement = &node;
    events.push_back("when");
    if (visitChildren) {
      node.condition.getRoot()->accept(*this);
      for (std::size_t i = 0; i < node.thenBranch.size(); ++i) node.thenBranch[i]->accept(*this);
      for (std::size_t i = 0; i < node.otherwiseBranch.size(); ++i) node.otherwiseBranch[i]->accept(*this);
    }
  }

  void visit(const ast::RepeatWhile& node) override {
    lastStatement = &node;
    events.push_back("repeat while");
    if (visitChildren) {
      node.condition.getRoot()->accept(*this);
      for (std::size_t i = 0; i < node.body.size(); ++i) node.body[i]->accept(*this);
    }
  }

  void visit(const ast::For& node) override {
    lastStatement = &node;
    events.push_back("for:" + node.variable);
    if (visitChildren) {
      node.stop.getRoot()->accept(*this);
      for (std::size_t i = 0; i < node.body.size(); ++i) node.body[i]->accept(*this);
    }
  }

  void visit(const ast::Function& node) override {
    lastStatement = &node;
    events.push_back("function:" + node.name);
    if (visitChildren) {
      for (std::size_t i = 0; i < node.body.size(); ++i) node.body[i]->accept(*this);
      node.result.getRoot()->accept(*this);
    }
  }
};
}

TEST(ParserTests, VisitorTraversesFunctionResultAndCallArgumentsInOrder) {
  Array<ast::ExpressionTree> arguments;
  arguments.push_back(ast::ExpressionTree::makeLeaf(UniquePtr<ast::Expr>(new ast::Identifier("x"))));
  arguments.push_back(ast::ExpressionTree::makeLeaf(UniquePtr<ast::Expr>(new ast::NumberLiteral("2"))));
  Array<std::string> parameters;
  parameters.push_back("x");
  const ast::Function function("twice", std::move(parameters), ast::ExpressionTree::makeLeaf(
      UniquePtr<ast::Expr>(new ast::Call("multiply", std::move(arguments)))));

  RecordingVisitor visitor;
  visitor.visitChildren = true;
  const ast::Statement& statement = function;
  statement.accept(visitor);

  EXPECT_EQ(visitor.lastStatement, &function);
  ASSERT_EQ(visitor.events.size(), 4u);
  EXPECT_EQ(visitor.events[0], "function:twice");
  EXPECT_EQ(visitor.events[1], "call:multiply");
  EXPECT_EQ(visitor.events[2], "identifier:x");
  EXPECT_EQ(visitor.events[3], "number:2");
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
  const auto program = parse("remember x = 1\nprint x\nfor i in range(2)\nprint i\nend");
  ASSERT_EQ(program.size(), 3u);
  const ast::When when(ast::ExpressionTree::makeLeaf(UniquePtr<ast::Expr>(new ast::Identifier("flag"))),
                       {}, {});
  const ast::RepeatWhile repeat(ast::ExpressionTree::makeLeaf(UniquePtr<ast::Expr>(new ast::Identifier("flag"))),
                                {});
  const ast::Statement* statements[] = {program[0].get(), program[1].get(), &when, &repeat, program[2].get()};

  RecordingVisitor visitor;
  ast::StatementVisitor& statementVisitor = visitor;
  for (const auto* statement : statements) {
    statement->accept(statementVisitor);
    EXPECT_EQ(visitor.lastStatement, statement);
  }
  ASSERT_EQ(visitor.events.size(), 5u);
  EXPECT_EQ(visitor.events[0], "remember:x");
  EXPECT_EQ(visitor.events[1], "print");
  EXPECT_EQ(visitor.events[2], "when");
  EXPECT_EQ(visitor.events[3], "repeat while");
  EXPECT_EQ(visitor.events[4], "for:i");
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
