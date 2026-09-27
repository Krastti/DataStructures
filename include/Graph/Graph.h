#ifndef DATASTRUCTURES_GRAPH_H
#define DATASTRUCTURES_GRAPH_H

#include "../Array/Array.h"
#include "../Trees/RedBlackTree.h"
#include <cstddef>

template<typename First, typename Second>
struct pair {
  First first;
  Second second;
};

template<typename Vertex, typename Weight>
class Graph {
private:
  using Edge = pair<Vertex, Weight>;
  Map<Vertex, Array<Edge>> adjacency;
  void upsertEdge(const Vertex& from, const Vertex& to, const Weight& weight);

public:
  void addVertex(const Vertex& vertex);
  void addEdge(const Vertex& from, const Vertex& to, const Weight& weight);
  [[nodiscard]] bool containsVertex(const Vertex& vertex) const;
  [[nodiscard]] const Array<Edge>& neighbors(const Vertex& vertex) const;
  [[nodiscard]] Array<Vertex> vertices() const;
  [[nodiscard]] size_t vertexCount() const;
  [[nodiscard]] size_t edgeCount() const;
};

#include "Graph.tpp"

#endif // DATASTRUCTURES_GRAPH_H
