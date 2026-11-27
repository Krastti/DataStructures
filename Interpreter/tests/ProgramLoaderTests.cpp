#include <gtest/gtest.h>

#include "../../DataStructures/tests/AllocationFailure.h"
#include "loader/programLoader.h"
#include "runtime/interpreter.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

namespace {
namespace fs = std::filesystem;
using minilang::LoadContext;
using minilang::PreparedProgram;
using minilang::ProgramLoader;

class ProgramLoaderTests : public ::testing::Test {
protected:
  fs::path directory;

  void SetUp() override {
    static std::atomic<unsigned int> sequence{0};
    const auto time = std::chrono::steady_clock::now().time_since_epoch().count();
    directory = fs::temp_directory_path() /
      ("minilang-loader-" + std::to_string(time) + "-" + std::to_string(sequence++));
    ASSERT_TRUE(fs::create_directory(directory));
  }

  void TearDown() override {
    std::error_code error;
    fs::remove_all(directory, error);
  }

  void write(const fs::path& name, const std::string& source) const {
    const auto path = directory / name;
    fs::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary);
    output << source;
    output.close();
    ASSERT_TRUE(output) << path;
  }

  PreparedProgram load(const std::string& source) const {
    return ProgramLoader::load(source, LoadContext{.baseDirectory = directory});
  }

  std::string execute(const PreparedProgram& program) const {
    std::ostringstream output;
    Interpreter{}.execute(program.statements, output);
    return output.str();
  }

  std::string errorFor(const std::string& source) const {
    try {
      auto program = load(source);
      ADD_FAILURE() << "Ожидалась ошибка загрузки";
    } catch (const std::runtime_error& error) {
      return error.what();
    }
    return {};
  }
};

TEST_F(ProgramLoaderTests, LoadsOrdinaryAndEmptyProgramsWithoutContext) {
  EXPECT_EQ(ProgramLoader::load("").statements.size(), 0u);
  const auto program = ProgramLoader::load("remember x = 2\nprint x * 3");
  EXPECT_EQ(execute(program), "6.0\n");
  EXPECT_EQ(program.tokens[0].type, TokenType::Remember);
  EXPECT_EQ(program.tokens[program.tokens.size() - 1].type, TokenType::EndOfFile);
}

TEST_F(ProgramLoaderTests, LoadsNestedLibrariesRelativeToTheirFiles) {
  write("lib/constants.mini", "function factor() -> 2 end");
  write("lib/math.mini", "using \"constants.mini\"\r\nfunction square(x) -> x * x * factor() end");
  const auto program = load("using \"lib/math.mini\"\nprint square(5)");
  ASSERT_EQ(program.statements.size(), 3u);
  const auto* first = dynamic_cast<const ast::Function*>(program.statements[0].get());
  ASSERT_NE(first, nullptr);
  EXPECT_EQ(first->name, "factor");
  EXPECT_EQ(execute(program), "50.0\n");
  EXPECT_EQ(program.tokens[0].type, TokenType::Using);
  EXPECT_EQ(program.tokens[1].lexeme, "\"lib/math.mini\"");
}

TEST_F(ProgramLoaderTests, UsesSuppliedMainTextAndSourceDirectory) {
  write("main.mini", "print 0");
  write("math.mini", "function square(x) -> x * x end");
  const auto program = ProgramLoader::load("using \"math.mini\"\nprint square(5)",
                                          LoadContext{.sourcePath = directory / "main.mini"});
  EXPECT_EQ(execute(program), "25.0\n");
}

TEST_F(ProgramLoaderTests, AcceptsAbsoluteImportsWithoutContext) {
  write("math.mini", "function square(x) -> x * x end");
  const auto program = ProgramLoader::load("using \"" + (directory / "math.mini").string() +
                                          "\"\nprint square(4)");
  EXPECT_EQ(execute(program), "16.0\n");
}

TEST_F(ProgramLoaderTests, RequiresContextForRelativeImports) {
  EXPECT_THROW(ProgramLoader::load("using \"math.mini\""), std::runtime_error);
  EXPECT_THROW(ProgramLoader::load("", LoadContext{directory / "main.mini", directory}),
               std::runtime_error);
}

TEST_F(ProgramLoaderTests, PreservesDirectiveOrderAndSkipsSharedDependencies) {
  write("common.mini", "function common() -> 1 end");
  write("a.mini", "using \"common.mini\"\nfunction a() -> common() end");
  write("b.mini", "using \"common.mini\"\nfunction b() -> common() + 1 end");
  const auto program = load("using \"a.mini\"\nusing \"b.mini\"\nprint a() + b()");
  ASSERT_EQ(program.statements.size(), 4u);
  const char* names[] = {"common", "a", "b"};
  for (std::size_t i = 0; i < 3; ++i) {
    const auto* function = dynamic_cast<const ast::Function*>(program.statements[i].get());
    ASSERT_NE(function, nullptr);
    EXPECT_EQ(function->name, names[i]);
  }
  EXPECT_EQ(execute(program), "3.0\n");
}

TEST_F(ProgramLoaderTests, CanonicalizesAlternatePathsAndSymbolicLinks) {
  write("lib/my math.mini", "function square(x) -> x * x end");
  std::error_code error;
  fs::create_symlink(directory / "lib/my math.mini", directory / "alias.mini", error);
  if (error) GTEST_SKIP() << error.message();
  const auto program = load("using \"lib/my math.mini\"\nusing \"lib/../lib/./my math.mini\"\n"
                            "using \"alias.mini\"\nprint square(3)");
  EXPECT_EQ(program.statements.size(), 2u);
  EXPECT_EQ(execute(program), "9.0\n");
}

TEST_F(ProgramLoaderTests, ReportsCompleteCycleAndSelfImport) {
  write("a.mini", "using \"b.mini\"");
  write("b.mini", "using \"a.mini\"");
  const auto message = errorFor("using \"a.mini\"");
  const auto a = fs::canonical(directory / "a.mini").string();
  const auto b = fs::canonical(directory / "b.mini").string();
  EXPECT_NE(message.find("Циклическая зависимость"), std::string::npos);
  EXPECT_NE(message.find(a + " → " + b + " → " + a), std::string::npos);
  write("a.mini", "using \"./a.mini\"");
  EXPECT_NE(errorFor("using \"a.mini\"").find(a + " → " + a), std::string::npos);
}

TEST_F(ProgramLoaderTests, DetectsImportsBackToMainFileBeforeLibraryParsing) {
  write("main.mini", "print 0");
  write("a.mini", "using \"main.mini\"");
  try {
    ProgramLoader::load("using \"a.mini\"", LoadContext{.sourcePath = directory / "main.mini"});
    FAIL() << "Ожидался цикл через основной файл";
  } catch (const std::runtime_error& error) {
    EXPECT_NE(std::string(error.what()).find("Циклическая зависимость"), std::string::npos);
  }
}

TEST_F(ProgramLoaderTests, ReportsMissingFilesAndDirectoriesWithImportChain) {
  write("a.mini", "using \"missing.mini\"");
  const auto message = errorFor("using \"a.mini\"");
  EXPECT_NE(message.find("Ошибка загрузки"), std::string::npos);
  EXPECT_NE(message.find("a.mini → " + (directory / "missing.mini").string()), std::string::npos);
  fs::create_directory(directory / "folder");
  EXPECT_NE(errorFor("using \"folder\"").find("Ожидался обычный файл"), std::string::npos);
}

TEST_F(ProgramLoaderTests, ReportsUnreadableLibrary) {
  write("private.mini", "function f() -> 1 end");
  const auto path = directory / "private.mini";
  fs::permissions(path, fs::perms::none);
  std::ifstream probe(path);
  if (probe) GTEST_SKIP() << "Текущий пользователь может читать файл без прав доступа";
  const auto message = errorFor("using \"private.mini\"");
  EXPECT_NE(message.find("Не удалось открыть файл"), std::string::npos);
  EXPECT_NE(message.find("private.mini"), std::string::npos);
}

TEST_F(ProgramLoaderTests, RejectsNullBytesInPaths) {
  std::string source = "using \"math";
  source.push_back('\0');
  source += ".mini\"";
  EXPECT_NE(errorFor(source).find("нулевой байт"), std::string::npos);
}

TEST_F(ProgramLoaderTests, ReportsLexicalAndSyntaxErrorsInLibraries) {
  write("a.mini", "using \"b.mini\"");
  write("b.mini", "function f() -> 1. end");
  auto message = errorFor("using \"a.mini\"");
  EXPECT_NE(message.find("Ошибка лексического анализа"), std::string::npos);
  EXPECT_NE(message.find((directory / "b.mini").string()), std::string::npos);
  EXPECT_NE(message.find("a.mini → " + (directory / "b.mini").string()), std::string::npos);
  write("b.mini", "print 1");
  message = errorFor("using \"a.mini\"");
  EXPECT_NE(message.find("Ошибка синтаксического анализа"), std::string::npos);
  EXPECT_NE(message.find("b.mini"), std::string::npos);
}

TEST_F(ProgramLoaderTests, KeepsDuplicateDeclarationsForRuntimeValidation) {
  write("a.mini", "function f() -> 1 end");
  write("b.mini", "function f() -> 2 end");
  const auto program = load("using \"a.mini\"\nusing \"b.mini\"\nprint f()");
  EXPECT_THROW(execute(program), std::invalid_argument);
}

TEST_F(ProgramLoaderTests, ReloadsFilesAfterSuccessAndFailure) {
  write("a.mini", "function f() -> 1 end");
  auto first = load("using \"a.mini\"\nprint f()");
  write("a.mini", "function f() -> 2 end");
  EXPECT_EQ(execute(first), "1.0\n");
  EXPECT_EQ(execute(load("using \"a.mini\"\nprint f()")), "2.0\n");
  write("a.mini", "function f() -> end");
  EXPECT_THROW(load("using \"a.mini\""), std::runtime_error);
  write("a.mini", "function f() -> 3 end");
  EXPECT_EQ(execute(load("using \"a.mini\"\nprint f()")), "3.0\n");
}

TEST_F(ProgramLoaderTests, ReleasesPartialProgramAndLoadedFilesOnFailure) {
  write("good.mini", "function square(x) -> x * x end");
  write("bad.mini", "function f() -> 1 end\nfunction broken() -> end");
  const std::string source = "using \"good.mini\"\nusing \"bad.mini\"\nprint 1";
  std::ptrdiff_t balance = 0;
  bool failed = false;
  {
    test::TrackAllocations guard(balance);
    try { auto program = load(source); }
    catch (const std::runtime_error&) { failed = true; }
  }
  EXPECT_TRUE(failed);
  EXPECT_EQ(balance, 0);
}

TEST_F(ProgramLoaderTests, ReleasesPreparedProgramAfterSuccess) {
  write("a.mini", "function f() -> 1 end");
  const std::string source = "using \"a.mini\"\nprint f()";
  std::ptrdiff_t balance = 0;
  {
    test::TrackAllocations guard(balance);
    auto program = load(source);
  }
  EXPECT_EQ(balance, 0);
}

TEST_F(ProgramLoaderTests, LoadsEmptyLibrariesAndLongDependencyChains) {
  write("empty.mini", "\n\r\n");
  EXPECT_EQ(load("using \"empty.mini\"").statements.size(), 0u);
  for (int i = 0; i < 150; ++i) {
    const auto source = i == 149 ? "function f() -> 7 end" :
      "using \"" + std::to_string(i + 1) + ".mini\"";
    write(std::to_string(i) + ".mini", source);
  }
  EXPECT_EQ(execute(load("using \"0.mini\"\nprint f()")), "7.0\n");
}
}
