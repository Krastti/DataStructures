#include "../include/Common.h"
#include "../include/IndexAppearance.h"
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
  indexAppearance::stylePage(root);
  auto* layout = new QVBoxLayout(root);
  layout->setContentsMargins(24, 16, 24, 18);
  layout->setSpacing(10);
  auto* workspace = new QHBoxLayout;
  workspace->setSpacing(16);
  layout->addLayout(workspace, 1);

  auto* split = new QSplitter;
  split->setChildrenCollapsible(false);
  workspace->addWidget(split, 1);
  auto* source = new QWidget;
  auto* sourceLayout = new QVBoxLayout(source);
  sourceLayout->setContentsMargins(0, 0, 10, 0);
  auto* sourceBar = new QHBoxLayout;
  indexAppearance::heading(sourceBar, "Исходный текст");
  sourceBar->addStretch();
  sourceLayout->addLayout(sourceBar);
  auto* text = new QPlainTextEdit("кот мир\nдом кот");
  text->setObjectName("sourceText");
  text->setAccessibleName("Исходный текст");
  text->setPlaceholderText("Введите текст или загрузите TXT");
  sourceLayout->addWidget(text);
  split->addWidget(source);

  auto* results = new QWidget;
  auto* resultLayout = new QVBoxLayout(results);
  resultLayout->setContentsMargins(10, 0, 0, 0);
  auto* indexBar = new QHBoxLayout;
  indexAppearance::heading(indexBar, "Указатель");
  indexBar->addStretch();
  resultLayout->addLayout(indexBar);
  auto* entries = table({"Слово", "Страницы"});
  entries->setObjectName("wordEntries");
  indexAppearance::styleTable(entries);
  auto* page = indexAppearance::number(1, 1, 1);
  auto* pageControls = new QHBoxLayout;
  auto* pageLabel = new QLabel("&Страница");
  pageLabel->setBuddy(page);
  pageControls->addWidget(pageLabel);
  pageControls->addWidget(page);
  pageControls->addStretch();
  auto* preview = new QPlainTextEdit;
  preview->setAccessibleName("Просмотр страницы");
  preview->setReadOnly(true);
  resultLayout->addWidget(entries, 2);
  resultLayout->addLayout(pageControls);
  resultLayout->addWidget(preview, 1);
  split->addWidget(results);
  split->setStretchFactor(0, 1);
  split->setStretchFactor(1, 1);

  workspace->addWidget(indexAppearance::divider(Qt::Vertical));
  auto* sidebar = new QVBoxLayout;
  sidebar->setSpacing(8);
  workspace->addLayout(sidebar);
  indexAppearance::heading(sidebar, "Построение указателя");
  auto* size = indexAppearance::number(2, 1000000, 80);
  auto* sizeLabel = new QLabel("С&имволов на странице");
  sizeLabel->setBuddy(size);
  sidebar->addWidget(sizeLabel);
  sidebar->addWidget(size);
  auto* controls = new QVBoxLayout;
  sidebar->addLayout(controls);
  sidebar->addSpacing(8);
  sidebar->addWidget(indexAppearance::divider(Qt::Horizontal));
  sidebar->addSpacing(8);

  auto* search = new QVBoxLayout;
  sidebar->addLayout(search);
  indexAppearance::heading(search, "Поиск слова");
  auto* query = new QLineEdit;
  query->setObjectName("wordQuery");
  query->setAccessibleName("Слово для поиска");
  query->setPlaceholderText("Слово для поиска");
  auto* algorithm = new indexAppearance::SearchModeInput;
  algorithm->setAccessibleName("Метод поиска слова");
  algorithm->addItems({"Бинарный поиск", "Красно-чёрное дерево", "Сравнить"});
  search->addWidget(query);
  search->addWidget(algorithm);
  sidebar->addStretch();

  layout->addWidget(indexAppearance::divider(Qt::Horizontal));
  auto* measurementBar = new QHBoxLayout;
  indexAppearance::heading(measurementBar, "Замеры");
  measurementBar->addStretch();
  layout->addLayout(measurementBar);
  auto* status = new QLabel("Введите текст и постройте указатель");
  status->setWordWrap(true);
  status->setObjectName("status");
  layout->addWidget(status);
  auto* metrics = table({"Операция", "Результат и время"});
  indexAppearance::styleTable(metrics);
  metrics->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  metrics->setFixedHeight(120);
  layout->addWidget(metrics);
  button("Экспорт замеров", measurementBar, [=] { exportTable(root, metrics); });
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
  button("Загрузить TXT", sourceBar, [=] {
    const QString path = QFileDialog::getOpenFileName(root, "Открыть текст", {}, "Текст (*.txt);;Все файлы (*)");
    if (path.isEmpty()) return;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) throw std::runtime_error(file.errorString().toStdString());
    text->setPlainText(QString::fromUtf8(file.readAll()));
  });
  auto* build = button("Построить", controls, [=] {
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
  build->setProperty("indexPrimary", true);
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
  button("Экспорт указателя", indexBar, [=] { exportTable(root, entries); });
  return root;
}
