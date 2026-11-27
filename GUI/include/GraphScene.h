#pragma once

#include "Common.h"
#include "../../Labs/ThirdLab/headers/algoritms/ShortestPath.h"
#include <cmath>
#include <map>
#include <memory>

class GraphScene : public QGraphicsScene {
public:
  using QGraphicsScene::QGraphicsScene;
  void display(const Graph<int, int>& graph, bool directed, const Array<int>& path);
};

class GraphView : public QGraphicsView {
public:
  using QGraphicsView::QGraphicsView;
protected:
  void wheelEvent(QWheelEvent* event) override {
    const double factor = event->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15;
    const double next = transform().m11() * factor;
    if (next >= 0.05 && next <= 20) scale(factor, factor);
    event->accept();
  }
};
