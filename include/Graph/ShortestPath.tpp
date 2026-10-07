#pragma once

#include <algorithm>
#include <random>
#include <stdexcept>
#include <type_traits>

namespace {
template<typename Weight>
Weight generateWeight(std::mt19937& random, Weight minWeight, Weight maxWeight) {
  if constexpr (std::is_integral_v<Weight>) {
    return std::uniform_int_distribution<Weight>(minWeight, maxWeight)(random);
  } else {
    return std::uniform_real_distribution<Weight>(minWeight, maxWeight)(random);
  }
}

template<typename Vertex, typename Weight>
PathResult<Vertex, Weight> restorePath(
    const Vertex& source, const Vertex& target,
    const Map<Vertex, Optional<Weight>>& distances,
    const Map<Vertex, Vertex>& previous) {
  if (!distances.containsKey(target) || !distances.at(target).has_value()) return {};

  PathResult<Vertex, Weight> result;
  result.found = true;
  result.distance = *distances.at(target);
  Vertex current = target;
  result.path.push_back(current);
  while (current != source) {
    if (!previous.containsKey(current)) return {};
    current = previous.at(current);
    result.path.push_back(current);
  }
  for (size_t i = 0; i < result.path.size() / 2; ++i) {
    std::swap(result.path[i], result.path[result.path.size() - 1 - i]);
  }
  return result;
}
}

template<typename Vertex, typename Weight>
PathResult<Vertex, Weight> dijkstra(const Graph<Vertex, Weight>& graph, const Vertex& source, const Vertex& target) {

  // Если начало или конец в графе отсутсвуют, то выбросить исключение
  if (!graph.containsVertex(source) || !graph.containsVertex(target)) {
    throw std::invalid_argument("Начальная и конечная вершины должны существовать");
  }

  // Проверка, что у каждой вершины, веса у выходящих ребер не отрицательны
  const Array<Vertex> vertices = graph.vertices();
  for (size_t i = 0; i < vertices.size(); ++i) {

    const Vertex& vertex = vertices[i];
    const auto& edges = graph.neighbors(vertex);

    for (size_t j = 0; j < edges.size(); ++j) {
      const Weight& weight = edges[j].second;
      if (weight < Weight{}) throw std::invalid_argument("Алгоритм Дейкстры требует неотрицательных весов рёбер");
    }
  }

  // Начальное растояние равно нулю, а предыдущих - пустая таблица
  Map<Vertex, Optional<Weight>> distances;
  Map<Vertex, Vertex> previous;

  for (size_t i = 0; i < vertices.size(); ++i) distances.insert(vertices[i], Optional<Weight>{});
  distances.at(source) = Weight{};

  // Помечаем все вершины в графе как не посещенные
  Map<Vertex, bool> visited;
  for (size_t i = 0; i < vertices.size(); ++i) visited.insert(vertices[i], false);

  Vertex current = source;
  while (current != target) {
    visited.at(current) = true;

    const Weight distance = *distances.at(current);
    const auto& edges = graph.neighbors(current);

    for (size_t j = 0; j < edges.size(); ++j) {
      const Vertex& neighbor = edges[j].first;
      const Weight& weight = edges[j].second;
      const Weight candidate = distance + weight;

      auto& neighborDistance = distances.at(neighbor);
      if (!neighborDistance.has_value() || candidate < *neighborDistance) {
        neighborDistance = candidate;
        if (previous.containsKey(neighbor)) previous.replace(neighbor, current);
        else previous.insert(neighbor, current);
      }
    }

    // Выбираем следующую ближайшую необработанную вершину.
    Optional<Vertex> next;
    Optional<Weight> minimum;
    for (size_t i = 0; i < vertices.size(); ++i) {
      const Vertex& vertex = vertices[i];
      const auto& vertexDistance = distances.at(vertex);
      if (visited.at(vertex) || !vertexDistance.has_value()) continue;

      if (!minimum.has_value() || *vertexDistance < *minimum) {
        minimum = *vertexDistance;
        next = vertex;
      }
    }

    if (!next.has_value()) break;
    current = *next;
  }
  return restorePath(source, target, distances, previous);
}

template<typename Vertex, typename Weight>
PathResult<Vertex, Weight> bellmanFord(const Graph<Vertex, Weight>& graph, const Vertex& source, const Vertex& target) {

  if (!graph.containsVertex(source) || !graph.containsVertex(target)) {
    throw std::invalid_argument("Начальная и конечная вершины должны существовать");
  }

  const Array<Vertex> vertices = graph.vertices();
  Map<Vertex, Optional<Weight>> distances;
  Map<Vertex, Vertex> previous;

  // Цикл добавляет в distances каждую вершину и не задает ей расстояние
  for (size_t i = 0; i < vertices.size(); ++i) distances.insert(vertices[i], Optional<Weight>{});
  //
  distances.at(source) = Weight{};

  for (size_t iteration = 1; iteration < vertices.size(); ++iteration) {
    bool changed = false;
    for (size_t i = 0; i < vertices.size(); ++i) {
      const Vertex& vertex = vertices[i];
      if (!distances.at(vertex).has_value()) continue;
      const auto& edges = graph.neighbors(vertex);
      for (size_t j = 0; j < edges.size(); ++j) {
        const auto& [neighbor, weight] = edges[j];
        const Weight candidate = *distances.at(vertex) + weight;
        if (!distances.at(neighbor).has_value() || candidate < *distances.at(neighbor)) {
          distances.at(neighbor) = candidate;
          if (previous.containsKey(neighbor)) previous.replace(neighbor, vertex);
          else previous.insert(neighbor, vertex);
          changed = true;
        }
      }
    }
    if (!changed) break;
  }
  for (size_t i = 0; i < vertices.size(); ++i) {
    const Vertex& vertex = vertices[i];
    if (!distances.at(vertex).has_value()) continue;
    const auto& edges = graph.neighbors(vertex);
    for (size_t j = 0; j < edges.size(); ++j) {
      const auto& [neighbor, weight] = edges[j];
      if (!distances.at(neighbor).has_value() || *distances.at(vertex) + weight < *distances.at(neighbor)) {
        throw std::domain_error("В достижимой части графа есть цикл отрицательного веса");
      }
    }
  }
  return restorePath(source, target, distances, previous);
}

template<typename Vertex, typename Weight>
UndirectedGraph<Vertex, Weight> generateConnectedGraph(size_t vertexCount, size_t extraEdges,
                                                       Weight minWeight, Weight maxWeight,
                                                       unsigned int seed) {
  if (vertexCount == 0) throw std::invalid_argument("Граф должен содержать хотя бы одну вершину");
  if (vertexCount > 2000) throw std::invalid_argument("Генератор поддерживает не более 2000 вершин");
  if (minWeight < Weight{} || maxWeight < minWeight) {
    throw std::invalid_argument("Веса должны задавать неотрицательный диапазон");
  }
  static_assert(std::is_arithmetic_v<Weight>, "Для генерации графа нужен арифметический тип веса");
  const size_t possibleEdges = vertexCount * (vertexCount - 1) / 2;
  if (extraEdges > possibleEdges - (vertexCount - 1)) {
    throw std::invalid_argument("Слишком много дополнительных рёбер для простого связного графа");
  }

  UndirectedGraph<Vertex, Weight> graph;
  for (size_t i = 0; i < vertexCount; ++i) graph.addVertex(static_cast<Vertex>(i));
  std::mt19937 random(seed);
  for (size_t i = 1; i < vertexCount; ++i) {
    graph.addEdge(static_cast<Vertex>(i - 1), static_cast<Vertex>(i),
                  generateWeight(random, minWeight, maxWeight));
  }

  Array<Pair<size_t, size_t>> candidates;
  for (size_t from = 0; from < vertexCount; ++from) {
    for (size_t to = from + 2; to < vertexCount; ++to) candidates.push_back({from, to});
  }
  if (candidates.size() > 1) std::shuffle(&candidates[0], &candidates[0] + candidates.size(), random);
  for (size_t i = 0; i < extraEdges; ++i) {
    graph.addEdge(static_cast<Vertex>(candidates[i].first), static_cast<Vertex>(candidates[i].second),
                  generateWeight(random, minWeight, maxWeight));
  }
  return graph;
}

template<typename Vertex, typename Weight>
DirectedGraph<Vertex, Weight> generateStronglyConnectedDirectedGraph(
    size_t vertexCount, size_t extraEdges, Weight minWeight, Weight maxWeight,
    unsigned int seed) {
  if (vertexCount == 0) throw std::invalid_argument("Граф должен содержать хотя бы одну вершину");
  if (vertexCount > 2000) throw std::invalid_argument("Генератор поддерживает не более 2000 вершин");
  if (minWeight < Weight{} || maxWeight < minWeight) {
    throw std::invalid_argument("Веса должны задавать неотрицательный диапазон");
  }
  static_assert(std::is_arithmetic_v<Weight>, "Для генерации графа нужен арифметический тип веса");
  const size_t cycleEdges = vertexCount == 1 ? 0 : vertexCount;
  const size_t possibleEdges = vertexCount * (vertexCount - 1);
  if (extraEdges > possibleEdges - cycleEdges) {
    throw std::invalid_argument("Слишком много дополнительных рёбер для простого сильно связного графа");
  }

  DirectedGraph<Vertex, Weight> graph;
  for (size_t i = 0; i < vertexCount; ++i) graph.addVertex(static_cast<Vertex>(i));
  std::mt19937 random(seed);
  for (size_t i = 0; i < cycleEdges; ++i) {
    graph.addEdge(static_cast<Vertex>(i), static_cast<Vertex>((i + 1) % vertexCount),
                  generateWeight(random, minWeight, maxWeight));
  }

  Array<Pair<size_t, size_t>> candidates;
  for (size_t from = 0; from < vertexCount; ++from) {
    for (size_t to = 0; to < vertexCount; ++to) {
      if (from != to && (from + 1) % vertexCount != to) candidates.push_back({from, to});
    }
  }
  if (candidates.size() > 1) std::shuffle(&candidates[0], &candidates[0] + candidates.size(), random);
  for (size_t i = 0; i < extraEdges; ++i) {
    graph.addEdge(static_cast<Vertex>(candidates[i].first), static_cast<Vertex>(candidates[i].second),
                  generateWeight(random, minWeight, maxWeight));
  }
  return graph;
}
