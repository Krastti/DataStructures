#include "Common.h"
#include "SecondLab/PersonIndex.h"
#include <algorithm>
#include <memory>
#include <random>
#include <vector>

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
  auto* layout = new QVBoxLayout(root);
  auto* tools = new QHBoxLayout;
  auto* count = number(1, 100000, 100);
  auto* seed = number(0, 1000000, 1);
  tools->addWidget(new QLabel("Размер:"));
  tools->addWidget(count);
  tools->addWidget(new QLabel("Seed:"));
  tools->addWidget(seed);
  layout->addLayout(tools);
  auto* sourceTools = new QHBoxLayout;
  layout->addLayout(sourceTools);
  auto* source = table({"ID", "Имя", "Фамилия", "Год"}, true);
  source->setObjectName("peopleSource");
  layout->addWidget(new QLabel("Исходные записи"));
  layout->addWidget(source);
  auto* exact = new QHBoxLayout;
  auto* first = new QLineEdit;
  first->setPlaceholderText("Имя");
  auto* last = new QLineEdit;
  last->setPlaceholderText("Фамилия");
  auto* year = number(-10000, 10000, 2000);
  exact->addWidget(first);
  exact->addWidget(last);
  exact->addWidget(year);
  layout->addLayout(exact);
  auto* range = new QHBoxLayout;
  auto* from = number(-10000, 10000, 1980);
  auto* to = number(-10000, 10000, 2000);
  auto* mode = new QComboBox;
  mode->addItems({"Индекс", "Полный просмотр", "Сравнить"});
  range->addWidget(new QLabel("Годы от"));
  range->addWidget(from);
  range->addWidget(new QLabel("до"));
  range->addWidget(to);
  range->addWidget(mode);
  layout->addLayout(range);
  auto* output = table({"ID", "Имя", "Фамилия", "Год"});
  output->setObjectName("peopleResults");
  layout->addWidget(new QLabel("Результат поиска"));
  layout->addWidget(output);
  auto* status = new QLabel("Добавьте записи или сгенерируйте набор");
  status->setWordWrap(true);
  status->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
  status->setObjectName("status");
  layout->addWidget(status);
  auto* metrics = table({"Операция", "Результат и время"});
  metrics->setMaximumHeight(110);
  layout->addWidget(metrics);
  button("Экспорт замеров", sourceTools, [=] { exportTable(root, metrics); });
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
  button("Построить индекс", sourceTools, [=] {
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
        using Key = std::tuple<int, std::string, std::string, int>;
        auto keys = [](const Array<PersonRecord>& records) {
          std::vector<Key> result;
          for (size_t i = 0; i < records.size(); ++i) {
            const auto& p = records[i];
            result.emplace_back(p.id, p.firstName, p.lastName, p.birthYear);
          }
          std::sort(result.begin(), result.end());
          return result;
        };
        details << (keys(indexed) == keys(scanned) ? "Ответы совпадают" : "Ответы различаются");
      }
      result.summary = QString("Найдено: %1; ").arg(result.matches.size()) + details.join("; ");
      return result;
    }, [=](const Result& result) { showPeople(output, result.matches); status->setText(result.summary);
      row(metrics, {"Замер", result.summary}); });
  });
  button("Экспорт результата", sourceTools, [=] { exportTable(root, output); });
  return root;
}
