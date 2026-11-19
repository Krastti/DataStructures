#include "../include/Common.h"

int main(int argc, char** argv) {
  QApplication app(argc, argv);
  QCoreApplication::setApplicationName("DataStructures");
  QMainWindow window;
  window.setWindowTitle("Структуры данных — ЛР-2 и ЛР-3");
  auto* tabs = new QTabWidget;
  tabs->addTab(alphabeticalTab(), "Алфавитный указатель");
  tabs->addTab(personTab(), "Персональный индекс");
  tabs->addTab(graphTab(), "Графы");
  window.setCentralWidget(tabs);
  window.resize(1200, 800);
  window.show();
  return app.exec();
}
