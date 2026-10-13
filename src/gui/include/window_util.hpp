#pragma once

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <array>
#include <cstddef>
#include <utility>

inline constexpr auto SETTINGS_KEY_ROLE = Qt::UserRole + 1;

template <typename T>
struct ComboBoxVariant {
    std::string_view settings_key;
    std::string_view ui_label;
    T value;
};

template <typename T, std::size_t N>
QComboBox *create_combo_box(
    const std::array<ComboBoxVariant<T>, N> &entries, T default_value
) {
    auto combo = new QComboBox();
    for (const auto &[key, label, value] : entries) {
        auto key_qstr = QString::fromUtf8(key.data(), key.size());
        auto label_qstr = QString::fromUtf8(label.data(), label.size());
        combo->addItem(label_qstr, QVariant::fromValue(value));
        combo->setItemData(combo->count() - 1, key_qstr, SETTINGS_KEY_ROLE);
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
