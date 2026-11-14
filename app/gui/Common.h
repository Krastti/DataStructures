#pragma once

#include <QtWidgets>
#include <QtConcurrent>
#include "../services/BenchmarkService.h"
#include "../services/CsvExport.h"
#include <string>
#include <exception>
#include <functional>
#include <stdexcept>
#include <utility>

inline QString utf(const std::string& value) {
  return QString::fromUtf8(value.data(), static_cast<qsizetype>(value.size()));
}

inline QSpinBox* number(int minimum, int maximum, int value) {
  auto* box = new QSpinBox;
  box->setRange(minimum, maximum);
  box->setValue(value);
  return box;
}

inline QPushButton* button(const QString& text, QLayout* layout, std::function<void()> action) {
  auto* result = new QPushButton(text);
  layout->addWidget(result);
  QObject::connect(result, &QPushButton::clicked, result, [result, action] {
    try { action(); }
    catch (const std::exception& error) { QMessageBox::warning(result, "Ошибка", utf(error.what())); }
  });
  return result;
}

inline QTableWidget* table(const QStringList& headers, bool editable = false) {
  auto* result = new QTableWidget(0, headers.size());
  result->setHorizontalHeaderLabels(headers);
  result->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
  if (!editable) result->setEditTriggers(QAbstractItemView::NoEditTriggers);
  result->setSelectionBehavior(QAbstractItemView::SelectRows);
  return result;
}

inline void row(QTableWidget* target, const QStringList& values) {
  const int index = target->rowCount();
  target->insertRow(index);
  for (int i = 0; i < values.size(); ++i) target->setItem(index, i, new QTableWidgetItem(values[i]));
}

inline QString cell(QTableWidget* target, int rowIndex, int column) {
  if (!target->item(rowIndex, column)) throw std::invalid_argument("Заполните все поля таблицы");
  return target->item(rowIndex, column)->text();
}

inline int integer(const QString& text) {
  bool ok = false;
  const int value = text.toInt(&ok);
  if (!ok) throw std::invalid_argument("Ожидается целое число");
  return value;
}

// На время работы блокируется ввод; виджеты обновляются только в основном потоке.
template<typename Work, typename Done>
void background(QWidget* owner, QLabel* status, Work work, Done done) {
  using Result = decltype(work());
  auto* watcher = new QFutureWatcher<Result>(owner);
  owner->setEnabled(false);
  status->setText("Выполняется…");
  QObject::connect(watcher, &QFutureWatcher<Result>::finished, owner, [=] {
    owner->setEnabled(true);
    try {
      auto result = watcher->result();
      if (!result.error.isEmpty()) throw std::runtime_error(result.error.toStdString());
      done(result);
    } catch (const std::exception& error) {
      status->setText("Ошибка");
      QMessageBox::warning(owner, "Ошибка", utf(error.what()));
    }
    watcher->deleteLater();
  });
  watcher->setFuture(QtConcurrent::run([work] {
    Result result;
    try { result = work(); }
    catch (const std::exception& error) { result.error = utf(error.what()); }
    return result;
  }));
}

QWidget* alphabeticalTab();
QWidget* personTab();
QWidget* graphTab();
