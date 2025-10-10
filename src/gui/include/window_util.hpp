#pragma once

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <utility>

QSpinBox *create_spin_box(int lower, int upper, int step_size, int value);
QComboBox *create_combo_box(QStringList items, QString current_text);
QHBoxLayout *create_container_layout(QWidget *container);
QSpinBox *create_spin_box_with_label(
    QHBoxLayout *layout,
    QWidget *widget,
    int lower,
    int upper,
    int step_size,
    int value
);
QComboBox *create_combo_box_with_layout(
    QHBoxLayout *layout,
    QWidget *widget,
    QStringList items,
    QString current_text
);
QDoubleSpinBox *create_double_spin_box(
    QHBoxLayout *layout,
    QWidget *widget,
    double lower,
    double upper,
    double step_size,
    double value
);
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
