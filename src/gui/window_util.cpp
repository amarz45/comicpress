#include "include/window_util.hpp"
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QWidget>
#include <utility>

QHBoxLayout *create_container_layout(QWidget *container) {
    auto layout = new QHBoxLayout(container);
    layout->setContentsMargins(20, 0, 0, 0);
    layout->setSpacing(10);
    return layout;
}

QComboBox *create_combo_box(QStringList items, QString current_text) {
    auto combo_box = new QComboBox();
    combo_box->addItems(items);
    combo_box->setCurrentText(current_text);
    return combo_box;
}

QComboBox *create_combo_box_with_layout(
    QHBoxLayout *layout,
    QWidget *widget,
    QStringList items,
    QString current_text
) {
    layout->addWidget(widget);
    auto combo_box = create_combo_box(items, current_text);
    layout->addWidget(combo_box);
    return combo_box;
}

QDoubleSpinBox *create_double_spin_box(
    QHBoxLayout *layout,
    QWidget *widget,
    double lower,
    double upper,
    double step_size,
    double value
) {
    layout->addWidget(widget);
    auto spin_box = new QDoubleSpinBox();
    spin_box->setRange(lower, upper);
    spin_box->setSingleStep(step_size);
    spin_box->setValue(value);
    layout->addWidget(spin_box);
    return spin_box;
}

QSpinBox *create_spin_box(int lower, int upper, int step_size, int value) {
    auto spin_box = new QSpinBox();
    spin_box->setRange(lower, upper);
    spin_box->setSingleStep(step_size);
    spin_box->setValue(value);
    return spin_box;
}

QSpinBox *create_spin_box_with_label(
    QHBoxLayout *layout,
    QWidget *widget,
    int lower,
    int upper,
    int step_size,
    int value
) {
    layout->addWidget(widget);
    auto spin_box = create_spin_box(lower, upper, step_size, value);
    layout->addWidget(spin_box);
    return spin_box;
}

QWidget *create_widget_with_info(
    QStyle *style, QWidget *main_widget, const char *tooltip_text
) {
    auto container = new QWidget();
    auto layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(5);

    auto has_tooltip = tooltip_text && strlen(tooltip_text) > 0;

    if (has_tooltip && style) {
        auto info_area = new QLabel();
        info_area->setFixedSize(16, 16);

        auto icon = style->standardIcon(
            QStyle::StandardPixmap::SP_MessageBoxInformation
        );
        auto formatted_tooltip = "<p>" + std::string(tooltip_text) + "</p>";
        info_area->setPixmap(icon.pixmap(16, 16));
        info_area->setToolTip(QString::fromStdString(formatted_tooltip));

        layout->addWidget(info_area);
    }

    layout->addWidget(main_widget);

    return container;
}

std::pair<QWidget *, QLabel *> create_control_with_info_pair(
    QStyle *style, QWidget *main_widget, const char *tooltip_text
) {
    auto container = new QWidget();
    auto layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    auto has_tooltip = tooltip_text && strlen(tooltip_text) > 0;
    layout->addWidget(main_widget);

    auto info_area = new QLabel();

    if (has_tooltip && style) {
        info_area->setFixedSize(16, 16);

        auto icon = style->standardIcon(
            QStyle::StandardPixmap::SP_MessageBoxInformation
        );
        auto formatted_tooltip = "<p>" + std::string(tooltip_text) + "</p>";
        info_area->setPixmap(icon.pixmap(16, 16));
        info_area->setToolTip(QString::fromStdString(formatted_tooltip));

        layout->addWidget(info_area);
    }

    return std::make_pair(container, info_area);
}

QWidget *create_control_with_info(
    QStyle *style, QWidget *main_widget, const char *tooltip_text
) {
    return create_control_with_info_pair(style, main_widget, tooltip_text)
        .first;
}

std::string time_to_str(int64_t millis) {
    int seconds = static_cast<int>(millis / 1000);

    int h = seconds / 3600;
    int rem = seconds % 3600;
    int m = rem / 60;
    int s = rem % 60;

    std::ostringstream oss;
    if (seconds < 60) {
        oss << s << " s";
    }
    else if (seconds < 3600) {
        oss << m << " min " << std::setw(2) << std::setfill('0') << s << " s";
    }
    else {
        oss << h << " h " << std::setw(2) << std::setfill('0') << m << " min "
            << std::setw(2) << std::setfill('0') << s << " s";
    }

    return oss.str();
}
