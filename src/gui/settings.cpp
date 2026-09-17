#include <QCheckBox>
#include <QComboBox>

#include "include/settings.hpp"
#include "include/window_util.hpp"

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
    return static_cast<int>(check_box->checkState());
}
void set_widget_value_from_settings(
    QAnyStringView field, QCheckBox *check_box
) {
    with_setting(field, [&](const QVariant &variant) {
        auto state = static_cast<Qt::CheckState>(variant.toInt());
        check_box->setCheckState(state);
    });
}

QVariant settings_value_from_widget(const QSpinBox *spin_box) {
    return spin_box->value();
}
void set_widget_value_from_settings(QAnyStringView field, QSpinBox *spin_box) {
    with_setting(field, [&](const QVariant &variant) {
        spin_box->setValue(variant.toInt());
    });
}

QVariant settings_value_from_widget(const QDoubleSpinBox *spin_box) {
    return spin_box->value();
}
void set_widget_value_from_settings(
    QAnyStringView field, QDoubleSpinBox *spin_box
) {
    with_setting(field, [&](const QVariant &variant) {
        spin_box->setValue(variant.toDouble());
    });
}
