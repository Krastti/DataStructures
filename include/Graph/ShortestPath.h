#ifndef DATASTRUCTURES_SHORTEST_PATH_H
#define DATASTRUCTURES_SHORTEST_PATH_H

#include "AdjacencyGraph.h"
#include "../Optional/Optional.h"

template<typename Vertex, typename Weight>
struct PathResult {
  bool found = false;
  Weight distance{};
  Array<Vertex> path;
};

template<typename Vertex, typename Weight>
PathResult<Vertex, Weight> dijkstra(const Graph<Vertex,
                                    Weight>& graph,
                                    const Vertex& source,
                                    const Vertex& target);

template<typename Vertex, typename Weight>
PathResult<Vertex, Weight> bellmanFord(const Graph<Vertex, Weight>& graph,
                                      const Vertex& source, const Vertex& target);

template<typename Vertex, typename Weight>
UndirectedGraph<Vertex, Weight> generateConnectedGraph(size_t vertexCount, size_t extraEdges,
                                                       Weight minWeight, Weight maxWeight,
                                                       unsigned int seed = 1);

template<typename Vertex, typename Weight>
DirectedGraph<Vertex, Weight> generateStronglyConnectedDirectedGraph(
    size_t vertexCount, size_t extraEdges, Weight minWeight, Weight maxWeight,
    unsigned int seed = 1);

#include "ShortestPath.tpp"

#endif // DATASTRUCTURES_SHORTEST_PATH_H
