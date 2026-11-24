#ifndef DATASTRUCTURES_AGRAPH_H
#define DATASTRUCTURES_AGRAPH_H

#include "Graph.h"
#include "../../FirstLab/headers/RedBlackTree.h"

template<typename Vertex, typename Weight>
class AGraph : public Graph<Vertex, Weight> {
protected:
  using Edge = Graph<Vertex, Weight>::Edge;

  Map<Vertex, Array<Edge>> storage;
  size_t edgesCount = 0;

  // Вставка записи о ребре или обновление веса. Возвращает true при вставке.
  bool upsertEdge(const Vertex& from, const Vertex& to, const Weight& weight);

public:
  // Добавление вершины, если её ещё нет в графе
  void addVertex(const Vertex& vertex) override;

  // Проверка наличия вершины
  [[nodiscard]] bool containsVertex(const Vertex& vertex) const override;

  // Список рёбер, исходящих из вершины
  [[nodiscard]] const Array<Edge>& neighbors(const Vertex& vertex) const override;

  // Список всех вершин
  [[nodiscard]] Array<Vertex> vertices() const override;

  // Количество вершин
  [[nodiscard]] size_t vertexCount() const override;

  // Количество рёбер графа
  [[nodiscard]] size_t edgeCount() const override;
};

template<typename Vertex, typename Weight>
class UndirectedGraph final : public AGraph<Vertex, Weight> {
public:
  // Добавление ребра в обоих направлениях. Петля записывается один раз.
  void addEdge(const Vertex& from, const Vertex& to, const Weight& weight) override;
};

template<typename Vertex, typename Weight>
class DirectedGraph final : public AGraph<Vertex, Weight> {
public:
  // Добавление ребра от from к to
  void addEdge(const Vertex& from, const Vertex& to, const Weight& weight) override;
};

#include "AGraph.tpp"

#endif // DATASTRUCTURES_AGRAPH_H
