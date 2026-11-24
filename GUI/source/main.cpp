#include "../include/Common.h"

int main(int argc, char** argv) {
  QApplication app(argc, argv);
  QCoreApplication::setApplicationName("DataStructures");
  QMainWindow window;
  window.setWindowTitle("Структуры данных — ЛР-2 и ЛР-3");
  auto* tabs = new QTabWidget;
  auto palette = tabs->palette();
  palette.setColor(QPalette::Window, QColor("#1d1d1d"));
  tabs->setPalette(palette);
  tabs->setAutoFillBackground(true);
  tabs->setObjectName("labTabs");
  tabs->tabBar()->setObjectName("labNavigation");
  tabs->setStyleSheet(R"(
    QTabWidget#labTabs::pane { border: none; }
    QTabWidget#labTabs::tab-bar { alignment: center; }
    QTabBar#labNavigation::tab {
      background: #1d1d1d; color: #f2f2f2; border: 1px solid #474747;
      border-radius: 18px; padding: 9px 20px; margin: 8px 3px 4px;
      font: 14px Arial;
    }
    QTabBar#labNavigation::tab:hover { background: #262c2d; }
    QTabBar#labNavigation::tab:selected {
      background: #1d3a3d; border-color: #18ddec;
    }
    QTabBar#labNavigation::tab:focus { border-color: #f2f2f2; }
  )");
  tabs->addTab(alphabeticalTab(), "Алфавитный указатель");
  tabs->addTab(personTab(), "Персональный индекс");
  auto* graphs = graphTab();
  auto graphPalette = graphs->palette();
  graphPalette.setColor(QPalette::Window, window.palette().color(QPalette::Window));
  graphs->setPalette(graphPalette);
  graphs->setAutoFillBackground(true);
  tabs->addTab(graphs, "Графы");
  window.setCentralWidget(tabs);
  window.resize(1280, 760);
  window.show();
  return app.exec();
}
