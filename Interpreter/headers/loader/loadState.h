#ifndef DATASTRUCTURES_LOAD_STATE_H
#define DATASTRUCTURES_LOAD_STATE_H

#include "Array/Array.h"
#include "RedBlackTree.h"
#include "UniquePtr.h"
#include "ast/statements.h"
#include "lexer/token.h"
#include "Stack/stack.h"
#include "parser/parser.h"

#include <cstddef>
#include <filesystem>
#include <string>

namespace minilang {

// Задаёт путь основного файла либо базовый каталог для разрешения относительных подключений.
struct LoadContext {
  // Путь основного файла, по которому определяются его каталог и участие в цепочке подключений.
  std::filesystem::path sourcePath;
  // Каталог для относительных подключений из текста без пути к файлу.
  std::filesystem::path baseDirectory;
};

// Хранит токены основного текста и владеет общим AST программы с подключёнными библиотеками.
struct PreparedProgram {
  // Токены основного текста для отображения в GUI.
  Array<Token> tokens;
  // Объявления библиотек и инструкции основной программы в порядке выполнения.
  Array<UniquePtr<ast::Statement>> statements;
};

// Хранит состояние обработки одного файла, чтобы продолжить её после загрузки зависимостей.
struct FileFrame {
  // Канонический путь файла. Для текста редактора без пути остаётся пустым.
  std::filesystem::path path;
  // Каталог, от которого разрешаются относительные подключения этого файла.
  std::filesystem::path directory;
  // Подключения и AST файла, ожидающие объединения с общей программой.
  Parser::ParsedFile parsed;
  // Индекс следующего подключения для обработки после возврата из зависимости.
  std::size_t nextImport = 0;
};

// Хранит состояние одной загрузки и собирает программу, отслеживая повторы и циклы подключений.
class LoadState {
protected:
  // Накапливает токены основного текста и общий AST до передачи вызывающему коду.
  PreparedProgram program;
  // Кадры незавершённых файлов, текущий файл находится на вершине.
  Stack<FileFrame> pending;
  // Имена файлов от основного исходника до текущего для сообщений об ошибках.
  Array<std::string> chain;

  // Канонические пути обработанных файлов. False обозначает активный файл, true — успешно загруженный.
  Map<std::string, bool> files;

  // Прерывает загрузку с указанием этапа, файла, причины и цепочки подключений.
  [[noreturn]] void fail(const std::string& stage, const std::string& path, const std::string& reason) const;

  // Канонизирует путь и проверяет, что он указывает на существующий обычный файл.
  [[nodiscard]] std::filesystem::path canonicalFile(const std::filesystem::path& path) const;

  // Разбирает текст в подключения и AST, сохраняет токены в переданный массив.
  // При ошибке добавляет путь файла и цепочку подключений.
  Parser::ParsedFile parse(const std::string& source, const std::string& path, Parser::SourceKind kind, Array<Token>& tokens) const;

  // Читает исходный текст без преобразования переводов строк, сообщает об ошибках доступа и чтения.
  [[nodiscard]] std::string read(const std::filesystem::path& path) const;

  // Разрешает путь зависимости и помещает новый файл в стек обработки.
  // Пропускает загруженные файлы и отклоняет циклические подключения.
  void loadImport(const std::string& import, const std::filesystem::path& directory);

  // Собирает программу обходом зависимостей в глубину и передаёт владение общим AST вызывающему коду.
  PreparedProgram loadProgram(const std::string& source, const LoadContext& context);
};

}

#endif // DATASTRUCTURES_LOAD_STATE_H
