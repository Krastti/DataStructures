#ifndef DATASTRUCTURES_TIMER_H
#define DATASTRUCTURES_TIMER_H

#include <chrono>
#include <iostream>
#include <string>

class Timer {
  using string = std::string;
  using clock = std::chrono::steady_clock;

  string name;
  clock::time_point start;
  bool stopped = false;
  double resultMs = 0.0;

public:
  explicit Timer(string name = "") : name(std::move(name)), start(clock::now()) {}

  double stop() {
    if (!stopped) {
      resultMs = std::chrono::duration<double, std::milli>(
          std::chrono::steady_clock::now() - start
      ).count();
      stopped = true;

      std::cout << "[Timer]";
      if (!name.empty()) std::cout << " " << name;
      std::cout << ": " << resultMs << " ms\n";
    }
    return resultMs;
  }

  ~Timer() {
    if (!stopped) stop();
  }
};

#endif // DATASTRUCTURES_TIMER_H
