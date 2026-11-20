#ifndef DATASTRUCTURES_TEST_ALLOCATION_FAILURE_H
#define DATASTRUCTURES_TEST_ALLOCATION_FAILURE_H

#include <cstddef>

namespace test {

struct RejectAllocations {
  explicit RejectAllocations(std::ptrdiff_t allowedAllocations = 0);
  ~RejectAllocations();

  RejectAllocations(const RejectAllocations&) = delete;
  RejectAllocations& operator=(const RejectAllocations&) = delete;

private:
  std::ptrdiff_t previousBudget;
};

struct TrackAllocations {
  explicit TrackAllocations(std::ptrdiff_t& balance);
  ~TrackAllocations();

  TrackAllocations(const TrackAllocations&) = delete;
  TrackAllocations& operator=(const TrackAllocations&) = delete;
};

}

#endif
