#pragma once

template<typename Vertex, typename Weight>
void AdjacencyGraph<Vertex, Weight>::addVertex(const Vertex& vertex) {
  if (!adjacency.containsKey(vertex)) adjacency.insert(vertex, Array<Edge>{});
}

template<typename Vertex, typename Weight>
bool AdjacencyGraph<Vertex, Weight>::upsertEdge(const Vertex& from, const Vertex& to, const Weight& weight) {
  Array<Edge>& edges = adjacency.at(from);

  for (size_t i = 0; i < edges.size(); ++i) {
    Edge& edge = edges[i];
    if (edge.first == to) {
      edge.second = weight;
      return false;
    }
  }
  edges.push_back(Edge{to, weight});
  return true;
}

template<typename Vertex, typename Weight>
void UndirectedGraph<Vertex, Weight>::addEdge(const Vertex& from, const Vertex& to, const Weight& weight) {
  this->addVertex(from);
  this->addVertex(to);
  if (this->upsertEdge(from, to, weight)) ++this->edgesCount;
  if (from != to) this->upsertEdge(to, from, weight);
}

template<typename Vertex, typename Weight>
void DirectedGraph<Vertex, Weight>::addEdge(const Vertex& from, const Vertex& to, const Weight& weight) {
  this->addVertex(from);
  this->addVertex(to);
  if (this->upsertEdge(from, to, weight)) ++this->edgesCount;
}

template<typename Vertex, typename Weight>
bool AdjacencyGraph<Vertex, Weight>::containsVertex(const Vertex& vertex) const {
  return adjacency.containsKey(vertex);
}

template<typename Vertex, typename Weight>
const Array<typename AdjacencyGraph<Vertex, Weight>::Edge>&
AdjacencyGraph<Vertex, Weight>::neighbors(const Vertex& vertex) const {
  return adjacency.at(vertex);
}

template<typename Vertex, typename Weight>
Array<Vertex> AdjacencyGraph<Vertex, Weight>::vertices() const {
  return adjacency.keys();
}

template<typename Vertex, typename Weight>
size_t AdjacencyGraph<Vertex, Weight>::vertexCount() const {
  return adjacency.size();
}

template<typename Vertex, typename Weight>
size_t AdjacencyGraph<Vertex, Weight>::edgeCount() const {
  return edgesCount;
}
