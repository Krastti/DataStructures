#include "../include/Common.h"
#include "AlphabeticalIndex.h"
#include <memory>

namespace {
struct Result {
  QString error;
  std::shared_ptr<AlphabeticalIndex> index;
  Array<size_t> pages;
  QString summary;
};
}

QWidget* alphabeticalTab() {
  auto* root = new QWidget;
  auto* layout = new QVBoxLayout(root);
  auto* controls = new QHBoxLayout;
  auto* size = number(2, 1000000, 80);
  controls->addWidget(new QLabel("Символов на странице:"));
  controls->addWidget(size);
  layout->addLayout(controls);
  auto* split = new QSplitter;
  auto* text = new QPlainTextEdit("кот мир\nдом кот");
  text->setObjectName("sourceText");
  text->setPlaceholderText("Введите текст или загрузите TXT");
  split->addWidget(text);
  auto* results = new QWidget;
  auto* resultLayout = new QVBoxLayout(results);
  auto* entries = table({"Слово", "Страницы"});
  entries->setObjectName("wordEntries");
  auto* page = number(1, 1, 1);
  auto* preview = new QPlainTextEdit;
  preview->setReadOnly(true);
  resultLayout->addWidget(entries);
  resultLayout->addWidget(new QLabel("Просмотр страницы:"));
  resultLayout->addWidget(page);
  resultLayout->addWidget(preview);
  split->addWidget(results);
  layout->addWidget(split);
  auto* search = new QHBoxLayout;
  auto* query = new QLineEdit;
  query->setObjectName("wordQuery");
  query->setPlaceholderText("Слово для поиска");
  auto* algorithm = new QComboBox;
  algorithm->addItems({"Бинарный поиск", "Красно-чёрное дерево", "Сравнить"});
  search->addWidget(query);
  search->addWidget(algorithm);
  layout->addLayout(search);
  auto* status = new QLabel("Введите текст и постройте указатель");
  status->setWordWrap(true);
  status->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
  status->setObjectName("status");
  layout->addWidget(status);
  auto* metrics = table({"Операция", "Результат и время"});
  metrics->setMaximumHeight(110);
  layout->addWidget(metrics);
  button("Экспорт замеров", controls, [=] { exportTable(root, metrics); });
  auto state = std::make_shared<std::shared_ptr<AlphabeticalIndex>>();
  auto invalidate = [=] {
    state->reset();
    metrics->setRowCount(0);
    entries->setRowCount(0);
    preview->clear();
    status->setText("Данные изменены — постройте указатель");
  };
  QObject::connect(text, &QPlainTextEdit::textChanged, root, invalidate);
  QObject::connect(size, &QSpinBox::valueChanged, root, invalidate);
  button("Загрузить TXT", controls, [=] {
    const QString path = QFileDialog::getOpenFileName(root, "Открыть текст", {}, "Текст (*.txt);;Все файлы (*)");
    if (path.isEmpty()) return;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) throw std::runtime_error(file.errorString().toStdString());
    text->setPlainText(QString::fromUtf8(file.readAll()));
  });
  button("Построить", controls, [=] {
    const QString source = text->toPlainText();
    const int capacity = size->value();
    background(root, status, [=] {
      Result result;
      Array<std::string> lines;
      for (const auto& line : source.split('\n')) lines.push_back(line.toStdString());
      double elapsed;
      result.index = measured([&] {
        return std::make_shared<AlphabeticalIndex>(AlphabeticalIndex::build(lines, capacity));
      }, elapsed);
      result.summary = QString("Построение: %1 мс; страниц: %2").arg(elapsed, 0, 'f', 3).arg(result.index->pages().size());
      return result;
    }, [=](const Result& result) {
      *state = result.index;
      entries->setRowCount(0);
      for (size_t i = 0; i < (*state)->entries().size(); ++i) {
        const auto& entry = (*state)->entries()[i];
        QStringList pages;
        for (size_t j = 0; j < entry.pages.size(); ++j) pages << QString::number(entry.pages[j]);
        row(entries, {utf(entry.word), pages.join(", ")});
      }
      page->setMaximum(static_cast<int>((*state)->pages().size()));
      page->setValue(1);
      preview->setPlainText(utf((*state)->pages()[0]));
      status->setText(result.summary);
      row(metrics, {"Замер", result.summary});
    });
  });
  QObject::connect(page, &QSpinBox::valueChanged, root, [=](int value) {
    if (*state) preview->setPlainText(utf((*state)->pages()[value - 1]));
  });
  button("Найти", search, [=] {
    if (!*state) throw std::invalid_argument("Сначала постройте указатель");
    auto index = *state;
    const auto word = query->text().toStdString();
    const int mode = algorithm->currentIndex();
    background(root, status, [=] {
      Result result;
      double binaryTime = 0, treeTime = 0;
      Array<size_t> binary, tree;
      if (mode != 1) binary = measured([&] { return index->findBinary(word); }, binaryTime);
      if (mode != 0) tree = measured([&] { return index->findTree(word); }, treeTime);
      result.pages = mode == 1 ? tree : binary;
      QStringList parts;
      if (mode != 1) parts << QString("Бинарный: %1 мс").arg(binaryTime, 0, 'f', 6);
      if (mode != 0) parts << QString("Дерево: %1 мс").arg(treeTime, 0, 'f', 6);
      if (mode == 2) {
        bool equal = binary.size() == tree.size();
        for (size_t i = 0; equal && i < binary.size(); ++i) equal = binary[i] == tree[i];
        parts << (equal ? "Ответы совпадают" : "Ответы различаются");
      }
      QStringList pages;
      for (size_t i = 0; i < result.pages.size(); ++i) pages << QString::number(result.pages[i]);
      parts << ("Страницы: " + (pages.isEmpty() ? QString("не найдено") : pages.join(", ")));
      result.summary = parts.join("; ");
      return result;
    }, [=](const Result& result) {
      status->setText(result.summary);
      row(metrics, {"Замер", result.summary});
      if (result.pages.size()) {
        page->setValue(static_cast<int>(result.pages[0]));
        preview->moveCursor(QTextCursor::Start);
        preview->find(query->text());
      }
    });
  });
  QObject::connect(entries, &QTableWidget::cellClicked, root, [=](int r, int) {
    query->setText(cell(entries, r, 0));
    page->setValue(integer(cell(entries, r, 1).section(',', 0, 0)));
    preview->moveCursor(QTextCursor::Start);
    preview->find(query->text());
  });
  button("Экспорт указателя", controls, [=] { exportTable(root, entries); });
  return root;
}
