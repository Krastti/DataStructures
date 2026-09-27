#ifndef DATASTRUCTURES_SHORTEST_PATH_H
#define DATASTRUCTURES_SHORTEST_PATH_H

#include "Graph.h"
#include <optional>

template<typename Vertex, typename Weight>
struct PathResult {
  bool found = false;
  Weight distance{};
  Array<Vertex> path;
};

template<typename Vertex, typename Weight>
PathResult<Vertex, Weight> dijkstra(const Graph<Vertex, Weight>& graph,
                                    const Vertex& source, const Vertex& target);

template<typename Vertex, typename Weight>
PathResult<Vertex, Weight> bellmanFord(const Graph<Vertex, Weight>& graph,
                                      const Vertex& source, const Vertex& target);

template<typename Vertex, typename Weight>
Graph<Vertex, Weight> generateConnectedGraph(size_t vertexCount, size_t extraEdges,
                                            Weight minWeight, Weight maxWeight,
                                            unsigned int seed = 1);

#include "ShortestPath.tpp"

#endif // DATASTRUCTURES_SHORTEST_PATH_H
