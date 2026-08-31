#pragma once

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <array>
#include <cstddef>
#include <utility>

template <typename T, std::size_t N>
QComboBox *create_combo_box(
    const std::array<std::pair<const char *, T>, N> &entries, T default_value
) {
    auto combo = new QComboBox();
    for (const auto &[label, value] : entries) {
        combo->addItem(label, QVariant::fromValue(value));
    }
    combo->setCurrentIndex(combo->findData(QVariant::fromValue(default_value)));
    // Fix: Prevent combo box from expanding to fill width
    combo->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    return combo;
}

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
