#ifndef DATASTRUCTURES_PROGRAM_LOADER_H
#define DATASTRUCTURES_PROGRAM_LOADER_H

#include "loadState.h"

#include <string>

namespace minilang {

class ProgramLoader : protected LoadState {
public:
  // Текст основного файла передаётся вызывающим кодом, библиотеки читаются с диска.
  static PreparedProgram load(const std::string& source, const LoadContext& context = {});
};

}

#endif // DATASTRUCTURES_PROGRAM_LOADER_H
