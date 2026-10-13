#include <QCheckBox>
#include <QComboBox>

#include "include/settings.hpp"
#include "include/window_util.hpp"

template <>
std::optional<bool> from_settings_value(const QVariant &variant) {
    // Values read from the INI file are strings, but values written earlier in
    // the same session are still `bool`.
    if (variant.typeId() == QMetaType::Bool) {
        return variant.toBool();
    }
    auto text = variant.toString();
    if (text == "true") {
        return true;
    }
    if (text == "false") {
        return false;
    }
    return std::nullopt;
}

template <>
std::optional<int> from_settings_value(const QVariant &variant) {
    auto ok = false;
    auto value = variant.toInt(&ok);
    return ok ? std::optional{value} : std::nullopt;
}

template <>
std::optional<double> from_settings_value(const QVariant &variant) {
    auto ok = false;
    auto value = variant.toDouble(&ok);
    return ok ? std::optional{value} : std::nullopt;
}

QVariant settings_value_from_widget(const QComboBox *combo_box) {
    return combo_box->currentData(SETTINGS_KEY_ROLE);
}
void set_widget_value_from_settings(
    QAnyStringView field, QComboBox *combo_box
) {
    with_setting(field, [&](const QVariant &variant) {
        auto index = combo_box->findData(variant, SETTINGS_KEY_ROLE);
        if (index >= 0) {
            combo_box->setCurrentIndex(index);
        }
    });
}

QVariant settings_value_from_widget(const QCheckBox *check_box) {
    return check_box->isChecked();
}
void set_widget_value_from_settings(
    QAnyStringView field, QCheckBox *check_box
) {
    with_setting<bool>(field, [&](bool is_checked) {
        check_box->setChecked(is_checked);
    });
}

QVariant settings_value_from_widget(const QSpinBox *spin_box) {
    return spin_box->value();
}
void set_widget_value_from_settings(QAnyStringView field, QSpinBox *spin_box) {
    with_setting<int>(field, [&](int value) { spin_box->setValue(value); });
}

QVariant settings_value_from_widget(const QDoubleSpinBox *spin_box) {
    return spin_box->value();
}
void set_widget_value_from_settings(
    QAnyStringView field, QDoubleSpinBox *spin_box
) {
    with_setting<double>(field, [&](double value) {
        spin_box->setValue(value);
    });
}
