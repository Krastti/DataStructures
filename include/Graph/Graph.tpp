#pragma once

#include <stdexcept>

template<typename Vertex, typename Weight>
void Graph<Vertex, Weight>::addVertex(const Vertex& vertex) {
  if (!adjacency.containsKey(vertex)) adjacency.insert(vertex, Array<Edge>{});
}

template<typename Vertex, typename Weight>
void Graph<Vertex, Weight>::upsertEdge(const Vertex& from, const Vertex& to, const Weight& weight) {
  Array<Edge>& edges = adjacency.at(from);
  for (size_t i = 0; i < edges.size(); ++i) {
    Edge& edge = edges[i];
    if (edge.first == to) {
      edge.second = weight;
      return;
    }
  }
  edges.push_back(Edge{to, weight});
}

template<typename Vertex, typename Weight>
void Graph<Vertex, Weight>::addEdge(const Vertex& from, const Vertex& to, const Weight& weight) {
  addVertex(from);
  addVertex(to);
  upsertEdge(from, to, weight);
  if (from != to) upsertEdge(to, from, weight);
}

template<typename Vertex, typename Weight>
bool Graph<Vertex, Weight>::containsVertex(const Vertex& vertex) const {
  return adjacency.containsKey(vertex);
}

template<typename Vertex, typename Weight>
const Array<typename Graph<Vertex, Weight>::Edge>& Graph<Vertex, Weight>::neighbors(const Vertex& vertex) const {
  return adjacency.at(vertex);
}

template<typename Vertex, typename Weight>
Array<Vertex> Graph<Vertex, Weight>::vertices() const {
  return adjacency.keys();
}

template<typename Vertex, typename Weight>
size_t Graph<Vertex, Weight>::vertexCount() const {
  return adjacency.size();
}

template<typename Vertex, typename Weight>
size_t Graph<Vertex, Weight>::edgeCount() const {
  size_t count = 0;
  const Array<Vertex> allVertices = vertices();
  for (size_t i = 0; i < allVertices.size(); ++i) {
    const Vertex& vertex = allVertices[i];
    const Array<Edge>& edges = neighbors(vertex);
    for (size_t j = 0; j < edges.size(); ++j) {
      const auto& [neighbor, weight] = edges[j];
      (void)weight;
      if (!(neighbor < vertex)) ++count;
    }
  }
  return count;
}
