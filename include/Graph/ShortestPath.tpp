#pragma once

#include <algorithm>
#include <random>
#include <stdexcept>
#include <type_traits>

namespace {
template<typename T, typename Compare>
class ArrayMinHeap {
private:
  Array<T> values;
  size_t activeCount = 0;
  Compare compare;

  void siftUp(size_t index) {
    while (index > 0) {
      const size_t parent = (index - 1) / 2;
      if (!compare(values[index], values[parent])) break;
      std::swap(values[index], values[parent]);
      index = parent;
    }
  }

  void siftDown(size_t index) {
    while (true) {
      const size_t left = index * 2 + 1;
      const size_t right = left + 1;
      size_t smallest = index;
      if (left < activeCount && compare(values[left], values[smallest])) smallest = left;
      if (right < activeCount && compare(values[right], values[smallest])) smallest = right;
      if (smallest == index) break;
      std::swap(values[index], values[smallest]);
      index = smallest;
    }
  }

public:
  [[nodiscard]] bool empty() const noexcept { return activeCount == 0; }

  void push(T value) {
    if (activeCount == values.size()) values.push_back(std::move(value));
    else values[activeCount] = std::move(value);
    siftUp(activeCount);
    ++activeCount;
  }

  T pop() {
    if (activeCount == 0) throw std::out_of_range("Cannot pop from an empty heap");
    T result = std::move(values[0]);
    --activeCount;
    if (activeCount > 0) {
      values[0] = std::move(values[activeCount]);
      siftDown(0);
    }
    return result;
  }
};

template<typename Weight, typename Vertex>
struct ShortestStateOrder {
  bool operator()(const pair<Weight, Vertex>& left,
                  const pair<Weight, Vertex>& right) const {
    if (left.first < right.first) return true;
    if (right.first < left.first) return false;
    return left.second < right.second;
  }
};

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
    const Map<Vertex, std::optional<Weight>>& distances,
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
PathResult<Vertex, Weight> dijkstra(const Graph<Vertex, Weight>& graph,
                                    const Vertex& source, const Vertex& target) {
  if (!graph.containsVertex(source) || !graph.containsVertex(target)) {
    throw std::invalid_argument("Source and target vertices must exist");
  }
  const Array<Vertex> vertices = graph.vertices();
  for (size_t i = 0; i < vertices.size(); ++i) {
    const Vertex& vertex = vertices[i];
    const auto& edges = graph.neighbors(vertex);
    for (size_t j = 0; j < edges.size(); ++j) {
      const auto& [neighbor, weight] = edges[j];
      (void)neighbor;
      if (weight < Weight{}) throw std::invalid_argument("Dijkstra requires non-negative edge weights");
    }
  }

  Map<Vertex, std::optional<Weight>> distances;
  Map<Vertex, Vertex> previous;
  for (size_t i = 0; i < vertices.size(); ++i) distances.insert(vertices[i], std::nullopt);
  distances.at(source) = Weight{};

  using State = pair<Weight, Vertex>;
  ArrayMinHeap<State, ShortestStateOrder<Weight, Vertex>> pending;
  pending.push(State{Weight{}, source});
  while (!pending.empty()) {
    auto [distance, vertex] = pending.pop();
    if (!distances.at(vertex).has_value() || distance != *distances.at(vertex)) continue;
    if (vertex == target) break;
    const auto& edges = graph.neighbors(vertex);
    for (size_t j = 0; j < edges.size(); ++j) {
      const auto& [neighbor, weight] = edges[j];
      const Weight candidate = distance + weight;
      if (!distances.at(neighbor).has_value() || candidate < *distances.at(neighbor)) {
        distances.at(neighbor) = candidate;
        if (previous.containsKey(neighbor)) previous.replace(neighbor, vertex);
        else previous.insert(neighbor, vertex);
        pending.push(State{candidate, neighbor});
      }
    }
  }
  return restorePath(source, target, distances, previous);
}

template<typename Vertex, typename Weight>
PathResult<Vertex, Weight> bellmanFord(const Graph<Vertex, Weight>& graph,
                                      const Vertex& source, const Vertex& target) {
  if (!graph.containsVertex(source) || !graph.containsVertex(target)) {
    throw std::invalid_argument("Source and target vertices must exist");
  }
  const Array<Vertex> vertices = graph.vertices();
  Map<Vertex, std::optional<Weight>> distances;
  Map<Vertex, Vertex> previous;
  for (size_t i = 0; i < vertices.size(); ++i) distances.insert(vertices[i], std::nullopt);
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
        throw std::domain_error("A reachable negative cycle exists");
      }
    }
  }
  return restorePath(source, target, distances, previous);
}

template<typename Vertex, typename Weight>
Graph<Vertex, Weight> generateConnectedGraph(size_t vertexCount, size_t extraEdges,
                                            Weight minWeight, Weight maxWeight,
                                            unsigned int seed) {
  if (vertexCount == 0) throw std::invalid_argument("A graph must contain at least one vertex");
  if (vertexCount > 2000) throw std::invalid_argument("The generator is limited to 2000 vertices");
  if (minWeight < Weight{} || maxWeight < minWeight) {
    throw std::invalid_argument("Weights must form a non-negative range");
  }
  static_assert(std::is_arithmetic_v<Weight>, "The graph generator requires an arithmetic weight type");
  const size_t possibleEdges = vertexCount * (vertexCount - 1) / 2;
  if (extraEdges > possibleEdges - (vertexCount - 1)) {
    throw std::invalid_argument("Too many extra edges for a simple connected graph");
  }

  Graph<Vertex, Weight> graph;
  for (size_t i = 0; i < vertexCount; ++i) graph.addVertex(static_cast<Vertex>(i));
  std::mt19937 random(seed);
  for (size_t i = 1; i < vertexCount; ++i) {
    graph.addEdge(static_cast<Vertex>(i - 1), static_cast<Vertex>(i),
                  generateWeight(random, minWeight, maxWeight));
  }

  Array<pair<size_t, size_t>> candidates;
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
