// ReSharper disable CppUseStructuredBinding
#include "loader/programLoader.h"

#include "lexer/lexer.h"

#include <cstddef>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace minilang {
namespace fs = std::filesystem;

[[noreturn]] void LoadState::fail(const std::string& stage, const std::string& path, const std::string& reason) const {
  std::string message = "Ошибка " + stage + " в " + path + ": " + reason;

  message += "\nЦепочка подключений: ";
  for (std::size_t i = 0; i < chain.size(); ++i) {
    if (i != 0) message += " → ";
    message += chain[i];
  }
  if (chain.size() == 0 || chain[chain.size() - 1] != path) {
    if (chain.size() != 0) message += " → ";
    message += path;
  }
  for (auto position = message.find('\0'); position != std::string::npos;
       position = message.find('\0', position + 2)) {
    message.replace(position, 1, "\\0");
  }
  throw std::runtime_error(message);
}

fs::path LoadState::canonicalFile(const fs::path& path) const {
  if (path.string().find('\0') != std::string::npos) {
    fail("загрузки", path.string(), "Путь содержит нулевой байт");
  }

  std::error_code error;
  auto canonical = fs::canonical(path, error);

  if (error) fail("загрузки", path.string(), "Не удалось разрешить путь: " + error.message());

  if (!fs::is_regular_file(canonical, error)) {
    fail("загрузки", canonical.string(), "Ожидался обычный файл");
  }
  return canonical;
}

Parser::ParsedFile LoadState::parse(const std::string& source, const std::string& path, const Parser::SourceKind kind, Array<Token>& tokens) const {
  try {
    tokens = Lexer::tokenize(source);
  } catch (const std::invalid_argument& error) {
    fail("лексического анализа", path, error.what());
  }

  try {
    return Parser::parseFile(tokens, kind);
  } catch (const std::invalid_argument& error) {
    fail("синтаксического анализа", path, error.what());
  }
}

std::string LoadState::read(const fs::path& path) const {
  std::ifstream input(path, std::ios::binary);

  if (!input) fail("загрузки", path.string(), "Не удалось открыть файл");

  try {
    const std::string source((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());

    if (input.bad()) fail("загрузки", path.string(), "Не удалось прочитать файл");

    return source;
  } catch (const std::ios_base::failure& error) {
    fail("загрузки", path.string(), "Не удалось прочитать файл: " + std::string(error.what()));
  }
}

void LoadState::loadImport(const std::string& import, const fs::path& directory) {
  const fs::path requested(import);

  if (requested.is_relative() && directory.empty()) {
    fail("загрузки", import, "Для относительного пути нужен путь исходного файла или базовый каталог");
  }

  const auto path = canonicalFile(requested.is_absolute() ? requested : directory / requested);
  const auto key = path.string();

  if (files.containsKey(key)) {
    if (!files.at(key)) {
      chain.push_back(key);
      fail("загрузки", key, "Циклическая зависимость");
    }
    return;
  }
  chain.push_back(key);
  files.insert(key, false);

  Array<Token> tokens;
  auto parsed = parse(read(path), key, Parser::SourceKind::Library, tokens);

  pending.push(FileFrame{.path = path, .directory = path.parent_path(), .parsed = std::move(parsed)});
}

PreparedProgram LoadState::loadProgram(const std::string& source, const LoadContext& context) {
  if (!context.sourcePath.empty() && !context.baseDirectory.empty()) {
    fail("загрузки", context.sourcePath.string(), "Задайте путь исходного файла либо базовый каталог");
  }

  fs::path path;
  fs::path directory;

  if (!context.sourcePath.empty()) {
    path = canonicalFile(context.sourcePath);
    directory = path.parent_path();
    files.insert(path.string(), false);
  } else if (!context.baseDirectory.empty()) {
    std::error_code error;
    directory = fs::absolute(context.baseDirectory, error);
    if (error) fail("загрузки", context.baseDirectory.string(), error.message());
  }

  const auto label = path.empty() ? std::string("<редактор>") : path.string();
  chain.push_back(label);

  // То есть он разбирает здесь основной файл и помещает его в стек незавершенных файлов
  auto parsed = parse(source, label, Parser::SourceKind::Main, program.tokens);
  pending.push(FileFrame{.path = path, .directory = directory, .parsed = std::move(parsed)});

  // А дальше в цикле начинает разбирать остальные файлы
  while (!pending.empty()) {
    auto& frame = pending.top();

    if (frame.nextImport < frame.parsed.imports.size()) {
      const auto& import = frame.parsed.imports[frame.nextImport++];
      loadImport(import, frame.directory);
      continue;
    }
    for (std::size_t i = 0; i < frame.parsed.statements.size(); ++i) {
      program.statements.push_back(std::move(frame.parsed.statements[i]));
    }
    if (!frame.path.empty()) files.at(frame.path.string()) = true;
    pending.pop();
    chain.pop_back();
  }
  return std::move(program);
}

PreparedProgram ProgramLoader::load(const std::string& source, const LoadContext& context) {
  return ProgramLoader{}.loadProgram(source, context);
}

}
