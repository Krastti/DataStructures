#include "../include/AlphabeticalIndex.h"
#include "../include/PersonIndex.h"
#include "../../../DataStructures/tests/Array/ArrayTestHelpers.h"
#include <gtest/gtest.h>

#include <initializer_list>
#include <stdexcept>

TEST(AlphabeticalIndexTests, CountsUtf8CharactersAndKeepsLineBreaks) {
  Array<std::string> lines;
  lines.push_back("кот мир");
  lines.push_back("дом");
  AlphabeticalIndex index = AlphabeticalIndex::build(lines, 10);

  ASSERT_EQ(index.pages().size(), 2);
  EXPECT_EQ(index.pages()[0], "кот");
  EXPECT_EQ(index.pages()[1], "мир\nдом");
  EXPECT_EQ(toVector(index.findBinary("дом")), toVector(index.findTree("дом")));
  EXPECT_EQ(toVector(index.findBinary("дом")), (std::vector<size_t>{2}));
  EXPECT_FALSE(index.containsBinary("нет"));
}

TEST(AlphabeticalIndexTests, FirstAndTenthPagesRespectReducedCapacity) {
  Array<std::string> lines;
  lines.push_back("w0 w1 w2 w3 w4 w5 w6 w7 w8 w9 w10 w11");
  AlphabeticalIndex index = AlphabeticalIndex::build(lines, 4);

  EXPECT_LE(index.pages()[0].size(), 2);
  ASSERT_GT(index.pages().size(), 10);
  EXPECT_LE(index.pages()[9].size(), 3);
}

TEST(AlphabeticalIndexTests, SkipsReducedPagesWhenWholeWordDoesNotFit) {
  Array<std::string> lines;
  lines.push_back("abcd abcd abcd abcd abcd abcd abcd abcd abcd");
  const auto index = AlphabeticalIndex::build(lines, 4);

  ASSERT_EQ(index.pages().size(), 11);
  for (size_t page = 0; page < index.pages().size(); ++page) {
    EXPECT_EQ(index.pages()[page], (page == 0 || page == 9 ? "" : "abcd")) << page + 1;
  }
  const std::vector<size_t> expected{2, 3, 4, 5, 6, 7, 8, 9, 11};
  EXPECT_EQ(toVector(index.findBinary("abcd")), expected);
  EXPECT_EQ(toVector(index.findTree("abcd")), expected);
}

TEST(AlphabeticalIndexTests, RejectsInvalidPageSizesAndOverlongWords) {
  Array<std::string> lines;
  lines.push_back("word");
  EXPECT_THROW(AlphabeticalIndex::build(lines, 1), std::invalid_argument);
  EXPECT_THROW(AlphabeticalIndex::build(lines, 3), std::invalid_argument);
}

TEST(PersonIndexTests, FindsCompositeKeysAndInclusiveYearRanges) {
  PersonIndex index;
  index.add({1, "Ada", "Lovelace", 1815});
  index.add({2, "Grace", "Hopper", 1906});
  index.add({3, "Ada", "Lovelace", 1815});
  index.add({4, "Alan", "Turing", 1912});

  EXPECT_EQ(index.findComposite("Ada", "Lovelace", 1815).size(), 2);
  EXPECT_EQ(index.findComposite("Ada", "Lovelace", 1816).size(), 0);
  const auto people = index.findBirthYearRange(1900, 1910);
  ASSERT_EQ(people.size(), 1);
  EXPECT_EQ(people[0].id, 2);
  EXPECT_THROW((void)index.findBirthYearRange(1910, 1900), std::invalid_argument);
}

TEST(PersonIndexTests, ComparesAllFieldsAndDuplicateCountsRegardlessOfOrder) {
  const PersonRecord ada{1, "Ada", "Lovelace", 1815};
  const PersonRecord alan{1, "Alan", "Turing", 1912};
  auto records = [](std::initializer_list<PersonRecord> values) {
    Array<PersonRecord> result;
    for (const auto& person : values) result.push_back(person);
    return result;
  };
  const auto expected = records({ada, alan, ada});

  EXPECT_TRUE(samePersonRecords(expected, records({alan, ada, ada})));
  EXPECT_FALSE(samePersonRecords(expected, records({alan, alan, ada})));
  EXPECT_FALSE(samePersonRecords(expected, records({alan, ada})));
  EXPECT_FALSE(samePersonRecords(expected, records({alan, ada, {2, "Ada", "Lovelace", 1815}})));
  EXPECT_FALSE(samePersonRecords(expected, records({alan, ada, {1, "Grace", "Lovelace", 1815}})));
  EXPECT_FALSE(samePersonRecords(expected, records({alan, ada, {1, "Ada", "Hopper", 1815}})));
  EXPECT_FALSE(samePersonRecords(expected, records({alan, ada, {1, "Ada", "Lovelace", 1816}})));
}
