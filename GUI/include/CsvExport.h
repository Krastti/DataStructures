#pragma once

#include <QtWidgets>
#include <stdexcept>

inline void saveTableCsv(QTableWidget* source, const QString& path) {
  QSaveFile file(path);
  if (!file.open(QIODevice::WriteOnly)) throw std::runtime_error(file.errorString().toStdString());
  QTextStream stream(&file);
  auto escape = [](QString text) { return '"' + text.replace("\"", "\"\"") + '"'; };
  QStringList headers;
  for (int c = 0; c < source->columnCount(); ++c) headers << escape(source->horizontalHeaderItem(c)->text());
  stream << headers.join(',') << '\n';
  for (int r = 0; r < source->rowCount(); ++r) {
    QStringList values;
    for (int c = 0; c < source->columnCount(); ++c) values << escape(source->item(r, c) ? source->item(r, c)->text() : QString());
    stream << values.join(',') << '\n';
  }
  stream.flush();
  if (stream.status() != QTextStream::Ok || !file.commit()) throw std::runtime_error("Не удалось сохранить CSV");
}

inline void exportTable(QWidget* owner, QTableWidget* source) {
  const QString path = QFileDialog::getSaveFileName(owner, "Экспорт CSV", {}, "CSV (*.csv)");
  if (!path.isEmpty()) saveTableCsv(source, path);
}
