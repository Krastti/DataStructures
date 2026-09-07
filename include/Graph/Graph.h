#ifndef DATASTRUCTURES_GRAPH_H
#define DATASTRUCTURES_GRAPH_H

#include "../Trees/RedBlackTree.h"

template <typename T>
class Graph {
private:
  Map<T, T> nestedTree;
  Map<T, T> vertexMap;
public:
  //

  // 1, [2,3,4] -> (1,2), (1,3), (1,4)
  // 2, [1,4] -> (1,2), (2,4) -> (2,4)
  // 3, [1] ->
  // 4, [1,2]

  Graph() = default;

  ~Graph() = default;
};

#endif // DATASTRUCTURES_GRAPH_H
