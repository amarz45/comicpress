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

template <typename Box = QSpinBox>
struct SpinBoxParams {
    using Value = decltype(std::declval<Box>().value());
    std::pair<Value, Value> range;
    Value step;
    Value value;
};

template <typename Box = QSpinBox>
Box *create_spin_box(const SpinBoxParams<Box> &params, auto &&...args) {
    auto *box = new Box(std::forward<decltype(args)>(args)...);
    box->setRange(params.range.first, params.range.second);
    box->setSingleStep(params.step);
    box->setValue(params.value);
    box->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    return box;
}

template <typename Box>
Box *create_spin_box(auto &&...args) {
    auto *box = new Box(std::forward<decltype(args)>(args)...);
    box->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    return box;
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
