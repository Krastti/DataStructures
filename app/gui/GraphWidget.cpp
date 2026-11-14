#include "GraphScene.h"
#include <memory>
#include <vector>

namespace {
struct Result {
  QString error;
  std::shared_ptr<Graph<int, int>> graph;
  PathResult<int, int> path;
  QString summary;
  QStringList times;
};
QString describe(const QString& name, const PathResult<int, int>& path) {
  if (!path.found) return name + ": путь не найден";
  QStringList vertices;
  for (size_t i = 0; i < path.path.size(); ++i) vertices << QString::number(path.path[i]);
  return name + QString(": расстояние %1; ").arg(path.distance) + vertices.join(" → ");
}
}

QWidget* graphTab() {
  auto* root = new QWidget;
  auto* layout = new QVBoxLayout(root);
  auto* controls = new QHBoxLayout;
  auto* kind = new QComboBox;
  kind->addItems({"Неориентированный", "Ориентированный"});
  auto* count = number(1, 10000, 8);
  auto* extra = number(0, 100000, 4);
  auto* seed = number(0, 1000000, 1);
  controls->addWidget(kind);
  controls->addWidget(new QLabel("Вершин:"));
  controls->addWidget(count);
  controls->addWidget(new QLabel("Доп. рёбер:"));
  controls->addWidget(extra);
  controls->addWidget(new QLabel("Seed:"));
  controls->addWidget(seed);
  layout->addLayout(controls);
  auto* split = new QSplitter;
  auto* left = new QWidget;
  auto* leftLayout = new QVBoxLayout(left);
  leftLayout->addWidget(new QLabel("Вершины: 0 … N−1. Рёбра:"));
  auto* edges = table({"Откуда", "Куда", "Вес"}, true);
  edges->setObjectName("edges");
  leftLayout->addWidget(edges);
  auto* tools = new QHBoxLayout;
  leftLayout->addLayout(tools);
  split->addWidget(left);
  auto* scene = new GraphScene(root);
  auto* view = new GraphView(scene);
  view->setRenderHint(QPainter::Antialiasing);
  view->setDragMode(QGraphicsView::ScrollHandDrag);
  split->addWidget(view);
  split->setStretchFactor(1, 2);
  layout->addWidget(split);
  auto* search = new QHBoxLayout;
  auto* from = number(0, 9999, 0);
  auto* to = number(0, 9999, 7);
  auto* mode = new QComboBox;
  mode->addItems({"Дейкстра", "Беллман — Форд", "Сравнить"});
  search->addWidget(new QLabel("Начало:"));
  search->addWidget(from);
  search->addWidget(new QLabel("Конец:"));
  search->addWidget(to);
  search->addWidget(mode);
  layout->addLayout(search);
  auto* times = table({"Алгоритм", "Время, мс"});
  times->setMaximumHeight(110);
  layout->addWidget(times);
  auto* status = new QLabel("Введите рёбра и постройте граф или сгенерируйте его");
  status->setWordWrap(true);
  status->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
  status->setObjectName("status");
  layout->addWidget(status);
  auto state = std::make_shared<std::shared_ptr<Graph<int, int>>>();
  auto invalidate = [=] {
    state->reset();
    scene->clear();
    times->setRowCount(0);
    status->setText("Исходные данные изменены — перестройте граф");
  };
  QObject::connect(edges, &QTableWidget::itemChanged, root, invalidate);
  QObject::connect(count, &QSpinBox::valueChanged, root, invalidate);
  QObject::connect(kind, &QComboBox::currentIndexChanged, root, invalidate);
  auto display = [=](const Result& result, bool replaceEdges) {
    if (replaceEdges) {
      QSignalBlocker blocker(edges);
      edges->setRowCount(0);
      const auto ids = result.graph->vertices();
      for (size_t i = 0; i < ids.size(); ++i) {
        const auto& neighbors = result.graph->neighbors(ids[i]);
        for (size_t j = 0; j < neighbors.size(); ++j) {
          const auto& e = neighbors[j];
          if (kind->currentIndex() == 0 && ids[i] > e.first) continue;
          row(edges, {QString::number(ids[i]), QString::number(e.first), QString::number(e.second)});
        }
      }
    }
    *state = result.graph;
    from->setMaximum(count->value() - 1);
    to->setMaximum(count->value() - 1);
    scene->display(**state, kind->currentIndex() == 1, result.path.path);
    view->fitInView(scene->itemsBoundingRect().adjusted(-30, -30, 30, 30), Qt::KeepAspectRatio);
    status->setText(result.summary);
  };
  button("Добавить ребро", tools, [=] { row(edges, {"0", "1", "1"}); });
  button("Удалить", tools, [=] {
    if (edges->currentRow() >= 0) { edges->removeRow(edges->currentRow()); invalidate(); }
  });
  button("Построить", tools, [=] {
    struct Edge { int from, to, weight; };
    std::vector<Edge> input;
    const int n = count->value();
    for (int r = 0; r < edges->rowCount(); ++r) {
      Edge edge{integer(cell(edges, r, 0)), integer(cell(edges, r, 1)), integer(cell(edges, r, 2))};
      if (edge.from < 0 || edge.to < 0 || edge.from >= n || edge.to >= n)
        throw std::invalid_argument("Номер вершины должен быть от 0 до N−1");
      input.push_back(edge);
    }
    const bool directed = kind->currentIndex() == 1;
    background(root, status, [=] {
      Result result;
      if (directed) result.graph = std::make_shared<DirectedGraph<int, int>>();
      else result.graph = std::make_shared<UndirectedGraph<int, int>>();
      for (int i = 0; i < n; ++i) result.graph->addVertex(i);
      for (const auto& e : input) result.graph->addEdge(e.from, e.to, e.weight);
      result.summary = "Граф построен. Вершины можно перетаскивать, колесо — масштаб.";
      return result;
    }, [=](const Result& result) { times->setRowCount(0); display(result, false); });
  });
  button("Сгенерировать", controls, [=] {
    const int n = count->value(), e = extra->value(), s = seed->value();
    const bool directed = kind->currentIndex() == 1;
    background(root, status, [=] {
      Result result;
      if (directed) result.graph = std::make_shared<DirectedGraph<int, int>>(
          generateStronglyConnectedDirectedGraph<int, int>(n, e, 1, 20, s));
      else result.graph = std::make_shared<UndirectedGraph<int, int>>(
          generateConnectedGraph<int, int>(n, e, 1, 20, s));
      result.summary = "Граф сгенерирован и готов к поиску";
      return result;
    }, [=](const Result& result) { times->setRowCount(0); display(result, true); });
  });
  button("Найти путь", search, [=] {
    if (!*state) throw std::invalid_argument("Сначала постройте граф");
    auto graph = *state;
    scene->display(*graph, kind->currentIndex() == 1, {});
    times->setRowCount(0);
    const int a = from->value(), b = to->value(), algorithm = mode->currentIndex();
    background(root, status, [=] {
      Result result;
      result.graph = graph;
      PathResult<int, int> d, bf;
      QStringList details;
      double elapsed;
      if (algorithm != 1) {
        d = measured([&] { return dijkstra(*graph, a, b); }, elapsed);
        result.times << QString::number(elapsed, 'f', 6);
        details << describe("Дейкстра", d);
      }
      if (algorithm != 0) {
        bf = measured([&] { return bellmanFord(*graph, a, b); }, elapsed);
        result.times << QString::number(elapsed, 'f', 6);
        details << describe("Беллман — Форд", bf);
      }
      if (algorithm == 2)
        details << ((d.found == bf.found && (!d.found || d.distance == bf.distance))
            ? "Расстояния совпадают" : "Расстояния различаются");
      result.path = algorithm == 1 ? bf : d;
      result.summary = details.join("\n");
      return result;
    }, [=](const Result& result) {
      times->setRowCount(0);
      int i = 0;
      if (algorithm != 1) row(times, {"Дейкстра", result.times[i++]});
      if (algorithm != 0) row(times, {"Беллман — Форд", result.times[i++]});
      display(result, false);
    });
  });
  button("Экспорт замеров", search, [=] { exportTable(root, times); });
  return root;
}
