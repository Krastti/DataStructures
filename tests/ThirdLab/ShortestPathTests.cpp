#include "../../include/Graph/ShortestPath.h"
#include "../Array/ArrayTestHelpers.h"
#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <type_traits>

TEST(ShortestPathTests, CustomPairStoresValuesAndSupportsStructuredBindings) {
  std_pair<std::string, int> pair{"vertex", 7};
  auto& [vertex, weight] = pair;
  weight = 9;
  EXPECT_EQ(vertex, "vertex");
  EXPECT_EQ(pair.second, 9);
  static_assert(std::is_same_v<decltype(Graph<int, int>{}.neighbors(0)[0]), const std_pair<int, int>&>);
}

TEST(ShortestPathTests, DijkstraAndBellmanFordReturnSameShortestPath) {
  Graph<int, int> graph;
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
  Graph<int, int> graph;
  graph.addVertex(1);
  graph.addVertex(2);
  EXPECT_FALSE(dijkstra(graph, 1, 2).found);
  EXPECT_THROW(dijkstra(graph, 1, 3), std::invalid_argument);

  graph.addEdge(1, 2, -1);
  EXPECT_THROW(dijkstra(graph, 1, 2), std::invalid_argument);
  EXPECT_THROW(bellmanFord(graph, 1, 2), std::domain_error);
}

TEST(ShortestPathTests, HandlesSourceEqualToTarget) {
  Graph<int, int> graph;
  graph.addVertex(5);
  const auto result = dijkstra(graph, 5, 5);
  EXPECT_TRUE(result.found);
  EXPECT_EQ(result.distance, 0);
  EXPECT_EQ(toVector(result.path), (std::vector<int>{5}));
}

TEST(ShortestPathTests, CountsSelfLoopsAndUpdatesRepeatedEdges) {
  Graph<int, int> graph;
  graph.addEdge(1, 1, 3);
  graph.addEdge(1, 2, 4);
  graph.addEdge(1, 2, 7);

  EXPECT_EQ(graph.edgeCount(), 2);
  ASSERT_EQ(graph.neighbors(1).size(), 2);
  EXPECT_EQ(graph.neighbors(1)[1].second, 7);
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
