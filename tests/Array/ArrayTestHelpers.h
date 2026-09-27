#ifndef DATASTRUCTURES_ARRAY_TEST_HELPERS_H
#define DATASTRUCTURES_ARRAY_TEST_HELPERS_H

#include "../../include/Array/Array.h"

#include <vector>

template<typename T>
std::vector<T> toVector(const Array<T>& values) {
  std::vector<T> result;
  result.reserve(values.size());
  for (size_t i = 0; i < values.size(); ++i) result.push_back(values[i]);
  return result;
}

#endif // DATASTRUCTURES_ARRAY_TEST_HELPERS_H
