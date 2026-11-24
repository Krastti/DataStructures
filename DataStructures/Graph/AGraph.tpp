#pragma once

template<typename Vertex, typename Weight>
void AGraph<Vertex, Weight>::addVertex(const Vertex& vertex) {
  if (!storage.containsKey(vertex)) storage.insert(vertex, Array<Edge>{});
}

template<typename Vertex, typename Weight>
bool AGraph<Vertex, Weight>::upsertEdge(const Vertex& from, const Vertex& to, const Weight& weight) {
  Array<Edge>& edges = storage.at(from);

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
bool AGraph<Vertex, Weight>::containsVertex(const Vertex& vertex) const {
  return storage.containsKey(vertex);
}

template<typename Vertex, typename Weight>
const Array<typename AGraph<Vertex, Weight>::Edge>&
AGraph<Vertex, Weight>::neighbors(const Vertex& vertex) const {
  return storage.at(vertex);
}

template<typename Vertex, typename Weight>
Array<Vertex> AGraph<Vertex, Weight>::vertices() const {
  return storage.keys();
}

template<typename Vertex, typename Weight>
size_t AGraph<Vertex, Weight>::vertexCount() const {
  return storage.size();
}

template<typename Vertex, typename Weight>
size_t AGraph<Vertex, Weight>::edgeCount() const {
  return edgesCount;
}
