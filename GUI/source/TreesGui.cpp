#include "BinaryTree.h"
#include "RedBlackTree.h"

#include <QApplication>
#include <QButtonGroup>
#include <QComboBox>
#include <QFutureWatcher>
#include <QHelpEvent>
#include <QLineEdit>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QRandomGenerator>
#include <QResizeEvent>
#include <QSpinBox>
#include <QStyle>
#include <QStyleOptionComboBox>
#include <QToolButton>
#include <QToolTip>
#include <QWindow>
#include <QtConcurrent/QtConcurrentRun>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <numeric>
#include <optional>
#include <random>
#include <utility>
#include <vector>

namespace {

constexpr int designWidth = 1280;
constexpr int designHeight = 720;
const QColor background("#1d1d1d");
const QColor foreground("#f2f2f2");
const QColor cyan("#18ddec");
const QColor purple("#aa6eea");
const QColor red("#fa3034");
const QColor divider("#474747");

constexpr std::array demoKeys{50, 30, 70, 20, 40, 60, 80, 10, 35, 45, 55, 90};
constexpr std::array demoX{515, 336, 693, 236, 418, 605, 786, 181, 363, 468, 566, 838};
constexpr std::array sampleSizes{1000, 10000, 100000, 500000};
constexpr int batchSize = 1000;

QString sampleSizeLabel(int size) {
  return QString::number(size / 1000) + "k nodes";
}

using Binary = BinaryTree<int, int>;
using RedBlack = RedBlackTree<int, int>;

template<class Tree>
class DrawableTree : public Tree {
public:
  const auto* firstNode() const { return this->root; }

  bool redNode(int key) const {
    if constexpr (requires { this->isRed(key); }) {
      return this->isRed(key);
    }
    return false;
  }

  void generate() {
    DrawableTree next;
    while (next.size() < demoKeys.size()) {
      const int key = QRandomGenerator::global()->bounded(1, 100);
      next.insert(key, key);
    }
    *this = std::move(next);
  }
};

struct Timings {
  double insert;
  double search;
  double remove;

  double total() const { return insert + search + remove; }
};

using Samples = std::array<std::array<Timings, sampleSizes.size()>, 2>;

template<class Operation>
double elapsedMicros(Operation operation) {
  const auto start = std::chrono::steady_clock::now();
  operation();
  return std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - start).count();
}

template<class Tree>
Timings measure(const std::vector<int>& keys, int size) {
  Tree tree;
  for (int i = 0; i < size; ++i) {
    tree.insert(keys[i], keys[i]);
  }
  const double insert = elapsedMicros([&] {
    for (int i = size; i < size + batchSize; ++i) {
      tree.insert(keys[i], keys[i]);
    }
  });
  // Наблюдаемый результат сохраняет поиски в замере при оптимизации.
  volatile int matches = 0;
  const double search = elapsedMicros([&] {
    int found = 0;
    for (int i = 0; i < batchSize; ++i) {
      found += tree.containsKey(keys[i]);
    }
    matches = found;
  });
  const double remove = elapsedMicros([&] {
    for (int i = size; i < size + batchSize; ++i) {
      tree.remove(keys[i]);
    }
  });
  return {insert, search, remove};
}

Samples runBenchmarks() {
  Samples samples{};
  std::mt19937 generator(42);
  for (std::size_t i = 0; i < sampleSizes.size(); ++i) {
    const int size = sampleSizes[i];
    std::vector<int> keys(size + batchSize);
    std::iota(keys.begin(), keys.end(), 0);
    std::shuffle(keys.begin(), keys.end(), generator);
    samples[0][i] = measure<Binary>(keys, size);
    samples[1][i] = measure<RedBlack>(keys, size);
  }
  return samples;
}

QFont interfaceFont(int pixels, QFont::Weight weight = QFont::Normal) {
  QFont font("Arial");
  font.setPixelSize(pixels);
  font.setWeight(weight);
  return font;
}

void drawText(QPainter& painter, QRectF box, const QString& text, int pixels,
              int align = Qt::AlignCenter, QFont::Weight weight = QFont::Normal) {
  painter.setFont(interfaceFont(pixels, weight));
  painter.setPen(foreground);
  painter.drawText(box, align, text);
}

enum class Icon { Insert, Delete, Search, Generate, Minimize, Maximize, Restore, Close };

QIcon makeIcon(Icon icon, QColor color) {
  QPixmap pixmap(72, 72);
  pixmap.fill(Qt::transparent);
  QPainter painter(&pixmap);
  painter.scale(3, 3);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setPen(QPen(color, 1.7, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  switch (icon) {
    case Icon::Insert:
      painter.drawLine(QPointF(12, 4), QPointF(12, 20));
      painter.drawLine(QPointF(4, 12), QPointF(20, 12));
      break;
    case Icon::Delete:
      painter.drawRoundedRect(QRectF(6, 7, 12, 14), 1, 1);
      painter.drawLine(4, 6, 20, 6);
      painter.drawLine(9, 3, 15, 3);
      painter.drawLine(10, 10, 10, 18);
      painter.drawLine(14, 10, 14, 18);
      break;
    case Icon::Search:
      painter.drawEllipse(QRectF(3, 3, 14, 14));
      painter.drawLine(15, 15, 21, 21);
      break;
    case Icon::Generate:
      painter.drawArc(QRectF(3, 3, 18, 18), 42 * 16, 287 * 16);
      painter.drawPolyline(QPolygonF{{21, 3}, {21, 9}, {15, 9}});
      break;
    case Icon::Minimize:
      painter.drawLine(5, 12, 19, 12);
      break;
    case Icon::Maximize:
      painter.drawRect(QRectF(5, 5, 14, 14));
      break;
    case Icon::Restore:
      painter.drawPolyline(QPolygonF{{8, 7}, {8, 4}, {20, 4}, {20, 16}, {17, 16}});
      painter.drawRect(QRectF(4, 8, 12, 12));
      break;
    case Icon::Close:
      painter.drawLine(5, 5, 19, 19);
      painter.drawLine(19, 5, 5, 19);
      break;
  }
  return QIcon(pixmap);
}

class ValueInput : public QSpinBox {
public:
  void setTextFont(const QFont& font) {
    lineEdit()->setFont(font);
  }

protected:
  void paintEvent(QPaintEvent* event) override {
    QSpinBox::paintEvent(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(foreground, 1.3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    const qreal x = width() - 20;
    const qreal y = height() / 2.0;
    painter.drawPolyline(QPolygonF{{x - 4, y - 4}, {x, y - 8}, {x + 4, y - 4}});
    painter.drawPolyline(QPolygonF{{x - 4, y + 4}, {x, y + 8}, {x + 4, y + 4}});
  }
};

class BenchmarkSizeInput : public QComboBox {
protected:
  void paintEvent(QPaintEvent* event) override {
    QComboBox::paintEvent(event);
    QStyleOptionComboBox option;
    initStyleOption(&option);
    const QPointF center = style()->subControlRect(
        QStyle::CC_ComboBox, &option, QStyle::SC_ComboBoxArrow, this).center();
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(foreground, 1.3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPolyline(QPolygonF{center + QPointF(-4, -2), center + QPointF(0, 2),
                                    center + QPointF(4, -2)});
  }
};

struct VisualNode {
  int key;
  bool red;
  QPointF center;
  int parent;
};

template<class Node, class Tree>
void collectNodes(const Node* root, const Tree& tree, std::vector<VisualNode>& nodes, int& column) {
  struct Pending {
    const Node* node;
    int parent;
    int depth;
  };
  std::vector<Pending> pending;
  int parent = -1;
  int depth = 0;
  const Node* node = root;
  // Обход слева направо задаёт горизонтальный порядок ключей.
  while (node || !pending.empty()) {
    while (node) {
      const int index = static_cast<int>(nodes.size());
      nodes.push_back({node->key, tree.redNode(node->key), {0, qreal(depth)}, parent});
      pending.push_back({node, index, depth});
      parent = index;
      ++depth;
      node = node->left;
    }
    const auto [current, index, level] = pending.back();
    pending.pop_back();
    nodes[index].center.setX(qreal(++column)); // Временно храним порядковый номер.
    parent = index;
    depth = level + 1;
    node = current->right;
  }
}

class TreesWindow : public QWidget {
public:
  TreesWindow() : QWidget(nullptr, Qt::Window | Qt::FramelessWindowHint) {
    setWindowTitle("Trees");
    setAccessibleName("Trees");
    setMinimumSize(900, 506);
    setStyleSheet(R"(
      QPushButton, QSpinBox, QComboBox {
        color: #f2f2f2; background: #262626;
        border: 1px solid #414141; border-radius: 10px;
      }
      QPushButton:hover { background: #323232; }
      QPushButton:pressed { background: #393939; }
      QPushButton:focus, QSpinBox:focus { border-color: #18ddec; }
      QPushButton#insert { background: #1bdceb; color: #101a1b; border-color: #11c5d4; }
      QPushButton#insert:hover { background: #5ceaf4; }
      QPushButton#insert:pressed { background: #14bbd1; }
      QPushButton[treeTab="true"] { background: transparent; border: 1px solid transparent; border-radius: 19px; }
      QPushButton[treeTab="true"]:hover { background: #262c2d; }
      QPushButton[treeTab="true"]:checked { background: #1d3a3d; border-color: #18ddec; }
      QPushButton[treeTab="true"]:focus { border-color: #18ddec; }
      QSpinBox { background: #1c1c1c; padding-left: 13px; padding-right: 31px; selection-background-color: #246068; }
      QSpinBox::up-button, QSpinBox::down-button { width: 31px; border: none; }
      QSpinBox::up-arrow, QSpinBox::down-arrow { width: 0px; height: 0px; }
      QComboBox { padding: 0 8px; }
      QComboBox:hover, QComboBox:focus { border-color: #18ddec; }
      QComboBox::drop-down { width: 24px; border: 0; }
      QComboBox::down-arrow { width: 0px; height: 0px; }
      QComboBox QAbstractItemView {
        color: #f2f2f2; background: #262626; border: 1px solid #414141;
        selection-color: #f2f2f2; selection-background-color: #246068;
      }
      QToolButton { border: 0; color: #f2f2f2; background: transparent; }
      QToolButton:hover { background: #353535; }
      QToolButton#close:hover { background: #bc2832; }
    )");

    auto* tabs = new QButtonGroup(this);
    binaryTab = addTab("Binary Tree", tabs, {371, 47, 234, 40});
    addTab("Red-Black Tree", tabs, {604, 47, 234, 40});
    binaryTab->setChecked(true);

    value = new ValueInput;
    value->setAccessibleName("Value");
    value->setRange(-999999, 999999);
    value->setValue(65);
    place(value, {967, 150, 283, 42}, 16);
    auto* insert = addAction("Insert", Icon::Insert, {967, 220, 136, 45}, "#101a1b");
    insert->setObjectName("insert");
    auto* remove = addAction("Delete", Icon::Delete, {1117, 220, 133, 45}, foreground);
    auto* search = addAction("Search", Icon::Search, {967, 282, 136, 45}, foreground);
    auto* generate = addAction("Generate", Icon::Generate, {1117, 282, 133, 45}, cyan);
    generate->setToolTip("Replace the selected tree with a random sample");

    benchmarkSize = new BenchmarkSizeInput;
    benchmarkSize->setAccessibleName("Benchmark size");
    for (const int size : sampleSizes) {
      benchmarkSize->addItem(sampleSizeLabel(size));
    }
    benchmarkSize->setCurrentIndex(benchmarkSize->count() - 1);
    place(benchmarkSize, {747, 398, 127, 31}, 15);
    connect(benchmarkSize, &QComboBox::currentIndexChanged, this, [this] { update(); });

    connect(tabs, &QButtonGroup::buttonClicked, this, [this] { clearStatus(); });
    connect(insert, &QPushButton::clicked, this, [this] { apply(Operation::Insert); });
    connect(remove, &QPushButton::clicked, this, [this] { apply(Operation::Delete); });
    connect(search, &QPushButton::clicked, this, [this] { apply(Operation::Search); });
    connect(generate, &QPushButton::clicked, this, [this] { apply(Operation::Generate); });
    connect(value, &QSpinBox::valueChanged, this, [this] { clearStatus(); });

    auto* minimize = addWindowButton("Minimize", Icon::Minimize, 1115);
    maximize = addWindowButton("Maximize", Icon::Maximize, 1170);
    auto* close = addWindowButton("Close", Icon::Close, 1224);
    close->setObjectName("close");
    connect(minimize, &QToolButton::clicked, this, &QWidget::showMinimized);
    connect(maximize, &QToolButton::clicked, this, [this] { toggleMaximized(); });
    connect(close, &QToolButton::clicked, this, &QWidget::close);
    for (const int key : demoKeys) {
      binary.insert(key, key);
      redBlack.insert(key, key);
    }
    connect(&watcher, &QFutureWatcher<Samples>::finished, this, [this] {
      try {
        samples = watcher.result();
      } catch (...) {
        benchmarkFailed = true;
      }
      update();
    });
    watcher.setFuture(QtConcurrent::run(runBenchmarks));
    resize(designWidth, designHeight);
  }

protected:
  void resizeEvent(QResizeEvent*) override {
    const qreal scale = sceneScale();
    const QPointF offset = sceneOffset();
    for (const auto& control : controls) {
      const auto& box = control.box;
      control.widget->setGeometry(QRectF(offset.x() + box.x() * scale, offset.y() + box.y() * scale,
                                        box.width() * scale, box.height() * scale).toAlignedRect());
      control.widget->setFont(interfaceFont(qRound(control.pixels * scale), control.weight));
      if (control.iconSize != 0) {
        const int size = qRound(control.iconSize * scale);
        static_cast<QAbstractButton*>(control.widget)->setIconSize({size, size});
      }
    }
    value->setTextFont(interfaceFont(qRound(16 * scale)));
  }

  bool event(QEvent* event) override {
    if (event->type() == QEvent::ToolTip) {
      const auto* help = static_cast<QHelpEvent*>(event);
      const QPointF point = (help->pos() - sceneOffset()) / sceneScale();
      if (QRectF(41, 438, 565, 219).contains(point)) {
        QToolTip::showText(help->globalPos(), "Total time for 1,000 inserts, 1,000 searches and 1,000 deletes", this);
        return true;
      }
      if (QRectF(654, 438, 596, 138).contains(point)) {
        QToolTip::showText(help->globalPos(),
                           "Average time per operation at the selected initial tree size", this);
        return true;
      }
    }
    return QWidget::event(event);
  }

  void mousePressEvent(QMouseEvent* event) override {
    if (event->button() == Qt::LeftButton && inTitleBar(event) && windowHandle()) {
      windowHandle()->startSystemMove();
    }
  }

  void mouseDoubleClickEvent(QMouseEvent* event) override {
    if (event->button() == Qt::LeftButton && inTitleBar(event)) {
      toggleMaximized();
    }
  }

  void changeEvent(QEvent* event) override {
    QWidget::changeEvent(event);
    if (event->type() == QEvent::WindowStateChange && maximize) {
      maximize->setIcon(makeIcon(isMaximized() ? Icon::Restore : Icon::Maximize, foreground));
      maximize->setToolTip(isMaximized() ? "Restore" : "Maximize");
      maximize->setAccessibleName(maximize->toolTip());
    }
  }

  void paintEvent(QPaintEvent*) override {
    QPainter painter(this);
    painter.fillRect(rect(), background);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.translate(sceneOffset());
    painter.scale(sceneScale(), sceneScale());

    painter.setPen(divider);
    painter.drawRoundedRect(QRectF(371, 47, 467, 39), 19.5, 19.5);
    painter.drawLine(928, 111, 928, 351);
    painter.drawLine(41, 381, 1250, 381);
    drawText(painter, {966, 116, 283, 25}, "Value", 16, Qt::AlignVCenter | Qt::AlignLeft,
             QFont::DemiBold);
    drawText(painter, {967, 330, 283, 37}, status, 14, Qt::AlignVCenter | Qt::AlignLeft);

    if (binaryTab->isChecked()) {
      drawTree(painter, binary, originalBinary);
    } else {
      drawTree(painter, redBlack, false);
    }
    drawBenchmarks(painter);
  }

private:
  enum class Operation { Insert, Delete, Search, Generate };

  struct Control {
    QWidget* widget;
    QRect box;
    int pixels;
    QFont::Weight weight;
    int iconSize;
  };

  DrawableTree<Binary> binary;
  DrawableTree<RedBlack> redBlack;
  bool originalBinary = true;
  QPushButton* binaryTab = nullptr;
  QToolButton* maximize = nullptr;
  ValueInput* value = nullptr;
  QComboBox* benchmarkSize = nullptr;
  std::vector<Control> controls;
  QString status;
  std::optional<int> highlighted;
  std::optional<Samples> samples;
  bool benchmarkFailed = false;
  QFutureWatcher<Samples> watcher;

  qreal sceneScale() const {
    return std::min(width() / qreal(designWidth), height() / qreal(designHeight));
  }

  QPointF sceneOffset() const {
    return {(width() - designWidth * sceneScale()) / 2, (height() - designHeight * sceneScale()) / 2};
  }

  bool inTitleBar(QMouseEvent* event) const {
    return (event->position().y() - sceneOffset().y()) / sceneScale() < 44;
  }

  void toggleMaximized() {
    if (isMaximized()) {
      showNormal();
    } else {
      showMaximized();
    }
  }

  void place(QWidget* widget, QRect box, int pixels, QFont::Weight weight = QFont::Normal,
             int iconSize = 0) {
    widget->setParent(this);
    controls.push_back({widget, box, pixels, weight, iconSize});
  }

  QPushButton* addTab(const QString& label, QButtonGroup* group, QRect box) {
    auto* button = new QPushButton(label);
    button->setProperty("treeTab", true);
    button->setCheckable(true);
    group->addButton(button);
    place(button, box, 16, QFont::DemiBold);
    return button;
  }

  QPushButton* addAction(const QString& label, Icon icon, QRect box, const QColor& color) {
    auto* button = new QPushButton(makeIcon(icon, color), label);
    place(button, box, 16, icon == Icon::Insert ? QFont::DemiBold : QFont::Normal, 25);
    return button;
  }

  QToolButton* addWindowButton(const QString& label, Icon icon, int x) {
    auto* button = new QToolButton;
    button->setAccessibleName(label);
    button->setToolTip(label);
    button->setIcon(makeIcon(icon, foreground));
    place(button, {x, 0, 54, 43}, 14, QFont::Normal, 20);
    return button;
  }

  void clearStatus() {
    highlighted.reset();
    status.clear();
    update();
  }

  void apply(Operation operation) {
    clearStatus();
    if (binaryTab->isChecked()) {
      if (applyTo(binary, operation)) {
        originalBinary = false;
      }
    } else {
      applyTo(redBlack, operation);
    }
    update();
  }

  template<class Tree>
  bool applyTo(Tree& tree, Operation operation) {
    if (operation == Operation::Generate) {
      tree.generate();
      return true;
    }
    const int key = value->value();
    const bool found = tree.containsKey(key);
    switch (operation) {
      case Operation::Insert:
        if (found) {
          status = "Value already exists";
          return false;
        }
        tree.insert(key, key);
        return true;
      case Operation::Delete:
        if (found) {
          tree.remove(key);
          return true;
        }
        status = "Value not found";
        return false;
      case Operation::Search:
        if (found) {
          highlighted = key;
          status = "Value found";
        } else {
          status = "Value not found";
        }
        return false;
      case Operation::Generate:
        return false;
    }
    return false;
  }

  template<class Tree>
  void drawTree(QPainter& painter, const Tree& tree, bool useDemoLayout) {
    if (!tree.firstNode()) {
      drawText(painter, {42, 101, 850, 245}, "Tree is empty", 17);
      return;
    }
    std::vector<VisualNode> nodes;
    nodes.reserve(tree.size());
    int column = 0;
    collectNodes(tree.firstNode(), tree, nodes, column);
    const auto deepest = std::ranges::max_element(nodes, {}, [](const VisualNode& node) {
      return node.center.y();
    });
    const auto levels = deepest->center.y();
    const qreal stepX = std::min(70.0, 680.0 / std::max(1, column - 1));
    const qreal stepY = std::min(64.5, 193.5 / std::max(1.0, levels));
    const qreal radius = std::min({22.0, stepX * 0.44, stepY * 0.44});
    for (auto& node : nodes) {
      qreal x = 515 + (node.center.x() - (column + 1) / 2.0) * stepX;
      if (useDemoLayout) {
        const auto key = std::ranges::find(demoKeys, node.key);
        x = demoX[key - demoKeys.begin()];
      }
      node.center = {x, 124.5 + node.center.y() * stepY};
    }

    painter.setPen(QPen(QColor("#d0d0d0"), 1.8));
    for (const auto& node : nodes) {
      if (node.parent >= 0) {
        painter.drawLine(nodes[node.parent].center, node.center);
      }
    }
    for (const auto& node : nodes) {
      const bool selected = highlighted == node.key;
      const QColor outline = node.red ? red : cyan;
      painter.setPen(QPen(selected ? QColor("#ffd77b") : outline, std::min(2.0, radius / 4)));
      painter.setBrush(selected ? QColor("#514326") : background);
      painter.drawEllipse(node.center, radius, radius);
      const QString label = QString::number(node.key);
      const int pixels = qFloor(std::min(radius * 0.76, radius * 3 / label.size()));
      if (pixels >= 7) {
        drawText(painter, QRectF(node.center.x() - radius, node.center.y() - radius,
                                2 * radius, 2 * radius), label, pixels, Qt::AlignCenter, QFont::DemiBold);
      }
    }
  }

  void drawBenchmarks(QPainter& painter) {
    painter.setPen(Qt::NoPen);
    painter.setBrush(cyan);
    painter.drawRect(QRectF(42, 413, 4, 10));
    painter.drawRect(QRectF(50, 408, 4, 15));
    painter.drawRect(QRectF(58, 403, 4, 20));
    drawText(painter, {80, 398, 500, 31}, "Benchmarks", 21, Qt::AlignVCenter | Qt::AlignLeft,
             QFont::DemiBold);

    const auto xAt = [](std::size_t index) {
      return 199.0 + (560.0 - 199.0) * index / (sampleSizes.size() - 1);
    };
    constexpr auto top = 455.0;
    constexpr auto bottom = 582.0;
    const std::array<QString, 5> ticks{"100 ms", "10 ms", "1 ms", "100 μs", "10 μs"};

    painter.save();
    painter.translate(53, 521);
    painter.rotate(-90);
    drawText(painter, {-60, -10, 120, 20}, "Time (μs)", 14);
    painter.restore();
    painter.setPen(QColor("#929292"));
    painter.drawLine(QPointF(151, top - 6), QPointF(151, bottom));
    painter.drawLine(QPointF(151, bottom), QPointF(606, bottom));
    for (std::size_t i = 0; i < ticks.size(); ++i) {
      const qreal y = top + (bottom - top) * i / (ticks.size() - 1);
      painter.setPen(QColor("#929292"));
      painter.drawLine(QPointF(145, y), QPointF(151, y));
      drawText(painter, {71, y - 11, 64, 22}, ticks[i], 15, Qt::AlignRight | Qt::AlignVCenter);
    }
    for (std::size_t i = 0; i < sampleSizes.size(); ++i) {
      const qreal x = xAt(i);
      painter.setPen(QColor("#929292"));
      painter.drawLine(QPointF(x, bottom), QPointF(x, bottom + 8));
      drawText(painter, {x - 45, 592, 90, 26}, sampleSizeLabel(sampleSizes[i]), 15);
    }

    const std::array<QString, 2> labels{"Binary Tree", "Red-Black Tree"};
    const std::array lineColors{purple, cyan};
    for (std::size_t series = 0; series < labels.size(); ++series) {
      const QColor& color = lineColors[series];
      if (samples) {
        QPolygonF points;
        for (std::size_t i = 0; i < sampleSizes.size(); ++i) {
          const double logTime = std::log10(std::max(10.0, (*samples)[series][i].total()));
          const qreal fraction = std::clamp((logTime - 1.0) / 4.0, 0.0, 1.0);
          points.append({xAt(i), bottom - fraction * (bottom - top)});
        }
        QPainterPath fill;
        fill.addPolygon(points);
        fill.lineTo(points.last().x(), bottom);
        fill.lineTo(points.first().x(), bottom);
        fill.closeSubpath();
        QLinearGradient gradient(0, points.last().y(), 0, bottom);
        QColor shaded = color;
        shaded.setAlpha(38);
        gradient.setColorAt(0, shaded);
        shaded.setAlpha(0);
        gradient.setColorAt(1, shaded);
        painter.fillPath(fill, gradient);
        painter.setPen(QPen(color, 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawPolyline(points);
        painter.setBrush(color);
        for (const auto& point : points) {
          painter.drawEllipse(point, 5, 5);
        }
      }

      const int x = 209 + 169 * series;
      painter.setPen(QPen(color, 2));
      painter.setBrush(color);
      painter.drawLine(x, 644, x + 35, 644);
      painter.drawEllipse(QPointF(x + 17.5, 644), 5, 5);
      drawText(painter, {x + 44.0, 630, 151, 28}, labels[series], 15, Qt::AlignLeft | Qt::AlignVCenter);
    }
    if (!samples) {
      drawText(painter, {155, 470, 450, 85}, benchmarkFailed ? "Measurements unavailable" : "Measuring…", 15);
    }

    drawText(painter, {654, 398, 92, 31}, "Averages at", 15, Qt::AlignLeft | Qt::AlignVCenter);
    drawText(painter, {654, 438, 185, 35}, "Structure", 16, Qt::AlignLeft | Qt::AlignVCenter,
             QFont::DemiBold);
    const std::array<QString, 3> headings{"Insert avg", "Search avg", "Delete avg"};
    for (std::size_t i = 0; i < headings.size(); ++i) {
      drawText(painter, {840.0 + 150 * i, 438, 108, 35}, headings[i], 16, Qt::AlignCenter,
               QFont::DemiBold);
    }
    for (int i = 0; i < 3; ++i) {
      painter.setPen(divider);
      painter.drawLine(654, 482 + i * 47, 1250, 482 + i * 47);
    }
    for (std::size_t series = 0; series < labels.size(); ++series) {
      const qreal y = 506 + 47 * series;
      painter.setPen(Qt::NoPen);
      painter.setBrush(series == 0 ? cyan : red);
      painter.drawEllipse(QPointF(661, y), 6.5, 6.5);
      drawText(painter, {682, y - 12, 155, 24}, labels[series], 16, Qt::AlignLeft | Qt::AlignVCenter);
      for (std::size_t i = 0; i < headings.size(); ++i) {
        QString result = "—";
        if (samples) {
          const auto& timing = (*samples)[series][benchmarkSize->currentIndex()];
          const std::array operations{timing.insert, timing.search, timing.remove};
          result = QString::number(operations[i] / batchSize, 'f', 3) + " μs";
        }
        drawText(painter, {840.0 + 150 * i, y - 12, 108, 24}, result, 15);
      }
    }
  }
};

}

int main(int argc, char** argv) {
  QApplication app(argc, argv);
  TreesWindow window;
  window.show();
  return app.exec();
}
