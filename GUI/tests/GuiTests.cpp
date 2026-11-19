#include "Common.h"
#include "GraphScene.h"
#include <gtest/gtest.h>
#include <memory>

namespace {
void click(QWidget* widget, const QString& text) {
  for (auto* button : widget->findChildren<QPushButton*>()) {
    if (button->text() == text) {
      button->click();
      return;
    }
  }
  FAIL() << "Button not found: " << text.toStdString();
}
bool waitForWork(QWidget* widget) {
  QElapsedTimer timer;
  timer.start();
  while (!widget->isEnabled() && timer.elapsed() < 10000) {
    QApplication::processEvents();
    QThread::msleep(1);
  }
  QApplication::processEvents();
  return widget->isEnabled();
}
QString status(QWidget* widget) {
  return widget->findChild<QLabel*>("status")->text();
}
void screenshot(QWidget* widget, const QString& name) {
  const auto directory = qEnvironmentVariable("GUI_SCREENSHOTS");
  if (directory.isEmpty()) return;
  widget->resize(1200, 800);
  widget->show();
  QApplication::processEvents();
  ASSERT_TRUE(widget->grab().save(directory + "/" + name + ".png"));
}
}

TEST(GuiTests, AlphabeticalSearchMatchesAndEditingInvalidatesIndex) {
  std::unique_ptr<QWidget> widget(alphabeticalTab());
  click(widget.get(), "Построить");
  ASSERT_TRUE(waitForWork(widget.get()));
  auto* entries = widget->findChild<QTableWidget*>("wordEntries");
  ASSERT_EQ(entries->rowCount(), 3);
  widget->findChild<QLineEdit*>("wordQuery")->setText("кот");
  widget->findChild<QComboBox*>()->setCurrentIndex(2);
  click(widget.get(), "Найти");
  ASSERT_TRUE(waitForWork(widget.get()));
  EXPECT_TRUE(status(widget.get()).contains("Ответы совпадают"));
  EXPECT_TRUE(status(widget.get()).contains("Страницы: 1"));
  screenshot(widget.get(), "alphabetical");
  widget->findChild<QPlainTextEdit*>("sourceText")->setPlainText("новый текст");
  EXPECT_EQ(entries->rowCount(), 0);
  EXPECT_TRUE(status(widget.get()).contains("Данные изменены"));
}

TEST(GuiTests, PersonIndexFindsDuplicatesAndInclusiveRange) {
  std::unique_ptr<QWidget> widget(personTab());
  auto* source = widget->findChild<QTableWidget*>("peopleSource");
  row(source, {"1", "Ada", "Lovelace", "1815"});
  row(source, {"1", "Ada", "Lovelace", "1815"});
  row(source, {"2", "Grace", "Hopper", "1906"});
  click(widget.get(), "Построить индекс");
  ASSERT_TRUE(waitForWork(widget.get()));
  auto edits = widget->findChildren<QLineEdit*>(QString(), Qt::FindDirectChildrenOnly);
  for (auto* edit : edits) {
    if (edit->placeholderText() == "Имя") edit->setText("Ada");
    if (edit->placeholderText() == "Фамилия") edit->setText("Lovelace");
  }
  const auto spins = widget->findChildren<QSpinBox*>();
  ASSERT_EQ(spins.size(), 5);
  spins[2]->setValue(1815);
  click(widget.get(), "Точный поиск");
  ASSERT_TRUE(waitForWork(widget.get()));
  auto* results = widget->findChild<QTableWidget*>("peopleResults");
  EXPECT_EQ(results->rowCount(), 2);
  spins[3]->setValue(1815);
  spins[4]->setValue(1906);
  widget->findChild<QComboBox*>()->setCurrentIndex(2);
  click(widget.get(), "Найти диапазон");
  ASSERT_TRUE(waitForWork(widget.get()));
  EXPECT_EQ(results->rowCount(), 3);
  EXPECT_TRUE(status(widget.get()).contains("Ответы совпадают"));
  screenshot(widget.get(), "people");
  source->item(0, 1)->setText("Changed");
  EXPECT_EQ(results->rowCount(), 0);
  EXPECT_TRUE(status(widget.get()).contains("Данные изменены"));
}

TEST(GuiTests, GraphSearchComparesDistancesAndEditingClearsScene) {
  std::unique_ptr<QWidget> widget(graphTab());
  auto* edges = widget->findChild<QTableWidget*>("edges");
  row(edges, {"0", "1", "2"});
  row(edges, {"1", "7", "3"});
  row(edges, {"0", "7", "9"});
  click(widget.get(), "Построить");
  ASSERT_TRUE(waitForWork(widget.get()));
  widget->findChildren<QComboBox*>()[1]->setCurrentIndex(2);
  click(widget.get(), "Найти путь");
  ASSERT_TRUE(waitForWork(widget.get()));
  EXPECT_TRUE(status(widget.get()).contains("расстояние 5"));
  EXPECT_TRUE(status(widget.get()).contains("Расстояния совпадают"));
  auto* view = widget->findChild<QGraphicsView*>();
  ASSERT_FALSE(view->scene()->items().isEmpty());
  screenshot(widget.get(), "graph");
  edges->item(0, 2)->setText("4");
  EXPECT_TRUE(view->scene()->items().isEmpty());
}

TEST(GuiTests, GeneratedDirectedGraphCanBeSearched) {
  std::unique_ptr<QWidget> widget(graphTab());
  widget->findChildren<QComboBox*>()[0]->setCurrentIndex(1);
  click(widget.get(), "Сгенерировать");
  ASSERT_TRUE(waitForWork(widget.get()));
  ASSERT_GT(widget->findChild<QTableWidget*>("edges")->rowCount(), 0);
  click(widget.get(), "Найти путь");
  ASSERT_TRUE(waitForWork(widget.get()));
  EXPECT_TRUE(status(widget.get()).contains("расстояние"));
}

TEST(GuiTests, InvalidRangeShowsErrorAndPreservesResponsiveness) {
  std::unique_ptr<QWidget> widget(personTab());
  click(widget.get(), "Построить индекс");
  ASSERT_TRUE(waitForWork(widget.get()));
  auto spins = widget->findChildren<QSpinBox*>();
  spins[3]->setValue(2000);
  spins[4]->setValue(1900);
  bool shown = false;
  QTimer::singleShot(0, [&] {
    for (auto* top : QApplication::topLevelWidgets()) {
      if (auto* message = qobject_cast<QMessageBox*>(top)) {
        shown = message->text().contains("Начало диапазона");
        message->accept();
      }
    }
  });
  click(widget.get(), "Найти диапазон");
  EXPECT_TRUE(shown);
  EXPECT_TRUE(widget->isEnabled());
}

TEST(GuiTests, CsvPreservesUtf8QuotesCommasAndNewlines) {
  std::unique_ptr<QTableWidget> source(table({"Слово", "Значение"}));
  row(source.get(), {"кот, \"мир\"", "строка\nдругая"});
  QTemporaryDir directory;
  ASSERT_TRUE(directory.isValid());
  const auto path = directory.filePath("result.csv");
  saveTableCsv(source.get(), path);
  QFile file(path);
  ASSERT_TRUE(file.open(QIODevice::ReadOnly));
  EXPECT_EQ(QString::fromUtf8(file.readAll()),
            "\"Слово\",\"Значение\"\n\"кот, \"\"мир\"\"\",\"строка\nдругая\"\n");
  EXPECT_THROW(saveTableCsv(source.get(), directory.filePath("missing/result.csv")), std::runtime_error);
}

TEST(GuiTests, WorkerErrorRestoresControlsForNegativeEdges) {
  std::unique_ptr<QWidget> widget(graphTab());
  widget->findChildren<QComboBox*>()[0]->setCurrentIndex(1);
  auto* edges = widget->findChild<QTableWidget*>("edges");
  row(edges, {"0", "7", "-1"});
  click(widget.get(), "Построить");
  ASSERT_TRUE(waitForWork(widget.get()));
  bool shown = false;
  QTimer dismiss;
  QObject::connect(&dismiss, &QTimer::timeout, [&] {
    for (auto* top : QApplication::topLevelWidgets()) {
      if (auto* message = qobject_cast<QMessageBox*>(top)) {
        shown = true;
        message->accept();
      }
    }
  });
  dismiss.start(1);
  click(widget.get(), "Найти путь");
  ASSERT_TRUE(waitForWork(widget.get()));
  EXPECT_TRUE(shown);
  EXPECT_EQ(status(widget.get()), "Ошибка");
  widget->findChildren<QComboBox*>()[1]->setCurrentIndex(1);
  click(widget.get(), "Найти путь");
  ASSERT_TRUE(waitForWork(widget.get()));
  EXPECT_TRUE(status(widget.get()).contains("расстояние -1"));
}

int main(int argc, char** argv) {
  QApplication app(argc, argv);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
