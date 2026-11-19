#include "../include/GraphScene.h"
#include <QPainterPath>
#include <QPolygonF>

namespace {
class VertexItem : public QGraphicsEllipseItem {
public:
  std::function<void()> moved;
  explicit VertexItem(int id) : QGraphicsEllipseItem(-18, -18, 36, 36) {
    setBrush(QColor("#e5efff"));
    setFlags(ItemIsMovable | ItemSendsGeometryChanges);
    setZValue(2);
    auto* label = new QGraphicsSimpleTextItem(QString::number(id), this);
    label->setPos(-label->boundingRect().width() / 2, -label->boundingRect().height() / 2);
  }
protected:
  QVariant itemChange(GraphicsItemChange change, const QVariant& value) override {
    if (change == ItemPositionHasChanged && moved) moved();
    return QGraphicsEllipseItem::itemChange(change, value);
  }
};

class EdgeItem : public QGraphicsPathItem {
public:
  VertexItem* from;
  VertexItem* to;
  bool directed;
  QGraphicsSimpleTextItem* label;
  EdgeItem(VertexItem* a, VertexItem* b, int weight, bool arrows, bool selected)
      : from(a), to(b), directed(arrows), label(new QGraphicsSimpleTextItem(QString::number(weight), this)) {
    setPen(QPen(selected ? QColor("#dc5030") : QColor("#8493a5"), selected ? 4 : 1.5));
    updatePath();
  }
  void updatePath() {
    const QPointF a = from->pos(), b = to->pos();
    QPainterPath curve;
    if (a == b) {
      curve.moveTo(a + QPointF(-12, -12));
      curve.cubicTo(a + QPointF(-65, -75), a + QPointF(65, -75), a + QPointF(12, -12));
    } else {
      const QLineF line(a, b);
      const QPointF unit = (b - a) / line.length();
      const QPointF normal(-unit.y(), unit.x());
      curve.moveTo(a + unit * 18);
      curve.quadTo((a + b) / 2 + normal * (directed ? 24 : 0), b - unit * 18);
    }
    label->setPos(curve.pointAtPercent(0.5));
    if (directed) {
      const QPointF end = curve.pointAtPercent(1), previous = curve.pointAtPercent(0.95);
      const QLineF tangent(previous, end);
      if (tangent.length() > 0) {
        const QPointF unit = (end - previous) / tangent.length(), normal(-unit.y(), unit.x());
        curve.moveTo(end - unit * 11 + normal * 5);
        curve.lineTo(end);
        curve.lineTo(end - unit * 11 - normal * 5);
      }
    }
    setPath(curve);
  }
};
}

void GraphScene::display(const Graph<int, int>& graph, bool directed, const Array<int>& path) {
  clear();
  if (graph.vertexCount() > 200 || graph.edgeCount() > 1000) {
    addText("Большой граф: используйте таблицу рёбер (визуализация до 200 вершин и 1000 рёбер)");
    return;
  }
  std::map<int, VertexItem*> vertices;
  const auto ids = graph.vertices();
  for (size_t i = 0; i < ids.size(); ++i) {
    auto* vertex = new VertexItem(ids[i]);
    addItem(vertex);
    const double angle = 6.283185307179586 * i / ids.size();
    vertex->setPos(250 * std::cos(angle), 250 * std::sin(angle));
    vertices[ids[i]] = vertex;
  }
  auto edges = std::make_shared<QList<EdgeItem*>>();
  for (size_t i = 0; i < ids.size(); ++i) {
    const auto& neighbors = graph.neighbors(ids[i]);
    for (size_t j = 0; j < neighbors.size(); ++j) {
      const auto& edge = neighbors[j];
      if (!directed && ids[i] > edge.first) continue;
      bool selected = false;
      for (size_t k = 1; k < path.size(); ++k)
        if ((path[k - 1] == ids[i] && path[k] == edge.first) ||
            (!directed && path[k] == ids[i] && path[k - 1] == edge.first)) selected = true;
      auto* item = new EdgeItem(vertices.at(ids[i]), vertices.at(edge.first), edge.second, directed, selected);
      addItem(item);
      edges->append(item);
    }
  }
  for (const auto& [id, vertex] : vertices)
    vertex->moved = [edges] { for (auto* edge : *edges) edge->updatePath(); };
}
