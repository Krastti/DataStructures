#ifndef DATASTRUCTURES_SECOND_LAB_PERSON_INDEX_H
#define DATASTRUCTURES_SECOND_LAB_PERSON_INDEX_H

#include "../Array/Array.h"
#include "../Trees/RedBlackTree.h"
#include <string>
#include <tuple>

struct PersonRecord {
  int id;
  std::string firstName;
  std::string lastName;
  int birthYear;
};

class PersonIndex {
private:
  using CompositeKey = std::tuple<std::string, std::string, int>;
  Map<CompositeKey, Array<PersonRecord>> compositeIndex;
  Map<int, Array<PersonRecord>> birthYearIndex;

public:
  void add(const PersonRecord& person);
  [[nodiscard]] Array<PersonRecord> findComposite(const std::string& firstName,
                                                  const std::string& lastName,
                                                  int birthYear) const;
  [[nodiscard]] Array<PersonRecord> findBirthYearRange(int firstYear, int lastYear) const;
  [[nodiscard]] Array<PersonRecord> all() const;
};

#endif // DATASTRUCTURES_SECOND_LAB_PERSON_INDEX_H
