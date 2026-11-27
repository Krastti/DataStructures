#include "runtime/interpreter.h"
#include "loader/programLoader.h"

#include <exception>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cerr << "Использование: MiniLangCpp <путь к файлу .mini>\n";
    return 2;
  }

  auto stage = "чтения файла";
  try {
    std::ifstream input(argv[1], std::ios::binary);
    if (!input) throw std::runtime_error(std::string("Не удалось открыть файл: ") + argv[1]);

    // Читаем без замены CRLF, чтобы лексер получил исходные переводы строк.
    const std::string source((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    if (input.bad()) throw std::runtime_error(std::string("Не удалось прочитать файл: ") + argv[1]);

    stage = "загрузки программы";
    const auto program = minilang::ProgramLoader::load(source, {.sourcePath = argv[1]});

    stage = "выполнения";
    Interpreter interpreter;
    interpreter.execute(program.statements, std::cout);
    std::cout.flush();
    if (!std::cout) throw std::runtime_error("Не удалось записать результат");
    return 0;
  } catch (const std::exception& error) {
    const std::string message = error.what();
    // Загрузчик и исполнитель уже добавляют этап к своим сообщениям об ошибках.
    if (!message.starts_with("Ошибка ")) std::cerr << "Ошибка " << stage << ": ";
    std::cerr << message << '\n';
    return 1;
  }
}
