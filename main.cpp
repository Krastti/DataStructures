#include "include/SmartPointers/UniquePtr.h"

#include <iostream>

int main() {

  UniquePtr arr(new int[5]());

  for (int i = 0; i < 5; i++) {
    arr.get()[i] = i;
  }

  std::cout << arr.get() << std::endl;

  return 0;
}
