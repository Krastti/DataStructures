#include "AlphabeticalIndex.h"

#include <stdexcept>
#include <sstream>

namespace {
size_t characterCount(const std::string& text) {
  size_t count = 0;
  for (unsigned char character : text) {
    if ((character & 0xC0) != 0x80) ++count;
  }
  return count;
}
}

AlphabeticalIndex AlphabeticalIndex::build(const Array<std::string>& lines, size_t pageSize) {
  if (pageSize < 2) throw std::invalid_argument("Page size must be at least two characters");

  AlphabeticalIndex index;
  index.pagesByWord.reset(new Map<std::string, Array<size_t>>());
  Array<std::string> words;
  Array<bool> beginsLine;
  for (size_t lineIndex = 0; lineIndex < lines.size(); ++lineIndex) {
    const std::string& line = lines[lineIndex];
    std::istringstream stream(line);
    std::string word;
    bool first = true;
    while (stream >> word) {
      words.push_back(word);
      beginsLine.push_back(first);
      first = false;
    }
  }

  size_t page = 1;
  size_t used = 0;
  index.pageContents.push_back(std::string{});
  for (size_t i = 0; i < words.size(); ++i) {
    const std::string& word = words[i];
    const size_t wordCharacters = characterCount(word);
    if (wordCharacters > pageSize) throw std::invalid_argument("A word is longer than the page size");

    const size_t threeQuarters = pageSize / 4 * 3 + (pageSize % 4 * 3) / 4;
    const size_t pageLimit = page == 1 ? pageSize / 2 : (page % 10 == 0 ? threeQuarters : pageSize);
    const size_t separator = used == 0 ? 0 : 1;
    if (used + separator + wordCharacters > pageLimit) {
      ++page;
      used = 0;
      while (index.pageContents.size() < page) index.pageContents.push_back(std::string{});
    }
    const char delimiter = beginsLine[i] ? '\n' : ' ';
    if (used > 0) index.pageContents[page - 1].push_back(delimiter);
    used += (used == 0 ? 0 : 1) + wordCharacters;
    index.pageContents[page - 1] += word;

    if (!index.pagesByWord->containsKey(word)) index.pagesByWord->insert(word, Array<size_t>{});
    Array<size_t>& wordPages = index.pagesByWord->at(word);
    if (wordPages.size() == 0 || wordPages[wordPages.size() - 1] != page) wordPages.push_back(page);
  }

  const auto entries = index.pagesByWord->entries();
  for (size_t i = 0; i < entries.size(); ++i) {
    const auto& [word, wordPages] = entries[i];
    index.orderedEntries.push_back({word, wordPages});
  }
  return index;
}

bool AlphabeticalIndex::containsBinary(const std::string& word) const {
  return findBinary(word).size() != 0;
}

Array<size_t> AlphabeticalIndex::findBinary(const std::string& word) const {
  size_t first = 0;
  size_t last = orderedEntries.size();
  while (first < last) {
    const size_t middle = first + (last - first) / 2;
    if (orderedEntries[middle].word < word) first = middle + 1;
    else last = middle;
  }
  if (first == orderedEntries.size() || orderedEntries[first].word != word) return {};
  return orderedEntries[first].pages;
}

Array<size_t> AlphabeticalIndex::findTree(const std::string& word) const {
  try {
    return pagesByWord->get(word);
  } catch (const std::logic_error&) {
    return {};
  }
}

const Array<AlphabeticalEntry>& AlphabeticalIndex::entries() const {
  return orderedEntries;
}

const Array<std::string>& AlphabeticalIndex::pages() const {
  return pageContents;
}
