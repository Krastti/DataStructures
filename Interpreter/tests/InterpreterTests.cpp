#include <gtest/gtest.h>

#include "runtime/environment.h"
#include "runtime/interpreter.h"
#include "lexer/lexer.h"
#include "parser/parser.h"

#include <iomanip>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>

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

TEST(EnvironmentTests, IsolatesParametersAndLocalNamesAcrossFunctionCalls) {
  minilang::Environment environment;
  environment.assign("x", 9.0);
  environment.assign("global", 8.0);

  Array<std::string> parameters;
  parameters.push_back("x");
  Array<minilang::Environment::Value> arguments;
  arguments.push_back(2.0);

  const auto result = environment.withFunctionScope(parameters, arguments, [&] {
    EXPECT_EQ(std::get<double>(environment.lookup("x")), 2.0);
    EXPECT_EQ(std::get<double>(environment.lookup("global")), 8.0);
    environment.assign("x", 3.0);
    environment.assign("local", true);

    const auto nested = environment.withFunctionScope({}, {}, [&] {
      EXPECT_EQ(std::get<double>(environment.lookup("x")), 9.0);
      EXPECT_THROW(static_cast<void>(environment.lookup("local")), std::invalid_argument);
      environment.assign("x", 4.0);
      return environment.lookup("x");
    });
    EXPECT_EQ(std::get<double>(nested), 4.0);
    EXPECT_TRUE(std::get<bool>(environment.lookup("local")));
    return environment.lookup("x");
  });

  EXPECT_EQ(std::get<double>(result), 3.0);
  EXPECT_EQ(std::get<double>(environment.lookup("x")), 9.0);
  EXPECT_THROW(static_cast<void>(environment.lookup("local")), std::invalid_argument);
}

TEST(EnvironmentTests, FunctionSeesOnlyLoopVariablesCreatedInItsOwnFrame) {
  minilang::Environment environment;
  environment.withLoopVariable("callerCounter", [&](auto& callerCounter) {
    callerCounter = 4.0;
    const auto result = environment.withFunctionScope({}, {}, [&] {
      EXPECT_THROW(static_cast<void>(environment.lookup("callerCounter")), std::invalid_argument);
      environment.assign("callerCounter", 5.0);
      environment.withLoopVariable("callerCounter", [&](auto& functionCounter) {
        environment.assign("callerCounter", 6.0);
        EXPECT_EQ(std::get<double>(functionCounter), 6.0);
        EXPECT_EQ(std::get<double>(environment.lookup("callerCounter")), 6.0);
      });
      return environment.lookup("callerCounter");
    });
    EXPECT_EQ(std::get<double>(result), 5.0);
    EXPECT_EQ(std::get<double>(callerCounter), 4.0);
  });
}

TEST(EnvironmentTests, RestoresFunctionAndLoopScopesAfterException) {
  minilang::Environment environment;
  environment.withLoopVariable("i", [&](auto& counter) {
    counter = 7.0;
    EXPECT_THROW(environment.withFunctionScope({}, {}, [&]() -> minilang::Environment::Value {
      environment.assign("temporary", true);
      environment.withLoopVariable("j", [](auto&) {
        throw std::runtime_error("test");
      });
      return false;
    }), std::runtime_error);
    EXPECT_EQ(std::get<double>(environment.lookup("i")), 7.0);
    EXPECT_THROW(static_cast<void>(environment.lookup("j")), std::invalid_argument);
    EXPECT_THROW(static_cast<void>(environment.lookup("temporary")), std::invalid_argument);
  });
}

TEST(EnvironmentTests, RejectsMismatchedArgumentsBeforeEnteringFunctionScope) {
  minilang::Environment environment;
  Array<std::string> parameters;
  parameters.push_back("x");

  EXPECT_THROW(environment.withFunctionScope(parameters, {}, [] {
    ADD_FAILURE() << "Выполнено тело функции с неверным числом аргументов";
    return false;
  }), std::invalid_argument);
}

TEST(EnvironmentTests, LimitsFunctionDepthAndUnwindsAllFrames) {
  minilang::Environment environment;
  std::size_t entered = 0;
  const auto recurse = [&](const auto& self) -> minilang::Environment::Value {
    return environment.withFunctionScope({}, {}, [&] {
      ++entered;
      environment.assign("temporary", true);
      return self(self);
    });
  };

  EXPECT_THROW(recurse(recurse), std::invalid_argument);
  EXPECT_EQ(entered, 128u);
  EXPECT_THROW(static_cast<void>(environment.lookup("temporary")), std::invalid_argument);
  EXPECT_TRUE(std::get<bool>(environment.withFunctionScope({}, {}, [] { return true; })));
}

TEST(InterpreterTests, EvaluatesFunctionCallsInExpressionsAndPreservesValueTypes) {
  EXPECT_EQ(runProgram(
      "function subtract(a, b) -> a - b end\n"
      "function identity(value) -> value end\n"
      "function message() -> \"ready\" end\n"
      "print subtract(identity(7), 3) * 2\n"
      "print identity(message())\n"
      "print identity(2 < 3)"), "8.0\nready\ntrue\n");
}

TEST(InterpreterTests, ResolvesFunctionBodiesOnCallWithIsolatedParameters) {
  EXPECT_EQ(runProgram(
      "function outer(x) -> read() + x end\n"
      "function read() -> x end\n"
      "remember x = 9\n"
      "print outer(2)\n"
      "print x"), "11.0\n9.0\n");
}

TEST(InterpreterTests, EvaluatesRecursiveFactorialWithLocalVariables) {
  EXPECT_EQ(runProgram(
      "function factorial(n) ->\n"
      "  remember result = 1\n"
      "  when n > 1\n"
      "    remember result = n * factorial(n - 1)\n"
      "  end\n"
      "  return result\n"
      "end\n"
      "print factorial(0)\n"
      "print factorial(1)\n"
      "print factorial(5)"), "1.0\n1.0\n120.0\n");
}

TEST(InterpreterTests, KeepsMultilineFunctionAssignmentsLocal) {
  EXPECT_EQ(runProgram(
      "remember result = 99\n"
      "function increment(x) ->\n"
      "  remember result = x + 1\n"
      "  return result\n"
      "end\n"
      "for i in range(2)\n"
      "  print increment(i)\n"
      "end\n"
      "print result"), "1.0\n2.0\n99.0\n");
}

TEST(InterpreterTests, AllowsMultilineFunctionWithOnlyAReturnLine) {
  EXPECT_EQ(runProgram("function f() ->\nreturn 2\nend\nprint f()"), "2.0\n");
}

TEST(InterpreterTests, SkipsFunctionReturnAfterBodyErrorAndCanRunAgain) {
  Interpreter interpreter;
  std::ostringstream output;
  const auto program = parseProgram(
      "function f(x) ->\n"
      "  print x\n"
      "  remember local = 5\n"
      "  print 1 / 0\n"
      "  return local\n"
      "end\n"
      "print f(7)");
  EXPECT_THROW(interpreter.execute(program, output), std::invalid_argument);
  interpreter.execute(parseProgram("remember local = 2\nprint local"), output);
  EXPECT_EQ(output.str(), "7.0\n2.0\n");
}

TEST(InterpreterTests, EvaluatesCallArgumentsInCallerLoopScope) {
  EXPECT_EQ(runProgram(
      "function identity(x) -> x end\n"
      "for i in range(2)\n"
      "  print identity(i)\n"
      "end"), "0.0\n1.0\n");
  EXPECT_THROW(runProgram(
      "function read() -> i end\n"
      "for i in range(1)\n"
      "  print read()\n"
      "end"), std::invalid_argument);
}

TEST(InterpreterTests, ReportsFunctionErrorsAndEvaluatesArgumentsLeftToRight) {
  const struct {
    const char* source;
    const char* expected;
  } cases[] = {
    {"print later(1 / 0)\nfunction later(x) -> x end", "неизвестная функция: later"},
    {"function id(x) -> x end\nprint id(1 / 0, 2)", "неверное число аргументов: id"},
    {"function id(x) -> x end\nprint id()", "неверное число аргументов: id"},
    {"function f() -> 1 end\nfunction f() -> 2 end", "повторное объявление функции: f"},
    {"function f(x, y) -> x end\nprint f(leftMissing, rightMissing)", "неизвестная переменная: leftMissing"},
    {"function f() -> f() end\nprint f()", "превышена глубина вызовов функций"}
  };
  for (const auto& [source, expected] : cases) {
    try {
      runProgram(source);
      ADD_FAILURE() << source;
    } catch (const std::invalid_argument& error) {
      EXPECT_EQ(error.what(), std::string("Ошибка выполнения: ") + expected);
    }
  }
}

TEST(InterpreterTests, ClearsFunctionDeclarationsAfterSuccessAndFailure) {
  Interpreter interpreter;
  std::ostringstream output;
  interpreter.execute(parseProgram("function f() -> 1 end"), output);
  EXPECT_THROW(interpreter.execute(parseProgram("print f()"), output), std::invalid_argument);

  EXPECT_THROW(interpreter.execute(parseProgram(
      "function f(x) -> x / 0 end\nprint f(2)"), output), std::invalid_argument);
  EXPECT_THROW(interpreter.execute(parseProgram("print f()"), output), std::invalid_argument);
  interpreter.execute(parseProgram("function f(x) -> x end\nprint f(7)"), output);
  EXPECT_EQ(output.str(), "7.0\n");
}

TEST(InterpreterTests, EvaluatesNestedExpressionsWithPrecedenceAndAssociativity) {
  EXPECT_EQ(runProgram("print 2 + 3 * 4\nprint (2 + 3) * 4\nprint 10 - 3 - 2\n"
                       "print 20 / 5 / 2\nprint (1 + 2) * (3 + 4)"),
            "14.0\n20.0\n5.0\n2.0\n21.0\n");
}

TEST(InterpreterTests, EvaluatesDecimalLiteralsInExpressions) {
  EXPECT_EQ(runProgram("print 0.25 + 2.5 * 2\nprint 3.0 - 4.5\nprint 1.25 < 1.5"),
            "5.25\n-1.5\ntrue\n");
}

TEST(InterpreterTests, AppliesPrefixAndPostfixUpdatesAndReturnsTheCorrectValue) {
  EXPECT_EQ(runProgram(
      "remember x = 2.5\n"
      "print ++x\n"
      "print x++ * 2\n"
      "print --x\n"
      "print x--\n"
      "print x\n"
      "--x\n"
      "x++\n"
      "print x"), "3.5\n7.0\n3.5\n3.5\n2.5\n2.5\n");
}

TEST(InterpreterTests, CompoundAssignmentReadsOldValueAndEvaluatesItsRightSideOnce) {
  EXPECT_EQ(runProgram(
      "remember x = 2.5\n"
      "remember y = 1\n"
      "x += y++ + 0.25\n"
      "print y\n"
      "x -= x++\n"
      "print x"), "2.0\n0.0\n");
}

TEST(InterpreterTests, UsesExistingFunctionScopeRulesForNumericUpdates) {
  EXPECT_EQ(runProgram(
      "remember x = 4.5\n"
      "function update() ->\n"
      "  x += 0.5\n"
      "  return --x\n"
      "end\n"
      "print update()\n"
      "print x"), "4.0\n4.5\n");
}

TEST(InterpreterTests, RejectsMissingOrNonNumericVariablesAndOperandsForUpdates) {
  for (const char* source : {"++x", "x += 1", "remember x = \"text\"\nx--",
                             "remember x = 1 < 2\nx -= 1", "remember x = 1\nx += \"text\""}) {
    EXPECT_THROW(runProgram(source), std::invalid_argument) << source;
  }
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

TEST(InterpreterTests, ForWithNonPositiveRangeSkipsBodyAndPreservesLoopVariable) {
  EXPECT_EQ(runProgram("remember i = 7\n"
                       "for i in range(0)\nprint missing\nend\n"
                       "for i in range(0 - 2)\nprint missing\nend\n"
                       "print i"), "7.0\n");
  EXPECT_THROW(runProgram("for i in range(0)\nend\nprint i"), std::invalid_argument);
}

TEST(InterpreterTests, ForRejectsInvalidRangeStopsBeforeExecutingTheBody) {
  for (const auto* stop : {"\"text\"", "1 < 2", "3 / 2", "9007199254740994"}) {
    const std::string source = std::string("for i in range(") + stop + ")\nprint 1\nend";
    Interpreter interpreter;
    std::ostringstream output;
    EXPECT_THROW(interpreter.execute(parseProgram(source), output), std::invalid_argument) << source;
    EXPECT_TRUE(output.str().empty()) << source;
  }
}

TEST(InterpreterTests, ForEvaluatesRangeOnceAndSharesLoopVariableWithNestedWhen) {
  EXPECT_EQ(runProgram(
      "remember limit = 3\n"
      "for i in range(limit)\n"
      "  when i == 1\n"
      "    print \"середина\"\n"
      "  otherwise\n"
      "    print i\n"
      "  end\n"
      "  remember limit = 0\n"
      "end"), "0.0\nсередина\n2.0\n");
}

TEST(InterpreterTests, NestedForWithSameNameRestoresOuterCounter) {
  EXPECT_EQ(runProgram(
      "for i in range(2)\n"
      "  for i in range(i + 2)\n"
      "    print i\n"
      "  end\n"
      "  print i\n"
      "end"), "0.0\n1.0\n0.0\n0.0\n1.0\n2.0\n1.0\n");
}

TEST(InterpreterTests, ForRestoresPreviousValueAfterShadowing) {
  EXPECT_EQ(runProgram(
      "remember i = \"до цикла\"\n"
      "for i in range(2)\n"
      "  print i\n"
      "  remember i = i + 10\n"
      "  print i\n"
      "end\n"
      "print i"), "0.0\n10.0\n1.0\n11.0\nдо цикла\n");
}

TEST(InterpreterTests, RememberInNestedForUpdatesTheVisibleCounterAndCreatesGlobalNames) {
  EXPECT_EQ(runProgram(
      "for i in range(1)\n"
      "  for j in range(1)\n"
      "    remember i = i + 5\n"
      "    remember total = i + j\n"
      "  end\n"
      "  print i\n"
      "end\n"
      "print total"), "5.0\n5.0\n");
}

TEST(InterpreterTests, ForCounterWithoutPreviousValueIsUnknownAfterEnd) {
  Interpreter interpreter;
  std::ostringstream output;
  EXPECT_THROW(interpreter.execute(parseProgram(
      "for i in range(1)\nprint i\nend\nprint i"), output), std::invalid_argument);
  EXPECT_EQ(output.str(), "0.0\n");
}

TEST(InterpreterTests, ClearsNestedForScopesAfterAnError) {
  Interpreter interpreter;
  std::ostringstream failed;
  EXPECT_THROW(interpreter.execute(parseProgram(
      "for i in range(2)\n"
      "  for j in range(2)\n"
      "    print i + j\n"
      "    print missing\n"
      "  end\n"
      "end"), failed), std::invalid_argument);
  EXPECT_EQ(failed.str(), "0.0\n");

  std::ostringstream output;
  interpreter.execute(parseProgram(
      "remember i = 10\nremember j = 20\n"
      "for i in range(1)\nprint i + j\nend\nprint i\nprint j"), output);
  EXPECT_EQ(output.str(), "20.0\n10.0\n20.0\n");
}

TEST(InterpreterTests, ReportsObserverEventsInOrderAcrossForScope) {
  struct RecordingObserver : Interpreter::Observer {
    std::ostringstream events;

    void enterStatement(const ast::Statement&) override { events << "statement\n"; }
    void enterExpression(const ast::Expr&) override { events << "expression\n"; }
    void leaveCall(const Interpreter::Value* value) override {
      if (value) events << "value " << std::get<double>(*value) << '\n';
      else events << "end statement\n";
    }
  };

  Interpreter interpreter;
  RecordingObserver observer;
  std::ostringstream output;
  interpreter.execute(parseProgram(
      "remember i = 1\n"
      "for i in range(i)\n"
      "  print i + 2\n"
      "end\n"
      "print i"), output, &observer);

  EXPECT_EQ(output.str(), "2.0\n1.0\n");
  EXPECT_EQ(observer.events.str(),
            "statement\nexpression\nvalue 1\nend statement\n"
            "statement\nexpression\nvalue 1\n"
            "statement\nexpression\nexpression\nvalue 0\nexpression\nvalue 2\nvalue 2\nend statement\n"
            "end statement\n"
            "statement\nexpression\nvalue 1\nend statement\n");
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
