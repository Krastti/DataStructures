#include "interpreter.h"
#include "lexer.h"
#include "parser.h"

#include <QApplication>
#include <QColor>
#include <QFile>
#include <QFileDialog>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QFrame>
#include <QGraphicsEllipseItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QHash>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QList>
#include <QLocale>
#include <QMainWindow>
#include <QMetaObject>
#include <QMutex>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QShortcut>
#include <QSplitter>
#include <QTabWidget>
#include <QTextCursor>
#include <QThread>
#include <QTreeWidget>
#include <QVariant>
#include <QVBoxLayout>
#include <QWaitCondition>
#include <QWheelEvent>

#include <algorithm>
#include <cstddef>
#include <exception>
#include <functional>
#include <initializer_list>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

QString tokenName(const TokenType type) {
  switch (type) {
    case TokenType::Remember: return "Remember";
    case TokenType::Print: return "Print";
    case TokenType::Identifier: return "Identifier";
    case TokenType::Number: return "Number";
    case TokenType::String: return "String";
    case TokenType::Plus: return "Plus";
    case TokenType::Minus: return "Minus";
    case TokenType::Star: return "Star";
    case TokenType::Slash: return "Slash";
    case TokenType::Equals: return "Equals";
    case TokenType::EqualEqual: return "EqualEqual";
    case TokenType::BangEqual: return "BangEqual";
    case TokenType::Less: return "Less";
    case TokenType::LessEqual: return "LessEqual";
    case TokenType::Greater: return "Greater";
    case TokenType::GreaterEqual: return "GreaterEqual";
    case TokenType::LeftParen: return "LeftParen";
    case TokenType::RightParen: return "RightParen";
    case TokenType::Newline: return "Newline";
    case TokenType::EndOfFile: return "EndOfFile";
    case TokenType::When: return "When";
    case TokenType::Otherwise: return "Otherwise";
    case TokenType::Repeat: return "Repeat";
    case TokenType::While: return "While";
    case TokenType::End: return "End";
    case TokenType::For: return "For";
    case TokenType::In: return "In";
    case TokenType::Range: return "Range";
  }
  return {};
}

QString operatorText(const TokenType type) {
  switch (type) {
    case TokenType::Plus: return "+";
    case TokenType::Minus: return "-";
    case TokenType::Star: return "*";
    case TokenType::Slash: return "/";
    case TokenType::EqualEqual: return "==";
    case TokenType::BangEqual: return "!=";
    case TokenType::Less: return "<";
    case TokenType::LessEqual: return "<=";
    case TokenType::Greater: return ">";
    case TokenType::GreaterEqual: return ">=";
    default: return tokenName(type);
  }
}

QString visibleText(const std::string& text) {
  return QString::fromStdString(text).replace("\\", "\\\\")
    .replace("\r", "\\r").replace("\n", "\\n").replace("\t", "\\t").replace(QChar('\0'), "\\0");
}

using Program = Array<UniquePtr<ast::Statement>>;
using AstNodeId = quintptr;
constexpr AstNodeId programNode = 0;
constexpr int edgeRole = Qt::UserRole;
constexpr int nodeIdRole = Qt::UserRole + 1;
constexpr int nodeNameRole = Qt::UserRole + 2;

class AstDiagram : public QGraphicsView {
public:
  AstDiagram() {
    setObjectName("astDiagram");
    setScene(new QGraphicsScene(this));
    setBackgroundBrush(QColor("#1c1c1c"));
    setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setAccessibleName("Диаграмма AST");
    setToolTip("Колесо мыши меняет масштаб. Перетаскивание перемещает дерево.");
  }

  void clear() {
    highlighted = nullptr;
    nodes.clear();
    scene()->clear();
    setSceneRect(QRectF(0, 0, 1, 1));
    resetTransform();
    needsFit = false;
    hasTree = false;
  }

  void setTree(const QTreeWidgetItem& root) {
    qreal nextLeaf = 0;
    drawTree(root, 0, nextLeaf);
    setSceneRect(scene()->itemsBoundingRect().adjusted(-22, -22, 22, 22));
    needsFit = true;
    hasTree = true;
    if (isVisible()) fitTree();
  }

  void fitTree() {
    if (!hasTree) return;
    needsFit = false;
    resetTransform();
    if (sceneRect().width() > viewport()->width() || sceneRect().height() > viewport()->height()) {
      fitInView(sceneRect(), Qt::KeepAspectRatio);
    }
    centerOn(sceneRect().center());
  }

  void zoom(qreal factor) {
    if (!hasTree) return;
    const qreal current = transform().m11();
    const qreal next = std::clamp(current * factor, std::min(current, 0.15), 2.0);
    scale(next / current, next / current);
  }

  QString nodeName(AstNodeId id) const {
    return nodes.value(id).name;
  }

  void highlight(AstNodeId id, const QString& value = {}) {
    if (highlighted) {
      highlighted->circle->setPen(QPen(QColor("#18ddec"), 2));
      highlighted->circle->setBrush(QColor("#1d2525"));
      setSubtitle(*highlighted, highlighted->subtitleText);
    }
    auto node = nodes.find(id);
    highlighted = node == nodes.end() ? nullptr : &node.value();
    if (!highlighted) return;

    highlighted->circle->setPen(QPen(QColor("#ffd77b"), 2.5));
    highlighted->circle->setBrush(QColor("#514326"));
    if (!value.isEmpty()) setSubtitle(*highlighted, "= " + value);
    ensureVisible(highlighted->circle, 24, 24);
  }

protected:
  void showEvent(QShowEvent* event) override {
    QGraphicsView::showEvent(event);
    if (needsFit) fitTree();
  }

  void wheelEvent(QWheelEvent* event) override {
    if (event->angleDelta().y() > 0) zoom(1.2);
    else if (event->angleDelta().y() < 0) zoom(1.0 / 1.2);
    event->accept();
  }

private:
  struct DiagramNode {
    QGraphicsEllipseItem* circle;
    QString name;
    QGraphicsSimpleTextItem* subtitle = nullptr;
    QString subtitleText;
  };

  static constexpr qreal radius = 32;
  static constexpr qreal columnStep = 83;
  static constexpr qreal levelStep = 105;
  bool needsFit = false;
  bool hasTree = false;
  QHash<AstNodeId, DiagramNode> nodes;
  DiagramNode* highlighted = nullptr;

  QPointF drawTree(const QTreeWidgetItem& node, int depth, qreal& nextLeaf) {
    const qreal y = depth * levelStep;
    qreal x = nextLeaf;
    QList<QPointF> children;
    for (int i = 0; i < node.childCount(); ++i) {
      const auto& child = *node.child(i);
      const auto& actualChild = child.data(0, edgeRole).toBool() ? *child.child(0) : child;
      children.append(drawTree(actualChild, depth + 1, nextLeaf));
    }

    // Листья занимают отдельные колонки, а родитель находится над центром своих детей.
    if (children.isEmpty()) {
      nextLeaf += columnStep;
    } else {
      x = (children.front().x() + children.back().x()) / 2;
    }
    const QPointF center(x, y);
    for (int i = 0; i < children.size(); ++i) {
      const auto& child = *node.child(i);
      const QString role = child.data(0, edgeRole).toBool() ? child.text(0) : QString{};
      drawEdge(center, children[i], role);
    }
    drawNode(center, node);
    return center;
  }

  void drawEdge(const QPointF& parent, const QPointF& child, const QString& role) {
    scene()->addLine(QLineF(parent, child), QPen(QColor("#a7a7a7"), 1.5))->setZValue(-2);
    if (role.isEmpty()) return;

    const QPointF center = (parent + child) / 2;
    auto* text = drawLabel(center, role, 11, QColor("#bdbdbd"));
    scene()->addRect(text->sceneBoundingRect().adjusted(-3, 0, 3, 0),
                     Qt::NoPen, QColor("#1c1c1c"))->setZValue(-1);
  }

  void drawNode(const QPointF& center, const QTreeWidgetItem& item) {
    const QString type = item.text(0);
    const QString detail = item.text(1);
    const QRectF bounds(center.x() - radius, center.y() - radius, 2 * radius, 2 * radius);
    auto* circle = scene()->addEllipse(bounds, QPen(QColor("#18ddec"), 2), QColor("#1d2525"));
    const QString description = detail.isEmpty() ? type : type + "\n" + detail;
    circle->setToolTip(description);

    const QColor foreground("#f2f2f2");
    QGraphicsSimpleTextItem* subtitle = nullptr;
    QString label = type;
    if (type == "NumberLiteral") label = "Number";
    if (type == "StringLiteral") label = "String";
    if (type == "RepeatWhile") {
      drawLabel(center - QPointF(0, 7), "Repeat", 12, foreground)->setToolTip(description);
      drawLabel(center + QPointF(0, 7), "While", 12, foreground)->setToolTip(description);
    } else if (detail.isEmpty()) {
      drawLabel(center, label, type == "otherwise" ? 10 : 12, foreground)->setToolTip(description);
    } else {
      drawLabel(center - QPointF(0, 8), detail, 12, foreground)->setToolTip(description);
      subtitle = drawLabel(center + QPointF(0, 9), label, 10, QColor("#a7d2d5"));
      subtitle->setToolTip(description);
    }

    const QVariant id = item.data(0, nodeIdRole);
    if (id.isValid()) {
      nodes.insert(id.value<AstNodeId>(), {circle, item.data(0, nodeNameRole).toString(), subtitle, label});
    }
  }

  static void setSubtitle(const DiagramNode& node, const QString& text) {
    if (!node.subtitle) return;
    const QFontMetrics metrics(node.subtitle->font());
    node.subtitle->setText(metrics.elidedText(text, Qt::ElideRight, 2 * radius - 12));
    node.subtitle->setPos(node.circle->rect().center() + QPointF(0, 9) - node.subtitle->boundingRect().center());
    node.subtitle->setToolTip(node.circle->toolTip() + "\n" + text);
  }

  QGraphicsSimpleTextItem* drawLabel(const QPointF& center, const QString& text, int pixels, const QColor& color) {
    QFont font = this->font();
    font.setPixelSize(pixels);
    const QFontMetrics metrics(font);
    auto* label = scene()->addSimpleText(metrics.elidedText(text, Qt::ElideRight, 2 * radius - 10), font);
    label->setBrush(color);
    label->setPos(center - label->boundingRect().center());
    label->setZValue(1);
    return label;
  }
};

class AstViews : private ast::StatementVisitor, private ast::ExprVisitor {
public:
  AstViews(const Program& program, QTreeWidget& statements, QTreeWidget& expressions, AstDiagram& diagram)
    : expressions(expressions) {
    QTreeWidgetItem root({"Program"});
    identify(root, programNode, "Program");
    addStatements(program, statements.invisibleRootItem(), &root, {});
    diagram.setTree(root);
    astItem = nullptr;
  }

private:
  QTreeWidget& expressions;
  QTreeWidgetItem* statementItem = nullptr;
  QTreeWidgetItem* astItem = nullptr;

  void addStatements(const Program& program, QTreeWidgetItem* statementParent, QTreeWidgetItem* astParent,
                     const QString& prefix) {
    for (std::size_t i = 0; i < program.size(); ++i) {
      const QString number = prefix + QString::number(i + 1);
      statementItem = new QTreeWidgetItem(statementParent, {number});
      astItem = new QTreeWidgetItem(astParent);
      program[i]->accept(*this);
    }
  }

  void addStatement(const ast::Statement& node, const QString& type, const QString& detail = {}) {
    const QString number = statementItem->text(0);
    statementItem->setText(1, type);
    statementItem->setText(2, detail);
    astItem->setText(0, type);
    astItem->setText(1, detail);
    const QString name = detail.isEmpty() ? type : type + " " + detail;
    identify(*astItem, reinterpret_cast<AstNodeId>(&node), number + "  " + name);
  }

  void addExpression(const QString& role, const ast::ExpressionTree& tree) {
    auto* statement = astItem;
    auto* root = addExpressionChild(astItem, role);
    astItem = root;
    tree.getRoot()->accept(*this);
    auto* item = new QTreeWidgetItem(&expressions, {
      statement->data(0, nodeNameRole).toString() + " · " + role, {}, QString::number(tree.size())
    });
    item->addChild(root->clone());
    astItem = statement;
  }

  void addBranch(const QString& label, const QString& prefix, const Program& branch,
                 QTreeWidgetItem* statementParent, QTreeWidgetItem* astParent) {
    auto* statementBranch = new QTreeWidgetItem(statementParent, {{}, label});
    auto* astBranch = new QTreeWidgetItem(astParent, {label});
    addStatements(branch, statementBranch, astBranch, prefix);
  }

  void addExprNode(const ast::Expr& node, const QString& type, const QString& detail) {
    astItem->setText(0, type);
    astItem->setText(1, detail);
    identify(*astItem, reinterpret_cast<AstNodeId>(&node), type + " " + detail);
  }

  void visit(const ast::Remember& node) override {
    addStatement(node, "Remember", QString::fromStdString(node.variable));
    addExpression("right", node.right);
  }

  void visit(const ast::Print& node) override {
    addStatement(node, "Print");
    addExpression("forOutput", node.forOutput);
  }

  void visit(const ast::When& node) override {
    addStatement(node, "When");
    addExpression("condition", node.condition);
    auto* statementParent = statementItem;
    auto* astParent = astItem;
    const QString prefix = statementParent->text(0);
    addBranch("then", prefix + ".then.", node.thenBranch, statementParent, astParent);
    addBranch("otherwise", prefix + ".otherwise.", node.otherwiseBranch, statementParent, astParent);
  }

  void visit(const ast::RepeatWhile& node) override {
    addStatement(node, "RepeatWhile");
    addExpression("condition", node.condition);
    addBranch("body", statementItem->text(0) + ".", node.body, statementItem, astItem);
  }

  void visit(const ast::For& node) override {
    addStatement(node, "For", QString::fromStdString(node.variable));
    addExpression("stop", node.stop);
    addBranch("body", statementItem->text(0) + ".", node.body, statementItem, astItem);
  }

  void visit(const ast::NumberLiteral& node) override {
    addExprNode(node, "NumberLiteral", QString::fromStdString(node.lexeme));
  }

  void visit(const ast::StringLiteral& node) override {
    addExprNode(node, "StringLiteral", visibleText(node.lexeme));
  }

  void visit(const ast::Identifier& node) override {
    addExprNode(node, "Identifier", QString::fromStdString(node.name));
  }

  void visit(const ast::Binary& node) override {
    addExprNode(node, "Binary", operatorText(node.operation));
    auto* parent = astItem;
    astItem = addExpressionChild(parent, "left");
    node.leftOperand()->accept(*this);
    astItem = addExpressionChild(parent, "right");
    node.rightOperand()->accept(*this);
  }

  static QTreeWidgetItem* addExpressionChild(QTreeWidgetItem* parent, const QString& role) {
    auto* slot = new QTreeWidgetItem(parent, {role});
    // В списке роль видна отдельной строкой, на диаграмме она подписывает ребро.
    slot->setData(0, edgeRole, true);
    return new QTreeWidgetItem(slot);
  }

  static void identify(QTreeWidgetItem& item, AstNodeId id, const QString& name) {
    item.setData(0, nodeIdRole, QVariant::fromValue(id));
    item.setData(0, nodeNameRole, name);
  }
};

QString displayValue(const Interpreter::Value& value) {
  if (const auto* number = std::get_if<double>(&value)) {
    return QString::number(*number, 'g', QLocale::FloatingPointShortest);
  }
  if (const auto* boolean = std::get_if<bool>(&value)) return *boolean ? "true" : "false";
  return "\"" + visibleText(std::get<std::string>(value)).replace("\"", "\\\"") + "\"";
}

enum class CallPhase { Program, Statement, Expression, Return };

struct CallEvent {
  CallPhase phase;
  AstNodeId node = programNode;
  QString value;
};

class ExecutionObserver : public Interpreter::Observer {
public:
  using Report = std::function<void(const CallEvent&)>;

  static void run(const Program& program, std::ostream& output, Report report) {
    Interpreter interpreter;
    ExecutionObserver observer(std::move(report));
    observer.report({CallPhase::Program, programNode, {}});
    interpreter.execute(program, output, &observer);
  }

  void enterStatement(const ast::Statement& statement) override {
    enter(CallPhase::Statement, reinterpret_cast<AstNodeId>(&statement));
  }

  void enterExpression(const ast::Expr& expression) override {
    enter(CallPhase::Expression, reinterpret_cast<AstNodeId>(&expression));
  }

  void leaveCall(const Interpreter::Value* value) override {
    report({CallPhase::Return, programNode, value ? displayValue(*value) : QString{}});
  }

private:
  static constexpr std::size_t callLimit = 10000;
  Report report;
  std::size_t calls = 0;

  explicit ExecutionObserver(Report report) : report(std::move(report)) {}

  void enter(CallPhase phase, AstNodeId node) {
    if (calls == callLimit) {
      throw std::runtime_error("Достигнут лимит выполнения (10 000 вызовов).");
    }
    ++calls;
    report({phase, node, {}});
  }
};

class ExecutionTrace {
public:
  ExecutionTrace(QTreeWidget& view, AstDiagram& diagram)
    : diagram(diagram), root(new QTreeWidgetItem(&view, {"execute", "Program"})), current(root) {
    root->setData(0, nodeIdRole, QVariant::fromValue(programNode));
  }

  QString apply(const CallEvent& event, bool stepping) {
    const bool returning = event.phase == CallPhase::Return;
    if (event.phase == CallPhase::Statement || event.phase == CallPhase::Expression) {
      const QString function = event.phase == CallPhase::Statement ? "executeStatement" : "evaluate";
      current = new QTreeWidgetItem(current, {function, diagram.nodeName(event.node)});
      current->setData(0, nodeIdRole, QVariant::fromValue(event.node));
    }

    auto* active = current;
    if (returning) {
      active->setText(2, event.value.isEmpty() ? QStringLiteral("Готово") : event.value);
      current = active->parent();
    }
    if (!stepping) return {};

    highlight(*active, event.value);
    constexpr qsizetype nodeLabelLimit = 65;
    QString name = active->text(1);
    if (name.size() > nodeLabelLimit) name = name.left(nodeLabelLimit) + "…";
    const QString call = active->text(0) + "(" + name + ")";
    if (returning) return "Возврат · " + call + " → " + active->text(2);
    return "Вход · " + call;
  }

  void finish(const QString& error = {}, bool stepping = false) {
    if (error.isEmpty()) {
      root->setText(2, "Готово");
      if (stepping) highlight(*root);
      return;
    }
    current->setText(2, error);
    current->setToolTip(2, error);
    current->setForeground(2, QColor("#ff9292"));
    if (stepping) highlight(*current);
    for (auto* parent = current->parent(); parent; parent = parent->parent()) {
      parent->setText(2, "Прервано");
    }
  }

private:
  AstDiagram& diagram;
  QTreeWidgetItem* root;
  QTreeWidgetItem* current;

  void highlight(QTreeWidgetItem& item, const QString& value = {}) {
    diagram.highlight(item.data(0, nodeIdRole).value<AstNodeId>(), value);
    item.treeWidget()->setCurrentItem(&item);
    item.setExpanded(true);
  }
};

class StepControl {
public:
  struct Stopped {};

  void pause() {
    QMutexLocker lock(&mutex);
    while (!ready && !stopped) changed.wait(&mutex);
    if (stopped) throw Stopped{};
    ready = false;
  }

  void advance() {
    QMutexLocker lock(&mutex);
    ready = true;
    changed.wakeOne();
  }

  void stop() {
    QMutexLocker lock(&mutex);
    stopped = true;
    changed.wakeOne();
  }

private:
  QMutex mutex;
  QWaitCondition changed;
  bool ready = false;
  bool stopped = false;
};

class MiniLangWindow : public QMainWindow {
public:
  MiniLangWindow() {
    setWindowTitle("MiniLang");
    resize(1250, 740);
    setMinimumSize(860, 560);
    setStyleSheet(R"(
      QWidget#workspace { background: #1d1d1d; }
      QLabel { color: #f2f2f2; }
      QLabel#title { font-size: 24px; font-weight: bold; }
      QLabel#status { color: #bdbdbd; }
      QLabel#stepInfo {
        color: #ffe2a0; background: #30291d; border: 1px solid #635139;
        border-radius: 8px; padding: 8px 10px;
      }
      QFrame[panel="true"] { background: #222222; border: 1px solid #414141; border-radius: 12px; }
      QPushButton {
        color: #f2f2f2; background: #262626; border: 1px solid #414141;
        border-radius: 10px; padding: 9px 18px;
      }
      QPushButton:hover { background: #323232; }
      QPushButton:pressed { background: #393939; }
      QPushButton:focus { border-color: #18ddec; }
      QPushButton#run { background: #1bdceb; color: #101a1b; border-color: #11c5d4; font-weight: bold; }
      QPushButton#run:hover { background: #5ceaf4; }
      QPushButton#run:pressed { background: #14bbd1; }
      QPushButton:disabled, QPushButton#run:disabled {
        color: #828282; background: #262626; border-color: #383838;
      }
      QTabWidget::pane { border: none; }
      QTabWidget::tab-bar { alignment: center; }
      QTabBar::tab {
        background: transparent; color: #f2f2f2; border: 1px solid transparent;
        border-radius: 17px; padding: 8px 10px; margin: 0 2px 8px;
      }
      QTabBar::tab:hover { background: #262c2d; }
      QTabBar::tab:selected { background: #1d3a3d; border-color: #18ddec; }
      QTabBar::tab:focus { border-color: #18ddec; }
      QPlainTextEdit, QTreeWidget {
        background: #1c1c1c; color: #f2f2f2; border: 1px solid #414141;
        border-radius: 8px; selection-background-color: #246068; selection-color: #ffffff;
      }
      QGraphicsView#astDiagram { border: 1px solid #414141; border-radius: 8px; }
      QPlainTextEdit { padding: 8px; }
      QPlainTextEdit:focus, QTreeWidget:focus { border-color: #18ddec; }
      QTreeWidget { alternate-background-color: #242424; }
      QTreeWidget::item { padding: 4px; }
      QHeaderView { background: #262626; color: #f2f2f2; }
      QHeaderView::section {
        background: #262626; color: #f2f2f2; border: 0; border-bottom: 1px solid #414141;
        padding: 7px;
      }
      QScrollBar:vertical, QScrollBar:horizontal { background: #202020; margin: 0; }
      QScrollBar:vertical { width: 12px; }
      QScrollBar:horizontal { height: 12px; }
      QScrollBar::handle { background: #505050; border: 2px solid #202020; border-radius: 5px; }
      QScrollBar::handle:hover { background: #656565; }
      QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }
      QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }
    )");

    auto* workspace = new QWidget;
    workspace->setObjectName("workspace");
    setCentralWidget(workspace);
    auto* layout = new QVBoxLayout(workspace);
    layout->setContentsMargins(20, 16, 20, 14);
    layout->setSpacing(12);

    auto* toolbar = new QHBoxLayout;
    auto* title = new QLabel("MiniLang");
    title->setObjectName("title");
    toolbar->addWidget(title);
    toolbar->addStretch();
    open = new QPushButton("Открыть .mini");
    open->setToolTip("Открыть файл (Ctrl+O)");
    toolbar->addWidget(open);
    run = new QPushButton("Запустить");
    run->setObjectName("run");
    run->setToolTip("Запустить программу (F5)");
    toolbar->addWidget(run);
    step = new QPushButton("Пошагово");
    step->setObjectName("step");
    step->setToolTip("Начать выполнение или перейти к следующему шагу (F8)");
    toolbar->addWidget(step);
    stop = new QPushButton("Остановить");
    stop->setObjectName("stop");
    stop->hide();
    toolbar->addWidget(stop);
    layout->addLayout(toolbar);

    const auto mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    source = new QPlainTextEdit;
    source->setObjectName("source");
    source->setAccessibleName("Код MiniLang");
    source->setFont(mono);
    source->setLineWrapMode(QPlainTextEdit::NoWrap);
    source->setPlainText(
      "remember count = 1\n"
      "repeat while count < 4\n"
      "  print (count + 2) * 3\n"
      "  remember count = count + 1\n"
      "end\n"
      "when count == 4\n"
      "  print \"готово\"\n"
      "otherwise\n"
      "  print \"другая ветвь\"\n"
      "end\n");
    output = new QPlainTextEdit;
    output->setAccessibleName("Вывод программы");
    output->setFont(mono);
    output->setReadOnly(true);
    output->setLineWrapMode(QPlainTextEdit::NoWrap);

    auto* editor = new QSplitter(Qt::Vertical);
    editor->addWidget(panel(source, "Исходный код"));
    editor->addWidget(panel(output, "Вывод print"));
    editor->setSizes({425, 155});
    editor->setChildrenCollapsible(false);

    tabs = new QTabWidget;
    tokens = addView("Токены", {"№", "Тип", "Лексема"}, false, {55, 145});
    statements = addView("Statements", {"Инструкция", "Тип", "Переменная"}, true, {230, 140});
    expressions = addView("Expressions", {"Выражение / узел", "Значение", "Узлы"}, true, {280, 135});
    addAstTab();
    trace = addView("Выполнение", {"Вызов", "Узел", "Результат"}, true, {210, 165});

    auto* content = new QSplitter(Qt::Horizontal);
    content->setChildrenCollapsible(false);
    content->addWidget(editor);
    content->addWidget(panel(tabs));
    content->setSizes({430, 775});
    layout->addWidget(content, 1);

    status = new QLabel;
    status->setObjectName("status");
    status->setTextFormat(Qt::PlainText);
    status->setWordWrap(true);
    layout->addWidget(status);

    connect(open, &QPushButton::clicked, this, [this] { openFile(); });
    connect(run, &QPushButton::clicked, this, [this] { runProgram(); });
    connect(step, &QPushButton::clicked, this, [this] {
      if (!stepControl) {
        runProgram(true);
        return;
      }
      step->setEnabled(false);
      stepControl->advance();
    });
    connect(stop, &QPushButton::clicked, this, [this] {
      step->setEnabled(false);
      stop->setEnabled(false);
      stepControl->stop();
    });
    auto* openShortcut = new QShortcut(QKeySequence::Open, this);
    connect(openShortcut, &QShortcut::activated, open, &QPushButton::click);
    auto* runShortcut = new QShortcut(QKeySequence(Qt::Key_F5), this);
    connect(runShortcut, &QShortcut::activated, run, &QPushButton::click);
    auto* stepShortcut = new QShortcut(QKeySequence(Qt::Key_F8), this);
    stepShortcut->setAutoRepeat(false);
    connect(stepShortcut, &QShortcut::activated, step, &QPushButton::click);
    connect(source, &QPlainTextEdit::textChanged, this, [this] {
      status->setText("Код изменён. Нажмите «Запустить», чтобы обновить результаты.");
    });
    runProgram();
  }

  ~MiniLangWindow() override {
    if (stepWorker) {
      stepControl->stop();
      stepWorker->wait();
    }
  }

private:
  QPlainTextEdit* source;
  QPlainTextEdit* output;
  QLabel* status;
  QLabel* stepInfo;
  QPushButton* open;
  QPushButton* run;
  QPushButton* step;
  QPushButton* stop;
  QTabWidget* tabs;
  QTreeWidget* tokens;
  QTreeWidget* statements;
  QTreeWidget* expressions;
  AstDiagram* ast;
  QWidget* astTab;
  QTreeWidget* trace;
  std::unique_ptr<ExecutionTrace> executionTrace;
  std::shared_ptr<StepControl> stepControl;
  std::unique_ptr<QThread> stepWorker;

  static QFrame* panel(QWidget* child, const QString& title = {}) {
    auto* frame = new QFrame;
    frame->setProperty("panel", true);
    auto* layout = new QVBoxLayout(frame);
    layout->setContentsMargins(12, 12, 12, 12);
    if (!title.isEmpty()) layout->addWidget(new QLabel(title));
    layout->addWidget(child, 1);
    return frame;
  }

  QTreeWidget* addView(const QString& title, const QStringList& headers, bool hierarchy,
                       std::initializer_list<int> widths) {
    auto* view = new QTreeWidget;
    view->setHeaderLabels(headers);
    view->setUniformRowHeights(true);
    view->setAlternatingRowColors(true);
    view->setRootIsDecorated(hierarchy);
    view->setAccessibleName(title);
    view->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    int column = 0;
    for (const int width : widths) view->setColumnWidth(column++, width);
    tabs->addTab(view, title);
    return view;
  }

  void addAstTab() {
    astTab = new QWidget;
    auto* layout = new QVBoxLayout(astTab);
    layout->setContentsMargins(0, 0, 0, 0);
    stepInfo = new QLabel;
    stepInfo->setObjectName("stepInfo");
    stepInfo->setTextFormat(Qt::PlainText);
    stepInfo->setTextInteractionFlags(Qt::TextSelectableByMouse);
    stepInfo->setWordWrap(true);
    stepInfo->hide();
    layout->addWidget(stepInfo);
    ast = new AstDiagram;
    layout->addWidget(ast, 1);

    auto* controls = new QHBoxLayout;
    controls->addStretch();
    auto* zoomOut = new QPushButton("−");
    zoomOut->setAccessibleName("Уменьшить AST");
    auto* zoomIn = new QPushButton("+");
    zoomIn->setAccessibleName("Увеличить AST");
    auto* fit = new QPushButton("Вместить дерево");
    controls->addWidget(zoomOut);
    controls->addWidget(zoomIn);
    controls->addWidget(fit);
    layout->addLayout(controls);

    connect(zoomOut, &QPushButton::clicked, ast, [this] { ast->zoom(1.0 / 1.2); });
    connect(zoomIn, &QPushButton::clicked, ast, [this] { ast->zoom(1.2); });
    connect(fit, &QPushButton::clicked, ast, &AstDiagram::fitTree);
    tabs->addTab(astTab, "AST");
  }

  void openFile() {
    const QString path = QFileDialog::getOpenFileName(this, "Открыть программу MiniLang", {},
                                                     "Программы MiniLang (*.mini);;Все файлы (*)");
    if (path.isEmpty()) return;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
      status->setText("Ошибка чтения файла. " + file.errorString());
      return;
    }
    const QByteArray bytes = file.readAll();
    if (file.error() != QFileDevice::NoError) {
      status->setText("Ошибка чтения файла. " + file.errorString());
      return;
    }
    source->setPlainText(QString::fromUtf8(bytes));
    runProgram();
  }

  void runProgram(bool stepping = false) {
    executionTrace.reset();
    for (auto* view : {tokens, statements, expressions, trace}) view->clear();
    ast->clear();
    output->clear();
    stepInfo->hide();

    QString stage = "лексического анализа";
    QWidget* errorTab = tokens;
    try {
      const auto lexed = Lexer::tokenize(source->toPlainText().toStdString());
      for (std::size_t i = 0; i < lexed.size(); ++i) {
        const auto& token = lexed[i];
        const QString lexeme = token.lexeme.empty() ? QStringLiteral("\"\"") : visibleText(token.lexeme);
        new QTreeWidgetItem(tokens, {QString::number(i + 1), tokenName(token.type), lexeme});
      }

      stage = "синтаксического анализа";
      errorTab = statements;
      auto program = Parser::parse(lexed);
      const AstViews views(program, *statements, *expressions, *ast);
      statements->expandAll();
      expressions->expandAll();

      stage = "выполнения";
      errorTab = trace;
      const QString summary = QString("Выполнение завершено · Токенов: %1 · Инструкций верхнего уровня: %2")
        .arg(lexed.size()).arg(program.size());
      executionTrace = std::make_unique<ExecutionTrace>(*trace, *ast);
      if (stepping) {
        startStepping(std::move(program), summary);
        return;
      }

      std::ostringstream result;
      try {
        ExecutionObserver::run(program, result, [this](const CallEvent& event) {
          executionTrace->apply(event, false);
        });
      } catch (const std::exception& error) {
        executionTrace->finish(QString::fromUtf8(error.what()));
        output->setPlainText(QString::fromStdString(result.str()));
        trace->expandAll();
        throw;
      }
      executionTrace->finish();
      trace->expandAll();
      output->setPlainText(QString::fromStdString(result.str()));
      status->setText(summary);
    } catch (const std::exception& error) {
      const QString message = QString::fromUtf8(error.what());
      status->setText(message.startsWith("Ошибка ") ? message : "Ошибка " + stage + ". " + message);
      tabs->setCurrentWidget(errorTab);
    }
  }

  void showStepMessage(const QString& message) {
    constexpr qsizetype limit = 150;
    stepInfo->setText(message.size() > limit ? message.left(limit) + "…" : message);
    stepInfo->setToolTip(message.toHtmlEscaped());
    stepInfo->show();
  }

  void appendOutput(const QString& text) {
    if (text.isEmpty()) return;
    output->moveCursor(QTextCursor::End);
    output->insertPlainText(text);
  }

  void setStepping(bool active) {
    source->setReadOnly(active);
    open->setEnabled(!active);
    run->setEnabled(!active);
    step->setText(active ? "Следующий шаг" : "Пошагово");
    step->setEnabled(!active);
    stop->setVisible(active);
    stop->setEnabled(active);
  }

  void startStepping(Program program, const QString& summary) {
    auto control = std::make_shared<StepControl>();
    stepWorker.reset(QThread::create([this, control, program = std::move(program), summary] {
      std::ostringstream stream;
      std::size_t printed = 0;
      const auto takeOutput = [&] {
        const auto buffer = stream.view();
        const auto appended = buffer.substr(printed);
        printed = buffer.size();
        return QString::fromUtf8(appended.data(), appended.size());
      };

      QString error;
      bool stopped = false;
      try {
        ExecutionObserver::run(program, stream, [&](const CallEvent& event) {
          QMetaObject::invokeMethod(this, [this, event, text = takeOutput()] {
            appendOutput(text);
            showStepMessage(executionTrace->apply(event, true));
            step->setEnabled(stop->isEnabled());
          }, Qt::QueuedConnection);
          // Рабочий поток ждёт команду, пока интерфейс показывает текущий вызов.
          control->pause();
        });
      } catch (const StepControl::Stopped&) {
        stopped = true;
      } catch (const std::exception& failure) {
        error = QString::fromUtf8(failure.what());
      }

      QMetaObject::invokeMethod(this, [this, stopped, error, text = takeOutput(), summary] {
        appendOutput(text);
        const QString interruption = stopped ? QStringLiteral("Выполнение остановлено") : error;
        executionTrace->finish(interruption, true);
        trace->expandAll();
        status->setText(interruption.isEmpty() ? summary : interruption);
        showStepMessage(interruption.isEmpty() ? QStringLiteral("Возврат · execute(Program) → Готово") : interruption);
        stepWorker->wait();
        stepWorker.reset();
        stepControl.reset();
        setStepping(false);
      }, Qt::QueuedConnection);
    }));
    stepControl = std::move(control);
    setStepping(true);
    showStepMessage("Начало выполнения");
    tabs->setCurrentWidget(astTab);
    status->setText("Пошаговое выполнение · F8 — следующий шаг");
    stepWorker->start();
  }
};

}

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  app.setApplicationName("MiniLang");
  MiniLangWindow window;
  window.show();
  return app.exec();
}
