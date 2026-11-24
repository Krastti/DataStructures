#pragma once

#include <QComboBox>
#include <QFrame>
#include <QLabel>
#include <QLayout>
#include <QPainter>
#include <QSpinBox>
#include <QStyleOptionComboBox>
#include <QStyleOptionSpinBox>
#include <QTableWidget>

namespace indexAppearance {

class NumberInput : public QSpinBox {
protected:
  void paintEvent(QPaintEvent* event) override {
    QSpinBox::paintEvent(event);
    QStyleOptionSpinBox option;
    initStyleOption(&option);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    drawArrow(painter, option, QStyle::SC_SpinBoxUp, QAbstractSpinBox::StepUpEnabled, -1);
    drawArrow(painter, option, QStyle::SC_SpinBoxDown, QAbstractSpinBox::StepDownEnabled, 1);
  }

private:
  void drawArrow(QPainter& painter, const QStyleOptionSpinBox& option, QStyle::SubControl control,
                 StepEnabledFlag enabled, int direction) const {
    const QPoint center = style()->subControlRect(QStyle::CC_SpinBox, &option, control, this).center();
    const QColor color(isEnabled() && stepEnabled().testFlag(enabled) ? "#f2f2f2" : "#777777");
    painter.setPen(QPen(color, 1.3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPolyline(QPolygon{center + QPoint(-3, -2 * direction),
                                  center + QPoint(0, direction),
                                  center + QPoint(3, -2 * direction)});
  }
};

inline QSpinBox* number(int minimum, int maximum, int value) {
  auto* box = new NumberInput;
  box->setRange(minimum, maximum);
  box->setValue(value);
  return box;
}

class SearchModeInput : public QComboBox {
protected:
  void paintEvent(QPaintEvent* event) override {
    QComboBox::paintEvent(event);
    QStyleOptionComboBox option;
    initStyleOption(&option);
    const QPoint center = style()->subControlRect(
        QStyle::CC_ComboBox, &option, QStyle::SC_ComboBoxArrow, this).center();
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor(isEnabled() ? "#f2f2f2" : "#777777"), 1.3,
                       Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPolyline(QPolygon{center + QPoint(-4, -2), center + QPoint(0, 2),
                                  center + QPoint(4, -2)});
  }
};

inline void stylePage(QWidget* page) {
  page->setObjectName("indexPage");
  page->setAttribute(Qt::WA_StyledBackground);
  auto palette = page->palette();
  palette.setColor(QPalette::PlaceholderText, QColor("#bcbcbc"));
  page->setPalette(palette);
  page->setStyleSheet(R"(
    QWidget#indexPage { background: #1d1d1d; color: #f2f2f2; }
    QWidget#indexPage QWidget { font-family: Arial; font-size: 14px; }
    QLabel { color: #f2f2f2; }
    QLabel[indexHeading="true"] { font-size: 16px; font-weight: bold; }
    QLabel#status { color: #c4c4c4; padding: 3px 0; }
    QFrame[indexDivider="true"] { background: #474747; }
    QSplitter::handle { background: #474747; }
    QSplitter::handle:hover { background: #18ddec; }
    QPushButton, QLineEdit, QSpinBox, QComboBox, QPlainTextEdit, QTableWidget {
      background: #262626; color: #f2f2f2;
      border: 1px solid #414141; border-radius: 10px;
      selection-background-color: #246068; selection-color: #f2f2f2;
    }
    QPushButton, QLineEdit, QSpinBox, QComboBox { min-height: 28px; padding: 2px 10px; }
    QSpinBox, QComboBox { padding-right: 24px; }
    QSpinBox QLineEdit { border: none; padding: 0; }
    QSpinBox::up-button, QSpinBox::down-button { width: 22px; border: none; }
    QSpinBox::up-arrow, QSpinBox::down-arrow { width: 0; height: 0; }
    QComboBox::drop-down { width: 22px; border: none; }
    QComboBox::down-arrow { width: 0; height: 0; }
    QPlainTextEdit { padding: 6px; }
    QLineEdit, QSpinBox, QPlainTextEdit, QTableWidget { background: #212121; }
    QPushButton:hover { background: #323232; }
    QPushButton:pressed { background: #393939; }
    QPushButton:focus, QLineEdit:focus, QSpinBox:focus, QComboBox:focus,
    QPlainTextEdit:focus, QTableWidget:focus { border-color: #18ddec; }
    QPushButton[indexPrimary="true"] {
      background: #1bdceb; color: #101a1b; border-color: #11c5d4; font-weight: bold;
    }
    QPushButton[indexPrimary="true"]:hover { background: #5ceaf4; }
    QPushButton[indexPrimary="true"]:pressed { background: #14bbd1; }
    QPushButton[indexPrimary="true"]:focus { border-color: #f2f2f2; }
    QPushButton:disabled, QPushButton[indexPrimary="true"]:disabled,
    QLineEdit:disabled, QSpinBox:disabled, QComboBox:disabled {
      background: #262626; color: #969696; border-color: #414141;
    }
    QComboBox QAbstractItemView {
      background: #262626; color: #f2f2f2; border: 1px solid #474747;
      selection-background-color: #246068; selection-color: #f2f2f2;
    }
    QTableWidget {
      alternate-background-color: #272727; gridline-color: #414141;
    }
    QTableCornerButton::section, QHeaderView, QHeaderView::section {
      background: #2c2c2c; color: #f2f2f2; border: none;
    }
    QHeaderView::section { padding: 5px 8px; border-bottom: 1px solid #474747; }
    QScrollBar { background: #212121; border: none; }
    QScrollBar:vertical { width: 10px; }
    QScrollBar:horizontal { height: 10px; }
    QScrollBar::handle { background: #575757; border-radius: 4px; min-height: 20px; min-width: 20px; }
    QScrollBar::handle:hover { background: #777777; }
    QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }
    QScrollBar::add-page, QScrollBar::sub-page { background: none; }
  )");
}

inline void heading(QLayout* layout, const QString& text) {
  auto* label = new QLabel(text);
  label->setProperty("indexHeading", true);
  layout->addWidget(label);
}

inline QFrame* divider(Qt::Orientation orientation) {
  auto* line = new QFrame;
  line->setProperty("indexDivider", true);
  if (orientation == Qt::Horizontal) {
    line->setFixedHeight(1);
  } else {
    line->setFixedWidth(1);
  }
  return line;
}

inline void styleTable(QTableWidget* table) {
  table->setAlternatingRowColors(true);
  table->setShowGrid(false);
  table->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  table->verticalHeader()->hide();
  table->verticalHeader()->setDefaultSectionSize(29);
}

}
