#include <gtest/gtest.h>

#include "lexer/lexer.h"

#include <stdexcept>
#include <string>

TEST(LexerTests, TokenizesAssignmentAndPrint) {
  Lexer lexer;
  const auto tokens = lexer.tokenize("remember age = 19\nprint age");
  ASSERT_EQ(tokens.size(), 8u);
  EXPECT_EQ(tokens[0].type, TokenType::Remember);
  EXPECT_EQ(tokens[1].type, TokenType::Identifier);
  EXPECT_EQ(tokens[1].lexeme, "age");
  EXPECT_EQ(tokens[2].type, TokenType::Equals);
  EXPECT_EQ(tokens[3].type, TokenType::Number);
  EXPECT_EQ(tokens[3].lexeme, "19");
  EXPECT_EQ(tokens[4].type, TokenType::Newline);
  EXPECT_EQ(tokens[5].type, TokenType::Print);
  EXPECT_EQ(tokens[6].type, TokenType::Identifier);
  EXPECT_EQ(tokens[7].type, TokenType::EndOfFile);
}

TEST(LexerTests, TokenizesRepeatWhileWhenOtherwiseEnd) {
  Lexer lexer;

  const auto tokens = lexer.tokenize("repeat while when otherwise end end");
  ASSERT_EQ(tokens.size(), 7u);
  EXPECT_EQ(tokens[0].type, TokenType::Repeat);
  EXPECT_EQ(tokens[1].type, TokenType::While);
  EXPECT_EQ(tokens[2].type, TokenType::When);
  EXPECT_EQ(tokens[3].type, TokenType::Otherwise);
  EXPECT_EQ(tokens[4].type, TokenType::End);
  EXPECT_EQ(tokens[5].type, TokenType::End);
  EXPECT_EQ(tokens[6].type, TokenType::EndOfFile);

}

TEST(LexerTests, RecognizesEveryOperatorAndParenthesis) {
  Lexer lexer;
  const auto tokens = lexer.tokenize("+ - * / = == != < <= > >= ( )");
  const TokenType types[] = {
    TokenType::Plus, TokenType::Minus, TokenType::Star, TokenType::Slash,
    TokenType::Equals, TokenType::EqualEqual, TokenType::BangEqual,
    TokenType::Less, TokenType::LessEqual, TokenType::Greater,
    TokenType::GreaterEqual, TokenType::LeftParen, TokenType::RightParen
  };
  const std::string lexemes[] = {"+", "-", "*", "/", "=", "==", "!=", "<", "<=", ">", ">=", "(", ")"};
  ASSERT_EQ(tokens.size(), 14u);
  for (std::size_t i = 0; i < 13; ++i) {
    EXPECT_EQ(tokens[i].type, types[i]);
    EXPECT_EQ(tokens[i].lexeme, lexemes[i]);
  }
  EXPECT_EQ(tokens[13].type, TokenType::EndOfFile);
}

TEST(LexerTests, RecognizesArrowOnlyWithAdjacentSymbols) {
  const auto tokens = Lexer::tokenize("a->b - > >= -");
  const Token expected[] = {
    {TokenType::Identifier, "a"},
    {TokenType::Arrow, "->"},
    {TokenType::Identifier, "b"},
    {TokenType::Minus, "-"},
    {TokenType::Greater, ">"},
    {TokenType::GreaterEqual, ">="},
    {TokenType::Minus, "-"},
    {TokenType::EndOfFile, ""}
  };
  ASSERT_EQ(tokens.size(), sizeof(expected) / sizeof(expected[0]));
  for (std::size_t i = 0; i < tokens.size(); ++i) {
    EXPECT_EQ(tokens[i].type, expected[i].type);
    EXPECT_EQ(tokens[i].lexeme, expected[i].lexeme);
  }
}

TEST(LexerTests, RecognizesReturnAsAnExactKeyword) {
  const auto tokens = Lexer::tokenize("return returnValue \"return\"");
  ASSERT_EQ(tokens.size(), 4u);
  EXPECT_EQ(tokens[0].type, TokenType::Return);
  EXPECT_EQ(tokens[0].lexeme, "return");
  EXPECT_EQ(tokens[1].type, TokenType::Identifier);
  EXPECT_EQ(tokens[2].type, TokenType::String);
  EXPECT_EQ(tokens[3].type, TokenType::EndOfFile);
}

TEST(LexerTests, KeepsTokenBoundariesWithoutWhitespace) {
  const auto tokens = Lexer::tokenize("print2<=12_x==7>3");
  const Token expected[] = {
    {TokenType::Identifier, "print2"},
    {TokenType::LessEqual, "<="},
    {TokenType::Number, "12"},
    {TokenType::Identifier, "_x"},
    {TokenType::EqualEqual, "=="},
    {TokenType::Number, "7"},
    {TokenType::Greater, ">"},
    {TokenType::Number, "3"},
    {TokenType::EndOfFile, ""}
  };
  ASSERT_EQ(tokens.size(), sizeof(expected) / sizeof(expected[0]));
  for (std::size_t i = 0; i < tokens.size(); ++i) {
    EXPECT_EQ(tokens[i].type, expected[i].type);
    EXPECT_EQ(tokens[i].lexeme, expected[i].lexeme);
  }
}

TEST(LexerTests, HandlesWhitespaceAndBothNewlineFormats) {
  Lexer lexer;
  const auto tokens = lexer.tokenize(" \t\r\n\n");
  ASSERT_EQ(tokens.size(), 3u);
  EXPECT_EQ(tokens[0].type, TokenType::Newline);
  EXPECT_EQ(tokens[0].lexeme, "\r\n");
  EXPECT_EQ(tokens[1].type, TokenType::Newline);
  EXPECT_EQ(tokens[1].lexeme, "\n");
  EXPECT_EQ(tokens[2].type, TokenType::EndOfFile);
}

TEST(LexerTests, KeepsKeywordPrefixesAndCaseSensitiveNamesAsIdentifiers) {
  Lexer lexer;
  const auto tokens = lexer.tokenize("remembered print2 Remember _x9 123");
  ASSERT_EQ(tokens.size(), 6u);
  for (std::size_t i = 0; i < 4; ++i) EXPECT_EQ(tokens[i].type, TokenType::Identifier);
  EXPECT_EQ(tokens[3].lexeme, "_x9");
  EXPECT_EQ(tokens[4].type, TokenType::Number);
  EXPECT_EQ(tokens[4].lexeme, "123");
}

TEST(LexerTests, EmptyInputAndRepeatedCallsHaveIndependentResults) {
  Lexer lexer;
  const auto first = lexer.tokenize("print 1");
  const auto empty = lexer.tokenize("");
  ASSERT_EQ(first.size(), 3u);
  ASSERT_EQ(empty.size(), 1u);
  EXPECT_EQ(empty[0].type, TokenType::EndOfFile);
  EXPECT_TRUE(empty[0].lexeme.empty());
}

TEST(LexerTests, RejectsUnsupportedCharactersAndIncompleteOperators) {
  Lexer lexer;
  for (const std::string source : {"!", "\r", "@"}) {
    EXPECT_THROW(lexer.tokenize(source), std::invalid_argument) << source;
  }
}

TEST(LexerTests, TokenizesDecimalsAndUpdateOperators) {
  const auto tokens = Lexer::tokenize("0.25++ -- += -= -> + -");
  const Token expected[] = {
    {TokenType::Number, "0.25"}, {TokenType::Increment, "++"},
    {TokenType::Decrement, "--"}, {TokenType::PlusEquals, "+="},
    {TokenType::MinusEquals, "-="}, {TokenType::Arrow, "->"},
    {TokenType::Plus, "+"}, {TokenType::Minus, "-"}, {TokenType::EndOfFile, ""}
  };
  ASSERT_EQ(tokens.size(), sizeof(expected) / sizeof(expected[0]));
  for (std::size_t i = 0; i < tokens.size(); ++i) {
    EXPECT_EQ(tokens[i].type, expected[i].type);
    EXPECT_EQ(tokens[i].lexeme, expected[i].lexeme);
  }
}

TEST(LexerTests, RejectsDecimalPointsWithoutDigitsOnBothSides) {
  for (const char* source : {".5", "1.", "1..5", "1.2.3", "1. 5"}) {
    EXPECT_THROW(Lexer::tokenize(source), std::invalid_argument) << source;
  }
}

TEST(LexerTests, TokenizesStringWithSpacesAndOperatorsAsOneToken) {
  const auto tokens = Lexer::tokenize("print \"remember x = 1 + (2) != 3 -> - 0.25 ++ +=\"");
  ASSERT_EQ(tokens.size(), 3u);
  EXPECT_EQ(tokens[0].type, TokenType::Print);
  EXPECT_EQ(tokens[1].type, TokenType::String);
  EXPECT_EQ(tokens[1].lexeme, "\"remember x = 1 + (2) != 3 -> - 0.25 ++ +=\"");
  EXPECT_EQ(tokens[2].type, TokenType::EndOfFile);
}

TEST(LexerTests, HandlesEmptyAdjacentStringsAndFollowingTokens) {
  const auto tokens = Lexer::tokenize("\"\"\"hello\"+1\r\nprint 2");
  ASSERT_EQ(tokens.size(), 8u);
  EXPECT_EQ(tokens[0].type, TokenType::String);
  EXPECT_EQ(tokens[0].lexeme, "\"\"");
  EXPECT_EQ(tokens[1].type, TokenType::String);
  EXPECT_EQ(tokens[1].lexeme, "\"hello\"");
  EXPECT_EQ(tokens[2].type, TokenType::Plus);
  EXPECT_EQ(tokens[3].type, TokenType::Number);
  EXPECT_EQ(tokens[4].type, TokenType::Newline);
  EXPECT_EQ(tokens[5].type, TokenType::Print);
  EXPECT_EQ(tokens[6].lexeme, "2");
  EXPECT_EQ(tokens[7].type, TokenType::EndOfFile);
}

TEST(LexerTests, PreservesTabsAndBackslashesInsideStrings) {
  const auto tokens = Lexer::tokenize("\"a\tb\\c\"");
  ASSERT_EQ(tokens.size(), 2u);
  EXPECT_EQ(tokens[0].type, TokenType::String);
  EXPECT_EQ(tokens[0].lexeme, "\"a\tb\\c\"");
}

TEST(LexerTests, RejectsUnterminatedAndMultilineStrings) {
  for (const std::string source : {"\"", "print \"text", "\"a\nb\"", "\"a\r\nb\"", "\"a\rb\""}) {
    EXPECT_THROW(Lexer::tokenize(source), std::invalid_argument) << source;
  }
}

TEST(LexerTests, RecognizesUsingAndPreservesQuotedPath) {
  const auto tokens = Lexer::tokenize("using \"lib/my math.mini\"\r\nusingValue Using");
  ASSERT_EQ(tokens.size(), 6u);
  EXPECT_EQ(tokens[0].type, TokenType::Using);
  EXPECT_EQ(tokens[0].lexeme, "using");
  EXPECT_EQ(tokens[1].type, TokenType::String);
  EXPECT_EQ(tokens[1].lexeme, "\"lib/my math.mini\"");
  EXPECT_EQ(tokens[2].type, TokenType::Newline);
  EXPECT_EQ(tokens[3].type, TokenType::Identifier);
  EXPECT_EQ(tokens[4].type, TokenType::Identifier);
  EXPECT_EQ(tokens[5].type, TokenType::EndOfFile);
}

TEST(LexerTests, KeepsMinusSeparateFromNumbersAndRecognizesDecrement) {
  const auto tokens = Lexer::tokenize("-5 -0.5 1-2 --x - -x");
  const TokenType expected[] = {
    TokenType::Minus, TokenType::Number, TokenType::Minus, TokenType::Number,
    TokenType::Number, TokenType::Minus, TokenType::Number,
    TokenType::Decrement, TokenType::Identifier,
    TokenType::Minus, TokenType::Minus, TokenType::Identifier, TokenType::EndOfFile
  };
  ASSERT_EQ(tokens.size(), 13u);
  for (std::size_t i = 0; i < 13; ++i) EXPECT_EQ(tokens[i].type, expected[i]);
  EXPECT_EQ(tokens[1].lexeme, "5");
  EXPECT_EQ(tokens[3].lexeme, "0.5");
}
