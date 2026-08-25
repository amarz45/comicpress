#pragma once

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <utility>

QComboBox *
create_combo_box(const QStringList &items, const QString &current_text);
QWidget *create_widget_with_info(
    QStyle *style, QWidget *main_widget, const char *tooltip_text
);
std::pair<QWidget *, QLabel *> create_control_with_info_pair(
    QStyle *style, QWidget *main_widget, const char *tooltip_text
);
QWidget *create_control_with_info(
    QStyle *style, QWidget *main_widget, const char *tooltip_text
);

std::string time_to_str(int64_t milliseconds);
