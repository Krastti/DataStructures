#include "PersonIndex.h"

#include <stdexcept>

namespace {
template<typename Key>
void addToBucket(Map<Key, Array<PersonRecord>>& tree, const Key& key,
                 const PersonRecord& person) {
  if (!tree.containsKey(key)) tree.insert(key, Array<PersonRecord>{});
  tree.at(key).push_back(person);
}
}

void PersonIndex::add(const PersonRecord& person) {
  addToBucket(compositeIndex, CompositeKey{person.firstName, person.lastName, person.birthYear}, person);
  addToBucket(birthYearIndex, person.birthYear, person);
}

Array<PersonRecord> PersonIndex::findComposite(const std::string& firstName,
                                               const std::string& lastName,
                                               int birthYear) const {
  try {
    return compositeIndex.get(CompositeKey{firstName, lastName, birthYear});
  } catch (const std::logic_error&) {
    return {};
  }
}

Array<PersonRecord> PersonIndex::findBirthYearRange(int firstYear, int lastYear) const {
  if (firstYear > lastYear) throw std::invalid_argument("The first year must not exceed the last year");
  Array<PersonRecord> result;
  const auto matches = birthYearIndex.range(firstYear, lastYear);
  for (size_t i = 0; i < matches.size(); ++i) {
    const auto& [year, people] = matches[i];
    (void)year;
    for (size_t j = 0; j < people.size(); ++j) result.push_back(people[j]);
  }
  return result;
}

Array<PersonRecord> PersonIndex::all() const {
  Array<PersonRecord> result;
  const auto entries = birthYearIndex.entries();
  for (size_t i = 0; i < entries.size(); ++i) {
    const auto& [year, people] = entries[i];
    (void)year;
    for (size_t j = 0; j < people.size(); ++j) result.push_back(people[j]);
  }
  return result;
}
