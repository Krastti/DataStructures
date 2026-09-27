#include "../../include/Graph/ShortestPath.h"
#include "../Array/ArrayTestHelpers.h"
#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <type_traits>

TEST(ShortestPathTests, CustomPairStoresValuesAndSupportsStructuredBindings) {
  Pair<std::string, int> value{"vertex", 7};
  auto& [vertex, weight] = value;
  weight = 9;
  EXPECT_EQ(vertex, "vertex");
  EXPECT_EQ(value.second, 9);
  static_assert(std::is_abstract_v<Graph<int, int>>);
  static_assert(std::is_same_v<decltype(UndirectedGraph<int, int>{}.neighbors(0)[0]), const Pair<int, int>&>);
}

TEST(ShortestPathTests, DijkstraAndBellmanFordReturnSameShortestPath) {
  UndirectedGraph<int, int> graph;
  graph.addEdge(0, 1, 4);
  graph.addEdge(0, 2, 1);
  graph.addEdge(2, 1, 2);
  graph.addEdge(1, 3, 1);
  graph.addEdge(2, 3, 5);

  const auto dijkstraResult = dijkstra(graph, 0, 3);
  const auto bellmanResult = bellmanFord(graph, 0, 3);
  EXPECT_TRUE(dijkstraResult.found);
  EXPECT_EQ(dijkstraResult.distance, 4);
  EXPECT_EQ(toVector(dijkstraResult.path), (std::vector<int>{0, 2, 1, 3}));
  EXPECT_EQ(bellmanResult.distance, dijkstraResult.distance);
  EXPECT_EQ(toVector(bellmanResult.path), toVector(dijkstraResult.path));
}

TEST(ShortestPathTests, ReportsUnreachableVerticesAndValidatesInputs) {
  UndirectedGraph<int, int> graph;
  graph.addVertex(1);
  graph.addVertex(2);
  EXPECT_FALSE(dijkstra(graph, 1, 2).found);
  EXPECT_THROW(dijkstra(graph, 1, 3), std::invalid_argument);

  graph.addEdge(1, 2, -1);
  EXPECT_THROW(dijkstra(graph, 1, 2), std::invalid_argument);
  EXPECT_THROW(bellmanFord(graph, 1, 2), std::domain_error);
}

TEST(ShortestPathTests, HandlesSourceEqualToTarget) {
  UndirectedGraph<int, int> graph;
  graph.addVertex(5);
  const auto result = dijkstra(graph, 5, 5);
  EXPECT_TRUE(result.found);
  EXPECT_EQ(result.distance, 0);
  EXPECT_EQ(toVector(result.path), (std::vector<int>{5}));
}

TEST(ShortestPathTests, CountsSelfLoopsAndUpdatesRepeatedEdges) {
  UndirectedGraph<int, int> graph;
  graph.addEdge(1, 1, 3);
  graph.addEdge(1, 2, 4);
  graph.addEdge(1, 2, 7);

  EXPECT_EQ(graph.edgeCount(), 2);
  ASSERT_EQ(graph.neighbors(1).size(), 2);
  EXPECT_EQ(graph.neighbors(1)[1].second, 7);
  EXPECT_EQ(graph.neighbors(2)[0].second, 7);
}

TEST(GraphTests, UndirectedOperationsWorkThroughInterface) {
  UndirectedGraph<int, int> concrete;
  Graph<int, int>& graph = concrete;
  graph.addVertex(4);
  graph.addVertex(4);
  graph.addEdge(1, 2, 3);
  graph.addEdge(2, 1, 8);
  graph.addEdge(2, 2, 5);
  EXPECT_EQ(graph.vertexCount(), 3);
  EXPECT_EQ(graph.edgeCount(), 2);
  EXPECT_TRUE(graph.containsVertex(4));
  EXPECT_EQ(graph.neighbors(1)[0].second, 8);
  EXPECT_EQ(graph.neighbors(2)[0].second, 8);
  EXPECT_EQ(graph.neighbors(2).size(), 2);
}

TEST(GraphTests, DirectedEdgesHaveIndependentDirectionsAndCounts) {
  DirectedGraph<int, int> concrete;
  Graph<int, int>& graph = concrete;
  graph.addEdge(1, 2, 3);
  EXPECT_EQ(graph.vertexCount(), 2);
  EXPECT_EQ(graph.edgeCount(), 1);
  EXPECT_EQ(graph.neighbors(2).size(), 0);
  graph.addEdge(1, 2, 4);
  graph.addEdge(2, 1, 7);
  graph.addEdge(2, 2, 9);
  graph.addEdge(2, 2, 10);
  EXPECT_EQ(graph.edgeCount(), 3);
  EXPECT_EQ(graph.neighbors(1)[0].second, 4);
  EXPECT_EQ(graph.neighbors(2)[0].second, 7);
  EXPECT_EQ(graph.neighbors(2)[1].second, 10);
}

TEST(ShortestPathTests, DirectedShortestPathsRespectDirection) {
  DirectedGraph<int, int> graph;
  graph.addEdge(0, 1, 2);
  graph.addEdge(1, 2, 3);
  graph.addEdge(0, 2, 9);
  const Graph<int, int>& view = graph;
  EXPECT_EQ(dijkstra(view, 0, 2).distance, 5);
  EXPECT_EQ(toVector(bellmanFord(view, 0, 2).path), (std::vector<int>{0, 1, 2}));
  EXPECT_FALSE(dijkstra(view, 2, 0).found);
  EXPECT_FALSE(bellmanFord(view, 2, 0).found);
}

TEST(ShortestPathTests, BellmanFordHandlesNegativeDirectedEdgesAndCycles) {
  DirectedGraph<int, int> graph;
  graph.addEdge(0, 1, 4);
  graph.addEdge(0, 2, 5);
  graph.addEdge(1, 2, -2);
  const auto result = bellmanFord(graph, 0, 2);
  EXPECT_EQ(result.distance, 2);
  EXPECT_EQ(toVector(result.path), (std::vector<int>{0, 1, 2}));
  EXPECT_THROW(dijkstra(graph, 0, 2), std::invalid_argument);
  graph.addEdge(2, 1, -3);
  EXPECT_THROW(bellmanFord(graph, 0, 2), std::domain_error);
}

TEST(ShortestPathTests, GeneratesConnectedSimpleGraphDeterministically) {
  const auto graph = generateConnectedGraph<int, int>(8, 4, 1, 9, 42);
  EXPECT_EQ(graph.vertexCount(), 8);
  EXPECT_EQ(graph.edgeCount(), 11);
  EXPECT_TRUE(dijkstra(graph, 0, 7).found);
  const auto floatingGraph = generateConnectedGraph<int, double>(4, 1, 0.5, 2.0, 7);
  EXPECT_TRUE(dijkstra(floatingGraph, 0, 3).found);
  EXPECT_THROW((generateConnectedGraph<int, int>(0, 0, 1, 2)), std::invalid_argument);
  EXPECT_THROW((generateConnectedGraph<int, int>(3, 2, 1, 2)), std::invalid_argument);
}

TEST(GraphTests, DirectedGeneratorIsStronglyConnectedAndDeterministic) {
  const auto first = generateStronglyConnectedDirectedGraph<int, int>(6, 5, 1, 9, 42);
  const auto second = generateStronglyConnectedDirectedGraph<int, int>(6, 5, 1, 9, 42);
  EXPECT_EQ(first.vertexCount(), 6);
  EXPECT_EQ(first.edgeCount(), 11);
  for (int from = 0; from < 6; ++from) {
    const auto& firstEdges = first.neighbors(from);
    const auto& secondEdges = second.neighbors(from);
    ASSERT_EQ(firstEdges.size(), secondEdges.size());
    for (size_t i = 0; i < firstEdges.size(); ++i) {
      EXPECT_EQ(firstEdges[i].first, secondEdges[i].first);
      EXPECT_EQ(firstEdges[i].second, secondEdges[i].second);
    }
    for (int to = 0; to < 6; ++to) EXPECT_TRUE(dijkstra(first, from, to).found);
  }
  const auto singleton = generateStronglyConnectedDirectedGraph<int, int>(1, 0, 1, 9);
  EXPECT_EQ(singleton.vertexCount(), 1);
  EXPECT_EQ(singleton.edgeCount(), 0);
  EXPECT_THROW((generateStronglyConnectedDirectedGraph<int, int>(0, 0, 1, 9)), std::invalid_argument);
  EXPECT_THROW((generateStronglyConnectedDirectedGraph<int, int>(2, 1, 1, 9)), std::invalid_argument);
}
