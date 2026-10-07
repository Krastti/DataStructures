#ifndef DATASTRUCTURES_ADJACENCY_GRAPH_H
#define DATASTRUCTURES_ADJACENCY_GRAPH_H

#include "Graph.h"
#include "../Trees/RedBlackTree.h"

// TODO Придумать название получше для AdjacencyGraph

template<typename Vertex, typename Weight>
class AdjacencyGraph : public Graph<Vertex, Weight> {
protected:
  using Edge = Graph<Vertex, Weight>::Edge;

  Map<Vertex, Array<Edge>> adjacency;
  size_t edgesCount = 0;

  bool upsertEdge(const Vertex& from, const Vertex& to, const Weight& weight);

public:
  void addVertex(const Vertex& vertex) override;

  [[nodiscard]] bool containsVertex(const Vertex& vertex) const override;

  [[nodiscard]] const Array<Edge>& neighbors(const Vertex& vertex) const override;
  [[nodiscard]] Array<Vertex> vertices() const override;

  [[nodiscard]] size_t vertexCount() const override;
  [[nodiscard]] size_t edgeCount() const override;
};

template<typename Vertex, typename Weight>
class UndirectedGraph final : public AdjacencyGraph<Vertex, Weight> {
public:
  void addEdge(const Vertex& from, const Vertex& to, const Weight& weight) override;
};

template<typename Vertex, typename Weight>
class DirectedGraph final : public AdjacencyGraph<Vertex, Weight> {
public:
  void addEdge(const Vertex& from, const Vertex& to, const Weight& weight) override;
};

#include "AdjacencyGraph.tpp"

#endif // DATASTRUCTURES_ADJACENCY_GRAPH_H
