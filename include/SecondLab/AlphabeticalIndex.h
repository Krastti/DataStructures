#ifndef DATASTRUCTURES_SECOND_LAB_ALPHABETICAL_INDEX_H
#define DATASTRUCTURES_SECOND_LAB_ALPHABETICAL_INDEX_H

#include "../Array/Array.h"
#include "../Trees/RedBlackTree.h"
#include "../SmartPointers/UniquePtr.h"
#include <cstddef>
#include <string>

struct AlphabeticalEntry {
  std::string word;
  Array<size_t> pages;
};

class AlphabeticalIndex {
private:
  UniquePtr<Map<std::string, Array<size_t>>> pagesByWord;
  Array<AlphabeticalEntry> orderedEntries;
  Array<std::string> pageContents;

public:
  static AlphabeticalIndex build(const Array<std::string>& lines, size_t pageSize);

  [[nodiscard]] bool containsBinary(const std::string& word) const;
  [[nodiscard]] Array<size_t> findBinary(const std::string& word) const;
  [[nodiscard]] Array<size_t> findTree(const std::string& word) const;
  [[nodiscard]] const Array<AlphabeticalEntry>& entries() const;
  [[nodiscard]] const Array<std::string>& pages() const;
};

#endif // DATASTRUCTURES_SECOND_LAB_ALPHABETICAL_INDEX_H
