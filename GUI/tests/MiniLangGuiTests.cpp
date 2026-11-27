#define MINILANG_GUI_TESTING
#include "../source/MiniLangGui.cpp"

#include <gtest/gtest.h>
#include <QElapsedTimer>
#include <QLineEdit>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidgetItemIterator>

namespace {

bool waitUntil(const std::function<bool()>& done) {
  QElapsedTimer timer;
  timer.start();
  while (!done() && timer.elapsed() < 5000) {
    QApplication::processEvents();
    QThread::msleep(1);
  }
  QApplication::processEvents();
  return done();
}

class MiniLangGuiTests : public ::testing::Test {
protected:
  QTemporaryDir directory;

  void write(const QString& name, const QByteArray& text) {
    QFile file(directory.filePath(name));
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    ASSERT_EQ(file.write(text), text.size());
  }

  QPlainTextEdit* editor(MiniLangWindow& window) {
    return window.findChild<QPlainTextEdit*>("source");
  }

  QPlainTextEdit* output(MiniLangWindow& window) {
    for (auto* widget : window.findChildren<QPlainTextEdit*>()) {
      if (widget->accessibleName() == "Вывод программы") return widget;
    }
    return nullptr;
  }

  QLabel* status(MiniLangWindow& window) { return window.findChild<QLabel*>("status"); }

  QTreeWidget* view(MiniLangWindow& window, const QString& name) {
    for (auto* widget : window.findChildren<QTreeWidget*>()) {
      if (widget->accessibleName() == name) return widget;
    }
    return nullptr;
  }

  void click(MiniLangWindow& window, const QString& text) {
    for (auto* button : window.findChildren<QPushButton*>()) {
      if (button->text() == text) {
        button->click();
        return;
      }
    }
    FAIL() << text.toStdString();
  }

  void dialogAction(const std::function<void()>& action, const QString& selected, bool cancel = false) {
    bool shown = false;
    QTimer answer;
    QObject::connect(&answer, &QTimer::timeout, [&] {
      auto* dialog = qobject_cast<QFileDialog*>(QApplication::activeModalWidget());
      if (!dialog) return;
      if (cancel) {
        shown = true;
        dialog->reject();
        return;
      }
      if (!shown) {
        if (dialog->fileMode() == QFileDialog::Directory) dialog->setDirectory(selected);
        else {
          dialog->setDirectory(QFileInfo(selected).absolutePath());
        }
        shown = true;
      }
      // Модель файлов загружается асинхронно, выбор повторяется после её обновления.
      if (dialog->fileMode() != QFileDialog::Directory) {
        auto* filename = dialog->findChild<QLineEdit*>("fileNameEdit");
        if (filename) filename->setText(selected);
      }
      QMetaObject::invokeMethod(dialog, "accept", Qt::QueuedConnection);
    });
    answer.start(10);
    QTimer deadline;
    deadline.setSingleShot(true);
    QObject::connect(&deadline, &QTimer::timeout, [&] {
      if (auto* dialog = qobject_cast<QFileDialog*>(QApplication::activeModalWidget())) dialog->reject();
      ADD_FAILURE() << "Диалог не завершился вовремя";
    });
    deadline.start(3000);
    action();
    EXPECT_TRUE(shown);
  }

  void open(MiniLangWindow& window) {
    dialogAction([&] { click(window, "Открыть .mini"); }, directory.filePath("main.mini"));
  }

  void screenshot(MiniLangWindow& window, const QString& name) {
    const auto destination = qEnvironmentVariable("GUI_SCREENSHOTS");
    if (destination.isEmpty()) return;
    window.show();
    QApplication::processEvents();
    click(window, "Вместить дерево");
    ASSERT_TRUE(window.grab().save(destination + "/" + name + ".png"));
  }
};

TEST_F(MiniLangGuiTests, OpenFileUsesItsDirectoryAndEditedText) {
  write("math.mini", "function square(x) -> x * x end");
  write("main.mini", "using \"math.mini\"\nprint square(5)");
  MiniLangWindow window;
  open(window);
  EXPECT_EQ(output(window)->toPlainText(), "25.0\n");
  EXPECT_EQ(view(window, "Токены")->topLevelItem(0)->text(1), "Using");
  EXPECT_EQ(view(window, "Statements")->topLevelItemCount(), 2);
  editor(window)->setPlainText("using \"math.mini\"\nprint square(6)");
  click(window, "Запустить");
  EXPECT_EQ(output(window)->toPlainText(), "36.0\n");
  write("math.mini", "function square(x) -> x * x + 1 end");
  click(window, "Запустить");
  EXPECT_EQ(output(window)->toPlainText(), "37.0\n");
  screenshot(window, "minilang-using");
}

TEST_F(MiniLangGuiTests, UnsavedTextChoosesAndRemembersBaseDirectory) {
  write("math.mini", "function square(x) -> x * x end");
  MiniLangWindow window;
  editor(window)->setPlainText("using \"math.mini\"\nprint square(5)");
  dialogAction([&] { click(window, "Запустить"); }, directory.path());
  EXPECT_EQ(output(window)->toPlainText(), "25.0\n");
  click(window, "Запустить");
  EXPECT_EQ(output(window)->toPlainText(), "25.0\n");
}

TEST_F(MiniLangGuiTests, CancelDirectoryChoiceCancelsNormalAndSteppedRuns) {
  MiniLangWindow window;
  editor(window)->setPlainText("using \"math.mini\"\nprint 123");
  for (const auto* action : {"Запустить", "Пошагово"}) {
    dialogAction([&] { click(window, action); }, {}, true);
    EXPECT_TRUE(status(window)->text().contains("Запуск отменён"));
    EXPECT_TRUE(output(window)->toPlainText().isEmpty());
    EXPECT_TRUE(window.findChild<QPushButton*>("run")->isEnabled());
  }
}

TEST_F(MiniLangGuiTests, AbsoluteImportsWorkWithoutDirectoryChoice) {
  write("math.mini", "function square(x) -> x * x end");
  MiniLangWindow window;
  editor(window)->setPlainText("using \"" + directory.filePath("math.mini") + "\"\nprint square(5)");
  click(window, "Запустить");
  EXPECT_EQ(output(window)->toPlainText(), "25.0\n");
}

TEST_F(MiniLangGuiTests, InvalidDirectiveIsReportedBeforeDirectoryChoice) {
  MiniLangWindow window;
  editor(window)->setPlainText("using \"math.mini\" extra");
  click(window, "Запустить");
  EXPECT_TRUE(status(window)->text().contains("Ошибка синтаксического анализа"));
  EXPECT_TRUE(output(window)->toPlainText().isEmpty());
}

TEST_F(MiniLangGuiTests, DependencyFailurePreservesTokensAndPreventsExecution) {
  write("main.mini", "using \"math.mini\"\nprint 123");
  write("math.mini", "print 456");
  MiniLangWindow window;
  open(window);
  EXPECT_TRUE(status(window)->text().contains("Ошибка синтаксического анализа"));
  EXPECT_TRUE(status(window)->text().contains("math.mini"));
  EXPECT_TRUE(status(window)->text().contains("Цепочка подключений"));
  EXPECT_TRUE(output(window)->toPlainText().isEmpty());
  EXPECT_EQ(view(window, "Токены")->topLevelItem(0)->text(1), "Using");
  EXPECT_EQ(view(window, "Statements")->topLevelItemCount(), 0);
}

TEST_F(MiniLangGuiTests, StepsIntoLibraryFunctionAndKeepsAstAfterCompletion) {
  write("main.mini", "using \"math.mini\"\nprint square(5)");
  write("math.mini", "function square(x) -> x * x end");
  MiniLangWindow window;
  open(window);
  click(window, "Пошагово");
  auto* run = window.findChild<QPushButton*>("run");
  auto* step = window.findChild<QPushButton*>("step");
  bool visitedProduct = false;
  int steps = 0;
  while (!run->isEnabled() && steps++ < 100) {
    ASSERT_TRUE(waitUntil([&] { return step->isEnabled() || run->isEnabled(); }));
    if (run->isEnabled()) break;
    if (window.findChild<QLabel*>("stepInfo")->text().contains("Binary")) {
      visitedProduct = true;
      screenshot(window, "minilang-using-step");
    }
    step->click();
  }
  ASSERT_TRUE(run->isEnabled());
  EXPECT_TRUE(visitedProduct);
  EXPECT_EQ(output(window)->toPlainText(), "25.0\n");
  EXPECT_TRUE(status(window)->text().contains("Выполнение завершено"));
  EXPECT_EQ(view(window, "Statements")->topLevelItemCount(), 2);
  screenshot(window, "minilang-using-finished");
  click(window, "Запустить");
  EXPECT_EQ(output(window)->toPlainText(), "25.0\n");
}

TEST_F(MiniLangGuiTests, DisplaysAndStepsThroughSignedNumberLiterals) {
  MiniLangWindow window;
  editor(window)->setPlainText("remember x = -2\nprint x * -0.5");
  click(window, "Запустить");
  EXPECT_EQ(output(window)->toPlainText(), "1.0\n");
  click(window, "Пошагово");
  auto* run = window.findChild<QPushButton*>("run");
  auto* step = window.findChild<QPushButton*>("step");
  bool visitedSignedLiteral = false;
  int steps = 0;
  while (!run->isEnabled() && steps++ < 100) {
    ASSERT_TRUE(waitUntil([&] { return step->isEnabled() || run->isEnabled(); }));
    if (run->isEnabled()) break;
    if (window.findChild<QLabel*>("stepInfo")->text().contains("NumberLiteral -")) {
      visitedSignedLiteral = true;
      screenshot(window, "minilang-negative-step");
    }
    step->click();
  }
  ASSERT_TRUE(run->isEnabled());
  EXPECT_TRUE(visitedSignedLiteral);
  EXPECT_EQ(output(window)->toPlainText(), "1.0\n");
}

TEST_F(MiniLangGuiTests, StopAndCloseWhileSteppingReleaseWorker) {
  write("main.mini", "using \"math.mini\"\nprint square(5)");
  write("math.mini", "function square(x) -> x * x end");
  MiniLangWindow window;
  open(window);
  click(window, "Пошагово");
  auto* step = window.findChild<QPushButton*>("step");
  ASSERT_TRUE(waitUntil([&] { return step->isEnabled(); }));
  click(window, "Остановить");
  auto* run = window.findChild<QPushButton*>("run");
  ASSERT_TRUE(waitUntil([&] { return run->isEnabled(); }));
  EXPECT_TRUE(status(window)->text().contains("Выполнение остановлено"));
  click(window, "Пошагово");
  ASSERT_TRUE(waitUntil([&] { return step->isEnabled(); }));
}
}

int main(int argc, char** argv) {
  QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
  QApplication app(argc, argv);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
