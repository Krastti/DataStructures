#include <gtest/gtest.h>

#include "../headers/interpreter.h"
#include "../headers/lexer.h"
#include "../headers/parser.h"

#include <iomanip>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace {
auto parseProgram(const std::string& source) {
  Parser parser;
  return parser.parse(Lexer::tokenize(source));
}

std::string runProgram(const std::string& source) {
  Interpreter interpreter;
  std::ostringstream output;
  interpreter.execute(parseProgram(source), output);
  return output.str();
}
}

TEST(InterpreterTests, ExecutesSpecificationExample) {
  EXPECT_EQ(runProgram("remember x = 10\nremember y = (x + 2) * 3\nprint y\n"
                       "print y >= 30\nremember x = y / 2\nprint x"),
            "36.0\ntrue\n18.0\n");
}

TEST(InterpreterTests, EvaluatesNestedExpressionsWithPrecedenceAndAssociativity) {
  EXPECT_EQ(runProgram("print 2 + 3 * 4\nprint (2 + 3) * 4\nprint 10 - 3 - 2\n"
                       "print 20 / 5 / 2\nprint (1 + 2) * (3 + 4)"),
            "14.0\n20.0\n5.0\n2.0\n21.0\n");
}

TEST(InterpreterTests, SupportsAllNumericOperators) {
  EXPECT_EQ(runProgram("print 7 + 2\nprint 7 - 2\nprint 7 * 2\nprint 7 / 2\n"
                       "print 2 < 3\nprint 3 <= 3\nprint 4 > 3\nprint 3 >= 3\n"
                       "print 2 == 2\nprint 2 != 3\nprint 3 < 2\nprint 2 == 3"),
            "9.0\n5.0\n14.0\n3.5\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\nfalse\nfalse\n");
}

TEST(InterpreterTests, StoresReplacesAndChangesVariableTypes) {
  EXPECT_EQ(runProgram("remember x = 1\nremember x = x + 1\nprint x\n"
                       "remember x = x > 0\nprint x\nremember x = \"text\"\nprint x"),
            "2.0\ntrue\ntext\n");
}

TEST(InterpreterTests, WhenEvaluatesOnlyTheSelectedBranch) {
  EXPECT_EQ(runProgram(
      "remember flag = 1 < 2\n"
      "when flag\n"
      "  remember flag = 1 > 2\n"
      "  print 1\n"
      "otherwise\n"
      "  print missing\n"
      "end\n"
      "when flag\n"
      "  print missing\n"
      "otherwise\n"
      "  print 2\n"
      "end\n"
      "when flag\n"
      "  print missing\n"
      "end"), "1.0\n2.0\n");
}

TEST(InterpreterTests, RepeatWhileSkipsTheBodyWhenInitiallyFalse) {
  EXPECT_EQ(runProgram("repeat while 1 > 2\nprint missing\nend\nprint 3"), "3.0\n");
}

TEST(InterpreterTests, NestedBlocksShareVariablesAndRepeatWhileChecksEachIteration) {
  EXPECT_EQ(runProgram(
      "remember n = 0\n"
      "when n == 0\n"
      "  repeat while n < 3\n"
      "    when n == 1\n"
      "      remember label = \"середина\"\n"
      "    otherwise\n"
      "      print n\n"
      "    end\n"
      "    remember n = n + 1\n"
      "  end\n"
      "end\n"
      "print n\n"
      "print label"), "0.0\n2.0\n3.0\nсередина\n");
}

TEST(InterpreterTests, SkippedBlocksDoNotCreateVariables) {
  EXPECT_THROW(runProgram("when 1 > 2\nremember hidden = 1\nend\nprint hidden"),
               std::invalid_argument);
}

TEST(InterpreterTests, RequiresBooleanConditionsInWhenAndRepeatWhile) {
  for (const auto* instruction : {"when", "repeat while"}) {
    for (const auto* condition : {"1", "\"text\""}) {
      const std::string source = std::string(instruction) + " " + condition + "\nprint 1\nend";
      Interpreter interpreter;
      std::ostringstream output;
      try {
        interpreter.execute(parseProgram(source), output);
        ADD_FAILURE() << source;
      } catch (const std::invalid_argument& error) {
        EXPECT_EQ(error.what(), std::string("Ошибка выполнения: условие ") + instruction +
                                " должно быть логическим");
      }
      EXPECT_TRUE(output.str().empty());
    }
  }
}

TEST(InterpreterTests, RepeatWhileStopsOnAnErrorInANestedBranch) {
  Interpreter interpreter;
  std::ostringstream output;
  EXPECT_THROW(interpreter.execute(parseProgram(
      "remember n = 0\n"
      "repeat while n < 2\n"
      "  print n\n"
      "  remember n = n + 1\n"
      "  when n == 1\n"
      "    print missing\n"
      "  end\n"
      "end\n"
      "print 9"), output), std::invalid_argument);
  EXPECT_EQ(output.str(), "0.0\n");
}

TEST(InterpreterTests, PrintsStringsWithoutQuotesAndPreservesContents) {
  EXPECT_EQ(runProgram("remember text = \"hello world\"\r\nprint text\nprint \"\"\n"
                       "print \"a\tb\\c\"\nprint \"Привет\""),
            "hello world\n\na\tb\\c\nПривет\n");
}

TEST(InterpreterTests, EmptyProgramProducesNoOutput) {
  EXPECT_EQ(runProgram("\n\r\n"), "");
}

TEST(InterpreterTests, ReportsUnknownAndCaseSensitiveVariableNames) {
  EXPECT_THROW(runProgram("print unknown"), std::invalid_argument);
  EXPECT_THROW(runProgram("remember x = 1\nprint X"), std::invalid_argument);
}

TEST(InterpreterTests, RejectsNonNumericOperandsWithoutImplicitConversions) {
  for (const auto* source : {"print \"a\" + 1", "print 1 + \"a\"", "print \"a\" == \"a\"",
      "print (1 < 2) + 1", "print 1 == (1 < 2)", "print 1 < 2 < 3"}) {
    EXPECT_THROW(runProgram(source), std::invalid_argument) << source;
  }
}

TEST(InterpreterTests, RejectsDivisionByPositiveAndNegativeZero) {
  EXPECT_THROW(runProgram("print 1 / 0"), std::invalid_argument);
  EXPECT_THROW(runProgram("print 1 / (0 / (0 - 1))"), std::invalid_argument);
}

TEST(InterpreterTests, PreservesEarlierOutputAndStopsOnFirstError) {
  Interpreter interpreter;
  std::ostringstream output;
  EXPECT_THROW(interpreter.execute(parseProgram("print 1\nprint missing\nprint 2"), output),
               std::invalid_argument);
  EXPECT_EQ(output.str(), "1.0\n");
}

TEST(InterpreterTests, EvaluatesLeftOperandBeforeRightOperand) {
  try {
    runProgram("print leftMissing + rightMissing");
    EXPECT_TRUE(false);
  } catch (const std::invalid_argument& error) {
    EXPECT_NE(std::string(error.what()).find("leftMissing"), std::string::npos);
  }
}

TEST(InterpreterTests, RepeatedExecutionsStartWithEmptyVariables) {
  Interpreter interpreter;
  std::ostringstream first;
  interpreter.execute(parseProgram("remember x = 1\nprint x"), first);
  EXPECT_EQ(first.str(), "1.0\n");
  std::ostringstream second;
  EXPECT_THROW(interpreter.execute(parseProgram("print x"), second), std::invalid_argument);
  EXPECT_EQ(second.str(), "");
  interpreter.execute(parseProgram("print 2"), second);
  EXPECT_EQ(second.str(), "2.0\n");
  EXPECT_EQ(first.str(), "1.0\n");
}

TEST(InterpreterTests, CanReuseInterpreterAfterExecutionError) {
  Interpreter interpreter;
  std::ostringstream output;
  EXPECT_THROW(interpreter.execute(parseProgram("remember x = 1\nprint 1 / 0"), output),
               std::invalid_argument);
  EXPECT_THROW(interpreter.execute(parseProgram("print x"), output), std::invalid_argument);
  interpreter.execute(parseProgram("remember y = 3\nprint y"), output);
  EXPECT_EQ(output.str(), "3.0\n");
}

TEST(InterpreterTests, StoresManyVariablesInTree) {
  std::string source;
  std::string expected;
  for (int i = 0; i < 100; ++i) {
    source += "remember var" + std::to_string(i) + " = " + std::to_string(i) + "\n";
  }
  for (int i = 0; i < 100; ++i) {
    source += "print var" + std::to_string(i) + "\n";
    expected += std::to_string(i) + ".0\n";
  }
  EXPECT_EQ(runProgram(source), expected);
}

TEST(InterpreterTests, OutputIgnoresLocaleAndFormattingFlags) {
  struct CommaPunctuation : std::numpunct<char> {
    char do_decimal_point() const override { return ','; }
  };
  Interpreter interpreter;
  std::ostringstream output;
  output.imbue(std::locale(std::locale::classic(), new CommaPunctuation));
  output << std::fixed << std::setprecision(1) << std::setw(10);
  const auto flags = output.flags();
  interpreter.execute(parseProgram("print 1 / 8\nprint 2\nprint 1 < 2"), output);
  EXPECT_EQ(output.str(), "0.125\n2.0\ntrue\n");
  EXPECT_EQ(output.flags(), flags);
  EXPECT_EQ(output.precision(), 1);
  EXPECT_EQ(output.width(), 10);
}

TEST(InterpreterTests, DetectsOutputFailureAndReleasesOutputReference) {
  Interpreter interpreter;
  std::ostringstream broken;
  broken.setstate(std::ios::badbit);
  const auto program = parseProgram("print 1");
  EXPECT_THROW(interpreter.execute(program, broken), std::runtime_error);
  const ast::Print print(ast::ExpressionTree::makeLeaf(UniquePtr<ast::Expr>(new ast::NumberLiteral("2"))));
  EXPECT_THROW(print.accept(interpreter), std::logic_error);
  std::ostringstream working;
  interpreter.execute(program, working);
  EXPECT_EQ(working.str(), "1.0\n");
}

TEST(InterpreterTests, RejectsInvalidManuallyBuiltAst) {
  Interpreter interpreter;
  std::ostringstream output;
  Array<UniquePtr<ast::Statement>> missing;
  missing.push_back(UniquePtr<ast::Statement>{});
  EXPECT_THROW(interpreter.execute(missing, output), std::invalid_argument);
  Array<UniquePtr<ast::Statement>> unsupported;
  auto unsupportedTree = ast::ExpressionTree::makeBinary(TokenType::Equals,
      ast::ExpressionTree::makeLeaf(UniquePtr<ast::Expr>(new ast::NumberLiteral("1"))),
      ast::ExpressionTree::makeLeaf(UniquePtr<ast::Expr>(new ast::NumberLiteral("2"))));
  unsupported.push_back(UniquePtr<ast::Statement>(new ast::Print(std::move(unsupportedTree))));
  EXPECT_THROW(interpreter.execute(unsupported, output), std::invalid_argument);
  const ast::NumberLiteral invalidNumber("12bad");
  EXPECT_THROW(invalidNumber.accept(interpreter), std::invalid_argument);
  const ast::StringLiteral invalidString("text");
  EXPECT_THROW(invalidString.accept(interpreter), std::invalid_argument);
}

TEST(InterpreterTests, DoesNotReusePreviousValueWhenExpressionProducesNoResult) {
  struct EmptyExpression : ast::Expr {
    void accept(ast::ExprVisitor&) const override {}
  };
  Array<UniquePtr<ast::Statement>> program;
  auto first = ast::ExpressionTree::makeLeaf(UniquePtr<ast::Expr>(new ast::NumberLiteral("1")));
  auto second = ast::ExpressionTree::makeLeaf(UniquePtr<ast::Expr>(new EmptyExpression));
  program.push_back(UniquePtr<ast::Statement>(new ast::Print(std::move(first))));
  program.push_back(UniquePtr<ast::Statement>(new ast::Print(std::move(second))));
  Interpreter interpreter;
  std::ostringstream output;
  EXPECT_THROW(interpreter.execute(program, output), std::logic_error);
  EXPECT_EQ(output.str(), "1.0\n");
}

TEST(InterpreterTests, FormatsSmallFractionsLargeNumbersAndNegativeZero) {
  EXPECT_EQ(runProgram("print 1 / 10\nprint 100000000000000000000\nprint 0 / (0 - 1)"),
            "0.1\n1.0e+20\n-0.0\n");
  Interpreter interpreter;
  const ast::NumberLiteral overflow(std::string(400, '9'));
  EXPECT_THROW(overflow.accept(interpreter), std::invalid_argument);
}
