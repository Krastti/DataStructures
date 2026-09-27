#include "include/SecondLab/AlphabeticalIndex.h"
#include "include/SecondLab/PersonIndex.h"
#include "include/Graph/ShortestPath.h"
#include "include/Array/Array.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>

namespace {
volatile size_t benchmarkSink = 0;

void consumeBenchmarkResult(size_t value) {
  benchmarkSink = benchmarkSink + value;
}

template<typename Action>
double averageNanoseconds(Action action, size_t repetitions = 1000) {
  const auto start = std::chrono::steady_clock::now();
  for (size_t i = 0; i < repetitions; ++i) action();
  const auto finish = std::chrono::steady_clock::now();
  return static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count()) /
         static_cast<double>(repetitions);
}

std::string csvField(const std::string& value) {
  std::string escaped = "\"";
  for (char character : value) {
    if (character == '"') escaped += '"';
    escaped += character;
  }
  return escaped + '"';
}

void runAlphabeticalIndex() {
  size_t pageSize;
  std::cout << "Размер страницы в символах: ";
  std::cin >> pageSize;
  std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  std::cout << "Введите текст построчно; отдельная точка завершает ввод:\n";
  Array<std::string> lines;
  std::string line;
  while (std::getline(std::cin, line) && line != ".") lines.push_back(line);

  const AlphabeticalIndex index = AlphabeticalIndex::build(lines, pageSize);
  std::cout << "Исходный текст:\n";
  for (size_t i = 0; i < lines.size(); ++i) std::cout << lines[i] << '\n';
  std::cout << "\nСтраницы:\n";
  for (size_t page = 0; page < index.pages().size(); ++page) {
    std::cout << page + 1 << ": " << index.pages()[page] << '\n';
  }
  std::cout << "\nАлфавитный указатель:\n";
  for (size_t i = 0; i < index.entries().size(); ++i) {
    const AlphabeticalEntry& entry = index.entries()[i];
    std::cout << entry.word << ": ";
    for (size_t j = 0; j < entry.pages.size(); ++j) std::cout << entry.pages[j] << ' ';
    std::cout << '\n';
  }

  Array<std::string> words;
  for (size_t i = 0; i < lines.size(); ++i) {
    const std::string& sourceLine = lines[i];
    std::istringstream stream(sourceLine);
    std::string word;
    while (stream >> word) words.push_back(word);
  }
  Array<std::string> ascendingWords = words;
  if (ascendingWords.size() > 1) std::sort(&ascendingWords[0], &ascendingWords[0] + ascendingWords.size());
  Array<std::string> descendingWords = ascendingWords;
  if (descendingWords.size() > 1) std::reverse(&descendingWords[0], &descendingWords[0] + descendingWords.size());
  Array<std::string> shuffledWords = words;
  std::mt19937 wordOrderRandom(42);
  if (shuffledWords.size() > 1) std::shuffle(&shuffledWords[0], &shuffledWords[0] + shuffledWords.size(), wordOrderRandom);
  auto asSingleLine = [](const Array<std::string>& orderedWords) {
    std::string text;
    for (size_t i = 0; i < orderedWords.size(); ++i) {
      const std::string& word = orderedWords[i];
      if (!text.empty()) text += ' ';
      text += word;
    }
    Array<std::string> result;
    if (!text.empty()) result.push_back(text);
    return result;
  };
  auto measureBuild = [&](const Array<std::string>& orderedWords) {
    const Array<std::string> benchmarkLines = asSingleLine(orderedWords);
    return averageNanoseconds([&] {
      const AlphabeticalIndex built = AlphabeticalIndex::build(benchmarkLines, pageSize);
      consumeBenchmarkResult(built.entries().size());
    }, 10);
  };
  const double ascendingBuildTime = measureBuild(ascendingWords);
  const double descendingBuildTime = measureBuild(descendingWords);
  const double shuffledBuildTime = measureBuild(shuffledWords);
  std::cout << "Построение дерева и указателя, нс (по возрастанию / убыванию / перемешано): "
            << ascendingBuildTime << " / " << descendingBuildTime << " / " << shuffledBuildTime << '\n';

  std::cout << "Слово для поиска: ";
  std::string query;
  std::cin >> query;
  std::cout << "1 — бинарный поиск, 2 — красно-чёрное дерево, 3 — сравнить: ";
  int algorithm;
  std::cin >> algorithm;
  auto printPages = [](const std::string& name, const Array<size_t>& pages) {
    std::cout << name << ": ";
    for (size_t i = 0; i < pages.size(); ++i) std::cout << pages[i] << ' ';
    if (pages.size() == 0) std::cout << "не найдено";
    std::cout << '\n';
  };
  if (algorithm < 1 || algorithm > 3) throw std::invalid_argument("Неизвестный алгоритм поиска");
  if (algorithm == 1 || algorithm == 3) {
    printPages("Бинарный поиск", index.findBinary(query));
    const double binaryTime = averageNanoseconds([&] {
      consumeBenchmarkResult(index.findBinary(query).size());
    });
    std::cout << "Среднее время бинарного поиска, нс: " << binaryTime << '\n';
  }
  if (algorithm == 2 || algorithm == 3) {
    printPages("Красно-чёрное дерево", index.findTree(query));
    const double treeTime = averageNanoseconds([&] {
      consumeBenchmarkResult(index.findTree(query).size());
    });
    std::cout << "Среднее время поиска в дереве, нс: " << treeTime << '\n';
  }
  std::ofstream csv("alphabetical_index.csv");
  csv << "word,pages\n";
  for (size_t i = 0; i < index.entries().size(); ++i) {
    const AlphabeticalEntry& entry = index.entries()[i];
    std::string pageList;
    for (size_t i = 0; i < entry.pages.size(); ++i) {
      if (i != 0) pageList += ';';
      pageList += std::to_string(entry.pages[i]);
    }
    csv << csvField(entry.word) << ',' << csvField(pageList) << '\n';
  }
  std::cout << "Индекс сохранён в alphabetical_index.csv\n";
  std::ofstream buildCsv("alphabetical_index_build.csv");
  buildCsv << "input_order,elapsed_nanoseconds,word_count\n"
           << "ascending," << ascendingBuildTime << ',' << words.size() << '\n'
           << "descending," << descendingBuildTime << ',' << words.size() << '\n'
           << "shuffled," << shuffledBuildTime << ',' << words.size() << '\n';
  std::cout << "Замеры построения сохранены в alphabetical_index_build.csv\n";
}

void runPersonIndex() {
  size_t count;
  std::cout << "Число записей (0 — автоматический набор): ";
  std::cin >> count;
  Array<PersonRecord> people;
  if (count == 0) {
    unsigned int seed;
    std::cout << "Размер автоматического набора и seed: ";
    std::cin >> count >> seed;
    if (count == 0 || count > 100000) {
      throw std::invalid_argument("Число генерируемых записей должно быть от 1 до 100000");
    }
    constexpr const char* firstNames[] = {"Ada", "Grace", "Alan", "Katherine", "Edsger"};
    constexpr const char* lastNames[] = {"Lovelace", "Hopper", "Turing", "Johnson", "Dijkstra"};
    std::mt19937 random(seed);
    std::uniform_int_distribution<int> years(1900, 2000);
    for (size_t i = 0; i < count; ++i) {
      people.push_back(PersonRecord{static_cast<int>(i + 1), firstNames[i % 5],
                                    lastNames[(i / 5) % 5], years(random)});
    }
  } else {
    if (count > 100000) throw std::invalid_argument("Число записей не должно превышать 100000");
    std::cout << "Введите записи: id firstName lastName birthYear\n";
    for (size_t i = 0; i < count; ++i) {
      PersonRecord person{};
      std::cin >> person.id >> person.firstName >> person.lastName >> person.birthYear;
      people.push_back(person);
    }
  }

  PersonIndex index;
  for (size_t i = 0; i < people.size(); ++i) index.add(people[i]);
  std::cout << "Исходные записи:\n";
  for (size_t i = 0; i < people.size(); ++i) {
    const PersonRecord& person = people[i];
    std::cout << person.id << ' ' << person.firstName << ' ' << person.lastName << ' ' << person.birthYear << '\n';
  }

  auto byBirthYear = [](const PersonRecord& left, const PersonRecord& right) {
    return std::tie(left.birthYear, left.id) < std::tie(right.birthYear, right.id);
  };
  Array<PersonRecord> ascendingPeople = people;
  if (ascendingPeople.size() > 1) {
    std::stable_sort(&ascendingPeople[0], &ascendingPeople[0] + ascendingPeople.size(), byBirthYear);
  }
  Array<PersonRecord> descendingPeople = ascendingPeople;
  if (descendingPeople.size() > 1) {
    std::reverse(&descendingPeople[0], &descendingPeople[0] + descendingPeople.size());
  }
  Array<PersonRecord> shuffledPeople = people;
  std::mt19937 personOrderRandom(42);
  if (shuffledPeople.size() > 1) {
    std::shuffle(&shuffledPeople[0], &shuffledPeople[0] + shuffledPeople.size(), personOrderRandom);
  }
  auto measureBuild = [&](const Array<PersonRecord>& orderedPeople) {
    return averageNanoseconds([&] {
      PersonIndex built;
      for (size_t i = 0; i < orderedPeople.size(); ++i) built.add(orderedPeople[i]);
      consumeBenchmarkResult(orderedPeople.size());
    }, 10);
  };
  const double ascendingBuildTime = measureBuild(ascendingPeople);
  const double descendingBuildTime = measureBuild(descendingPeople);
  const double shuffledBuildTime = measureBuild(shuffledPeople);
  std::cout << "Построение индексов, нс (год по возрастанию / убыванию / перемешано): "
            << ascendingBuildTime << " / " << descendingBuildTime << " / " << shuffledBuildTime << '\n';
  std::ofstream buildCsv("person_index_build.csv");
  buildCsv << "input_order,elapsed_nanoseconds,record_count\n"
           << "ascending," << ascendingBuildTime << ',' << people.size() << '\n'
           << "descending," << descendingBuildTime << ',' << people.size() << '\n'
           << "shuffled," << shuffledBuildTime << ',' << people.size() << '\n';

  std::cout << "Составной поиск (firstName lastName birthYear): ";
  std::string firstName, lastName;
  int birthYear;
  std::cin >> firstName >> lastName >> birthYear;
  const auto compositeMatches = index.findComposite(firstName, lastName, birthYear);
  std::cout << "Найдено записей: " << compositeMatches.size() << '\n';
  for (size_t i = 0; i < compositeMatches.size(); ++i) {
    const PersonRecord& person = compositeMatches[i];
    std::cout << person.id << ' ' << person.firstName << ' ' << person.lastName << ' ' << person.birthYear << '\n';
  }

  int firstYear, lastYear;
  std::cout << "Диапазон годов рождения (от до): ";
  std::cin >> firstYear >> lastYear;
  std::cout << "1 — индексированное дерево, 2 — полный просмотр, 3 — сравнить: ";
  int algorithm;
  std::cin >> algorithm;
  if (algorithm < 1 || algorithm > 3) throw std::invalid_argument("Неизвестный алгоритм поиска");
  Array<PersonRecord> indexedMatches;
  Array<PersonRecord> linearMatches;
  if (algorithm == 1 || algorithm == 3) indexedMatches = index.findBirthYearRange(firstYear, lastYear);
  if (algorithm == 2 || algorithm == 3) {
    for (size_t i = 0; i < people.size(); ++i) {
      const PersonRecord& person = people[i];
      if (person.birthYear >= firstYear && person.birthYear <= lastYear) linearMatches.push_back(person);
    }
  }
  auto printPeople = [](const Array<PersonRecord>& records) {
    for (size_t i = 0; i < records.size(); ++i) {
      const PersonRecord& person = records[i];
      std::cout << person.id << ' ' << person.firstName << ' ' << person.lastName << ' '
                << person.birthYear << '\n';
    }
  };
  if (algorithm == 1 || algorithm == 3) {
    std::cout << "Результат индексированного поиска:\n";
    printPeople(indexedMatches);
  }
  if (algorithm == 2 || algorithm == 3) {
    std::cout << "Результат полного просмотра:\n";
    printPeople(linearMatches);
  }
  if (algorithm == 1 || algorithm == 3) {
    const double indexTime = averageNanoseconds([&] {
      consumeBenchmarkResult(index.findBirthYearRange(firstYear, lastYear).size());
    });
    std::cout << "Среднее время индексированного поиска, нс: " << indexTime << '\n';
  }
  if (algorithm == 2 || algorithm == 3) {
    const double scanTime = averageNanoseconds([&] {
      size_t matches = 0;
      for (size_t i = 0; i < people.size(); ++i) {
        matches += people[i].birthYear >= firstYear && people[i].birthYear <= lastYear;
      }
      consumeBenchmarkResult(matches);
    });
    std::cout << "Среднее время полного просмотра, нс: " << scanTime << '\n';
  }
  std::cout << "Совпадений: индекс=" << indexedMatches.size() << ", полный просмотр=" << linearMatches.size() << '\n';
  if (algorithm == 3) {
    Array<int> indexedIds, linearIds;
    for (size_t i = 0; i < indexedMatches.size(); ++i) indexedIds.push_back(indexedMatches[i].id);
    for (size_t i = 0; i < linearMatches.size(); ++i) linearIds.push_back(linearMatches[i].id);
    if (indexedIds.size() > 1) std::sort(&indexedIds[0], &indexedIds[0] + indexedIds.size());
    if (linearIds.size() > 1) std::sort(&linearIds[0], &linearIds[0] + linearIds.size());
    bool sameIds = indexedIds.size() == linearIds.size();
    for (size_t i = 0; sameIds && i < indexedIds.size(); ++i) sameIds = indexedIds[i] == linearIds[i];
    std::cout << "Результаты совпадают: " << (sameIds ? "да" : "нет") << '\n';
  }
  std::ofstream csv("person_range.csv");
  csv << "id,first_name,last_name,birth_year\n";
  const auto& exportedMatches = algorithm == 2 ? linearMatches : indexedMatches;
  for (size_t i = 0; i < exportedMatches.size(); ++i) {
    const PersonRecord& person = exportedMatches[i];
    csv << person.id << ',' << csvField(person.firstName) << ',' << csvField(person.lastName) << ','
        << person.birthYear << '\n';
  }
  std::cout << "Результат сохранён в person_range.csv\n";
}

template<typename Weight>
void displayPath(const std::string& name, const PathResult<int, Weight>& result) {
  std::cout << name << ": ";
  if (!result.found) {
    std::cout << "путь не найден\n";
    return;
  }
  std::cout << "расстояние " << result.distance << ", путь ";
  for (size_t i = 0; i < result.path.size(); ++i) {
    if (i != 0) std::cout << " -> ";
    std::cout << result.path[i];
  }
  std::cout << '\n';
}

void displayGraph(const Graph<int, int>& graph) {
  std::cout << "Граф: " << graph.vertexCount() << " вершин, " << graph.edgeCount() << " рёбер\n";
  const Array<int> vertices = graph.vertices();
  for (size_t i = 0; i < vertices.size(); ++i) {
    const int vertex = vertices[i];
    std::cout << vertex << ": ";
    const auto& edges = graph.neighbors(vertex);
    for (size_t j = 0; j < edges.size(); ++j) {
      const auto& [neighbor, weight] = edges[j];
      std::cout << neighbor << '(' << weight << ") ";
    }
    std::cout << '\n';
  }
}

void runGraphLab() {
  std::cout << "Тип графа: 1 — неориентированный, 2 — ориентированный: ";
  int graphKind;
  std::cin >> graphKind;
  if (graphKind != 1 && graphKind != 2) throw std::invalid_argument("Неизвестный тип графа");
  const char* graphType = graphKind == 1 ? "undirected" : "directed";
  std::cout << "1 — ручной ввод, 2 — генерация связного графа: ";
  int mode;
  std::cin >> mode;
  std::unique_ptr<Graph<int, int>> graph;
  if (mode == 1) {
    if (graphKind == 1) graph = std::make_unique<UndirectedGraph<int, int>>();
    else graph = std::make_unique<DirectedGraph<int, int>>();
    size_t vertices, edges;
    std::cout << "Число вершин и рёбер: ";
    std::cin >> vertices >> edges;
    for (size_t i = 0; i < vertices; ++i) graph->addVertex(static_cast<int>(i));
    std::cout << "Введите рёбра: from to weight\n";
    for (size_t i = 0; i < edges; ++i) {
      int from, to, weight;
      std::cin >> from >> to >> weight;
      graph->addEdge(from, to, weight);
    }
  } else if (mode == 2) {
    size_t vertices, extraEdges;
    unsigned int seed;
    std::cout << "Число вершин, дополнительных рёбер, seed: ";
    std::cin >> vertices >> extraEdges >> seed;
    if (graphKind == 1) {
      graph = std::make_unique<UndirectedGraph<int, int>>(
          generateConnectedGraph<int, int>(vertices, extraEdges, 1, 20, seed));
    } else {
      graph = std::make_unique<DirectedGraph<int, int>>(
          generateStronglyConnectedDirectedGraph<int, int>(vertices, extraEdges, 1, 20, seed));
    }
  } else {
    throw std::invalid_argument("Неизвестный режим ввода");
  }

  displayGraph(*graph);
  int source, target;
  std::cout << "Начальная и конечная вершины: ";
  std::cin >> source >> target;
  std::cout << "1 — алгоритм Дейкстры, 2 — Беллман-Форд, 3 — сравнить: ";
  int algorithm;
  std::cin >> algorithm;
  if (algorithm < 1 || algorithm > 3) throw std::invalid_argument("Неизвестный алгоритм поиска");
  PathResult<int, int> dijkstraResult;
  PathResult<int, int> bellmanResult;
  if (algorithm == 1 || algorithm == 3) dijkstraResult = dijkstra(*graph, source, target);
  if (algorithm == 2 || algorithm == 3) bellmanResult = bellmanFord(*graph, source, target);
  if (algorithm == 1 || algorithm == 3) displayPath("Дейкстра", dijkstraResult);
  if (algorithm == 2 || algorithm == 3) displayPath("Беллман-Форд", bellmanResult);
  if (algorithm == 3) {
    const bool sameResult = dijkstraResult.found == bellmanResult.found &&
                            (!dijkstraResult.found || dijkstraResult.distance == bellmanResult.distance);
    std::cout << "Кратчайшие расстояния совпадают: " << (sameResult ? "да" : "нет") << '\n';
  }
  double dijkstraTime = 0;
  double bellmanTime = 0;
  if (algorithm == 1 || algorithm == 3) {
    dijkstraTime = averageNanoseconds([&] {
      const auto result = dijkstra(*graph, source, target);
      consumeBenchmarkResult(result.path.size() + (result.found ? static_cast<size_t>(result.distance) : 0));
    }, 100);
    std::cout << "Среднее время Дейкстры на этом графе, нс: " << dijkstraTime << '\n';
  }
  if (algorithm == 2 || algorithm == 3) {
    bellmanTime = averageNanoseconds([&] {
      const auto result = bellmanFord(*graph, source, target);
      consumeBenchmarkResult(result.path.size() + (result.found ? static_cast<size_t>(result.distance) : 0));
    }, 100);
    std::cout << "Среднее время Беллмана-Форда на этом графе, нс: " << bellmanTime << '\n';
  }

  std::ofstream csv("shortest_path_comparison.csv");
  csv << "graph_type,algorithm,elapsed_nanoseconds,vertices,edges,source,target,distance\n";
  if (algorithm == 1 || algorithm == 3) {
    csv << graphType << ",Dijkstra," << dijkstraTime << ',' << graph->vertexCount() << ',' << graph->edgeCount() << ','
        << source << ',' << target << ',' << (dijkstraResult.found ? std::to_string(dijkstraResult.distance) : "") << '\n';
  }
  if (algorithm == 2 || algorithm == 3) {
    csv << graphType << ",Bellman-Ford," << bellmanTime << ',' << graph->vertexCount() << ',' << graph->edgeCount() << ','
        << source << ',' << target << ',' << (bellmanResult.found ? std::to_string(bellmanResult.distance) : "") << '\n';
  }
  std::cout << "Сравнение сохранено в shortest_path_comparison.csv\n";
}

void runSecondLab() {
  std::cout << "ЛР-2: 1 — алфавитный указатель, 2 — индексы записей: ";
  int action;
  std::cin >> action;
  if (action == 1) runAlphabeticalIndex();
  else if (action == 2) runPersonIndex();
  else throw std::invalid_argument("Неизвестный режим ЛР-2");
}
}

int main() {
  std::cout << "Лабораторные работы по структурам и поиску\n";
  while (true) {
    std::cout << "\n1 — ЛР-2, 2 — ЛР-3, 0 — выход\nВыбор: ";
    int choice;
    if (!(std::cin >> choice) || choice == 0) return 0;
    try {
      if (choice == 1) runSecondLab();
      else if (choice == 2) runGraphLab();
      else std::cout << "Неизвестный пункт меню\n";
    } catch (const std::exception& error) {
      std::cout << "Ошибка: " << error.what() << '\n';
    }
  }
}
