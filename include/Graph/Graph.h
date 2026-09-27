#ifndef DATASTRUCTURES_GRAPH_H
#define DATASTRUCTURES_GRAPH_H

#include "../Array/Array.h"
#include <cstddef>

template<typename First, typename Second>
struct Pair {
  First first;
  Second second;
};

template<typename Vertex, typename Weight>
class Graph {
public:
  using Edge = Pair<Vertex, Weight>;
  virtual ~Graph() = default;
  virtual void addVertex(const Vertex& vertex) = 0;
  virtual void addEdge(const Vertex& from, const Vertex& to, const Weight& weight) = 0;
  [[nodiscard]] virtual bool containsVertex(const Vertex& vertex) const = 0;
  [[nodiscard]] virtual const Array<Edge>& neighbors(const Vertex& vertex) const = 0;
  [[nodiscard]] virtual Array<Vertex> vertices() const = 0;
  [[nodiscard]] virtual size_t vertexCount() const = 0;
  [[nodiscard]] virtual size_t edgeCount() const = 0;
};

#endif // DATASTRUCTURES_GRAPH_H
