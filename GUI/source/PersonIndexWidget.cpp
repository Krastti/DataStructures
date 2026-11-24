#include "../include/Common.h"
#include "../include/IndexAppearance.h"
#include "PersonIndex.h"
#include <memory>
#include <random>

namespace {
struct Result {
  QString error;
  std::shared_ptr<PersonIndex> index;
  Array<PersonRecord> matches;
  Array<PersonRecord> generated;
  QString summary;
};
void showPeople(QTableWidget* target, const Array<PersonRecord>& people) {
  target->setRowCount(0);
  for (size_t i = 0; i < people.size(); ++i) {
    const auto& p = people[i];
    row(target, {QString::number(p.id), utf(p.firstName), utf(p.lastName), QString::number(p.birthYear)});
  }
}
}

QWidget* personTab() {
  auto* root = new QWidget;
  indexAppearance::stylePage(root);
  auto* layout = new QVBoxLayout(root);
  layout->setContentsMargins(24, 16, 24, 18);
  layout->setSpacing(10);
  auto* workspace = new QHBoxLayout;
  workspace->setSpacing(16);
  layout->addLayout(workspace, 1);

  auto* split = new QSplitter(Qt::Vertical);
  split->setChildrenCollapsible(false);
  workspace->addWidget(split, 1);
  auto* sourcePanel = new QWidget;
  auto* sourceLayout = new QVBoxLayout(sourcePanel);
  sourceLayout->setContentsMargins(0, 0, 0, 6);
  indexAppearance::heading(sourceLayout, "Исходные записи");
  auto* source = table({"ID", "Имя", "Фамилия", "Год"}, true);
  source->setObjectName("peopleSource");
  indexAppearance::styleTable(source);
  sourceLayout->addWidget(source, 1);
  auto* sourceTools = new QHBoxLayout;
  sourceLayout->addLayout(sourceTools);
  split->addWidget(sourcePanel);
  auto* resultPanel = new QWidget;
  auto* resultLayout = new QVBoxLayout(resultPanel);
  resultLayout->setContentsMargins(0, 6, 0, 0);
  auto* resultBar = new QHBoxLayout;
  indexAppearance::heading(resultBar, "Результат поиска");
  resultBar->addStretch();
  resultLayout->addLayout(resultBar);
  auto* output = table({"ID", "Имя", "Фамилия", "Год"});
  output->setObjectName("peopleResults");
  indexAppearance::styleTable(output);
  resultLayout->addWidget(output, 1);
  split->addWidget(resultPanel);
  split->setStretchFactor(0, 1);
  split->setStretchFactor(1, 1);

  workspace->addWidget(indexAppearance::divider(Qt::Vertical));
  auto* sidebar = new QVBoxLayout;
  sidebar->setSpacing(8);
  workspace->addLayout(sidebar);
  indexAppearance::heading(sidebar, "Подготовка индекса");
  auto* count = indexAppearance::number(1, 100000, 100);
  auto* seed = indexAppearance::number(0, 1000000, 1);
  auto* generationFields = new QGridLayout;
  auto* countLabel = new QLabel("Число записей");
  countLabel->setBuddy(count);
  auto* seedLabel = new QLabel("Seed");
  seedLabel->setBuddy(seed);
  generationFields->addWidget(countLabel, 0, 0);
  generationFields->addWidget(seedLabel, 0, 1);
  generationFields->addWidget(count, 1, 0);
  generationFields->addWidget(seed, 1, 1);
  sidebar->addLayout(generationFields);
  auto* tools = new QHBoxLayout;
  sidebar->addLayout(tools);
  sidebar->addWidget(indexAppearance::divider(Qt::Horizontal));

  indexAppearance::heading(sidebar, "Точный поиск");
  auto* names = new QHBoxLayout;
  auto* first = new QLineEdit;
  first->setPlaceholderText("Имя");
  first->setAccessibleName("Имя");
  auto* last = new QLineEdit;
  last->setPlaceholderText("Фамилия");
  last->setAccessibleName("Фамилия");
  names->addWidget(first);
  names->addWidget(last);
  sidebar->addLayout(names);
  auto* exact = new QHBoxLayout;
  auto* year = indexAppearance::number(-10000, 10000, 2000);
  auto* yearLabel = new QLabel("Год");
  yearLabel->setBuddy(year);
  exact->addWidget(yearLabel);
  exact->addWidget(year, 1);
  sidebar->addLayout(exact);
  sidebar->addWidget(indexAppearance::divider(Qt::Horizontal));

  indexAppearance::heading(sidebar, "Поиск по диапазону");
  auto* years = new QHBoxLayout;
  auto* from = indexAppearance::number(-10000, 10000, 1980);
  auto* to = indexAppearance::number(-10000, 10000, 2000);
  auto* fromLabel = new QLabel("С");
  fromLabel->setBuddy(from);
  from->setAccessibleName("Начальный год");
  auto* toLabel = new QLabel("по");
  toLabel->setBuddy(to);
  to->setAccessibleName("Конечный год");
  years->addWidget(fromLabel);
  years->addWidget(from, 1);
  years->addWidget(toLabel);
  years->addWidget(to, 1);
  sidebar->addLayout(years);
  auto* range = new QHBoxLayout;
  auto* mode = new indexAppearance::SearchModeInput;
  mode->setAccessibleName("Метод поиска по диапазону");
  mode->addItems({"Индекс", "Полный просмотр", "Сравнить"});
  range->addWidget(mode, 1);
  sidebar->addLayout(range);
  sidebar->addStretch();

  layout->addWidget(indexAppearance::divider(Qt::Horizontal));
  auto* measurementBar = new QHBoxLayout;
  indexAppearance::heading(measurementBar, "Замеры");
  measurementBar->addStretch();
  layout->addLayout(measurementBar);
  auto* status = new QLabel("Добавьте записи или сгенерируйте набор");
  status->setWordWrap(true);
  status->setObjectName("status");
  layout->addWidget(status);
  auto* metrics = table({"Операция", "Результат и время"});
  indexAppearance::styleTable(metrics);
  metrics->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  metrics->setFixedHeight(120);
  layout->addWidget(metrics);
  button("Экспорт замеров", measurementBar, [=] { exportTable(root, metrics); });
  auto index = std::make_shared<std::shared_ptr<PersonIndex>>();
  auto people = std::make_shared<Array<PersonRecord>>();
  QObject::connect(source, &QTableWidget::itemChanged, root, [=] {
    index->reset();
    metrics->setRowCount(0);
    output->setRowCount(0);
    status->setText("Данные изменены — перестройте индекс");
  });
  button("Добавить строку", sourceTools, [=] { row(source, {"0", "", "", "2000"}); });
  button("Удалить строку", sourceTools, [=] {
    if (source->currentRow() >= 0) {
      source->removeRow(source->currentRow());
      index->reset();
      metrics->setRowCount(0);
      output->setRowCount(0);
      status->setText("Данные изменены — перестройте индекс");
    }
  });
  button("Сгенерировать", tools, [=] {
    const int amount = count->value(), randomSeed = seed->value();
    background(root, status, [=] {
      Result result;
      std::mt19937 random(randomSeed);
      for (int i = 0; i < amount; ++i)
        result.generated.push_back({i + 1, "Name" + std::to_string(random() % 100),
            "Surname" + std::to_string(random() % 100), static_cast<int>(1950 + random() % 75)});
      return result;
    }, [=](const Result& result) {
      QSignalBlocker blocker(source);
      showPeople(source, result.generated);
      index->reset();
      metrics->setRowCount(0);
      output->setRowCount(0);
      metrics->setRowCount(0);
      status->setText("Набор создан — постройте индекс");
    });
  });
  auto* build = button("Построить индекс", tools, [=] {
    Array<PersonRecord> input;
    for (int r = 0; r < source->rowCount(); ++r) {
      const auto name = cell(source, r, 1).trimmed();
      const auto surname = cell(source, r, 2).trimmed();
      if (name.isEmpty() || surname.isEmpty()) throw std::invalid_argument("Имя и фамилия обязательны");
      input.push_back({integer(cell(source, r, 0)), name.toStdString(), surname.toStdString(), integer(cell(source, r, 3))});
    }
    auto snapshot = std::make_shared<Array<PersonRecord>>(std::move(input));
    background(root, status, [=] {
      Result result;
      double elapsed;
      result.index = measured([&] {
        auto built = std::make_shared<PersonIndex>();
        for (size_t i = 0; i < snapshot->size(); ++i) built->add((*snapshot)[i]);
        return built;
      }, elapsed);
      result.summary = QString("Индекс построен: %1 записей, %2 мс").arg(snapshot->size()).arg(elapsed, 0, 'f', 3);
      return result;
    }, [=](const Result& result) {
      *index = result.index;
      *people = *snapshot;
      output->setRowCount(0);
      status->setText(result.summary);
      row(metrics, {"Замер", result.summary});
    });
  });
  build->setProperty("indexPrimary", true);
  button("Точный поиск", exact, [=] {
    if (!*index) throw std::invalid_argument("Сначала постройте индекс");
    auto current = *index;
    const auto f = first->text().toStdString(), l = last->text().toStdString();
    const int y = year->value();
    background(root, status, [=] {
      Result result;
      double elapsed;
      result.matches = measured([&] { return current->findComposite(f, l, y); }, elapsed);
      result.summary = QString("Найдено: %1; %2 мс").arg(result.matches.size()).arg(elapsed, 0, 'f', 6);
      return result;
    }, [=](const Result& result) { showPeople(output, result.matches); status->setText(result.summary);
      row(metrics, {"Замер", result.summary}); });
  });
  button("Найти диапазон", range, [=] {
    if (!*index) throw std::invalid_argument("Сначала постройте индекс");
    const int a = from->value(), b = to->value(), algorithm = mode->currentIndex();
    if (a > b) throw std::invalid_argument("Начало диапазона больше конца");
    auto current = *index;
    background(root, status, [=] {
      Result result;
      Array<PersonRecord> indexed, scanned;
      double indexTime = 0, scanTime = 0;
      if (algorithm != 1) indexed = measured([&] { return current->findBirthYearRange(a, b); }, indexTime);
      if (algorithm != 0) scanned = measured([&] {
        Array<PersonRecord> matches;
  Array<PersonRecord> generated;
        for (size_t i = 0; i < people->size(); ++i)
          if ((*people)[i].birthYear >= a && (*people)[i].birthYear <= b) matches.push_back((*people)[i]);
        return matches;
      }, scanTime);
      result.matches = algorithm == 1 ? scanned : indexed;
      QStringList details;
      if (algorithm != 1) details << QString("Индекс: %1 мс").arg(indexTime, 0, 'f', 6);
      if (algorithm != 0) details << QString("Просмотр: %1 мс").arg(scanTime, 0, 'f', 6);
      if (algorithm == 2) {
        details << (samePersonRecords(indexed, scanned) ? "Ответы совпадают" : "Ответы различаются");
      }
      result.summary = QString("Найдено: %1; ").arg(result.matches.size()) + details.join("; ");
      return result;
    }, [=](const Result& result) { showPeople(output, result.matches); status->setText(result.summary);
      row(metrics, {"Замер", result.summary}); });
  });
  button("Экспорт результата", resultBar, [=] { exportTable(root, output); });
  return root;
}
