#include "include/options.hpp"
#include "include/ui_constants.hpp"
#include "include/window_util.hpp"
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSizePolicy>
#include <QSpinBox>
#include <QStyle>
#include <QWidget>
#include <thread>

void add_pdf_pixel_density_widget(QStyle *style, Options *options) {
    options->pdf_pixel_density_spin_box
        = create_spin_box(300, 4'800, 300, 1'200);

    auto pdf_pixel_density_widget = create_widget_with_info(
        style, new QLabel("PDF pixel density (PPI)"), PDF_TOOLTIP
    );

    options->settings_layout->addRow(
        pdf_pixel_density_widget, options->pdf_pixel_density_spin_box
    );
}

void add_convert_to_greyscale_widget(QStyle *style, Options *options) {
    auto label_widget = create_widget_with_info(
        style, new QLabel("Convert pages to greyscale"), GREYSCALE_TOOLTIP
    );
    options->convert_to_greyscale = new QCheckBox("Enable");
    options->convert_to_greyscale->setChecked(true);

    options->settings_layout->addRow(
        label_widget, options->convert_to_greyscale
    );
}

void add_double_page_spread_widget(QStyle *style, Options *options) {
    auto widget = create_widget_with_info(
        style,
        new QLabel("Double-page spread actions"),
        DOUBLE_PAGE_SPREAD_TOOLTIP
    );
    options->double_page_spread_combo_box = create_combo_box(
        {"Rotate page", "Split into two pages", "Both", "None"}, "Rotate page"
    );

    options->settings_layout->addRow(
        widget, options->double_page_spread_combo_box
    );

    // Rotation options
    options->rotation_options_container = new QWidget();
    auto rotation_layout = new QFormLayout(options->rotation_options_container);
    rotation_layout->setContentsMargins(20, 0, 0, 0);
    rotation_layout->setHorizontalSpacing(10);
    rotation_layout->setLabelAlignment(Qt::AlignLeft);

    auto rotation_label = new QLabel("Rotation direction");

    auto radio_group_layout = new QHBoxLayout();
    options->clockwise_radio = new QRadioButton("Clockwise");
    options->counter_clockwise_radio = new QRadioButton("Counterclockwise");
    options->clockwise_radio->setChecked(true);
    radio_group_layout->addWidget(options->clockwise_radio);
    radio_group_layout->addWidget(options->counter_clockwise_radio);
    radio_group_layout->addStretch();

    auto radio_widget = new QWidget();
    radio_widget->setLayout(radio_group_layout);

    rotation_layout->addRow(rotation_label, radio_widget);

    options->settings_layout->addWidget(options->rotation_options_container);
}

void add_remove_spine_widget(QStyle *style, Options *options) {
    auto label_widget = create_widget_with_info(
        style, new QLabel("Remove spines"), REMOVE_SPINE_TOOLTIP
    );
    options->remove_spine_check_box = new QCheckBox("Enable");
    options->remove_spine_check_box->setChecked(true);

    options->settings_layout->addRow(
        label_widget, options->remove_spine_check_box
    );
}

void add_contrast_widget(QStyle *style, Options *options) {
    auto label_widget = create_widget_with_info(
        style, new QLabel("Stretch contrast"), CONTRAST_TOOLTIP
    );
    options->contrast_check_box = new QCheckBox("Enable");
    options->contrast_check_box->setChecked(true);

    options->settings_layout->addRow(label_widget, options->contrast_check_box);
}

void add_scaling_widgets(QStyle *style, Options *options) {
    auto label_widget = create_widget_with_info(
        style, new QLabel("Scale pages"), SCALE_TOOLTIP
    );
    options->enable_image_scaling_check_box = new QCheckBox("Enable");

    options->settings_layout->addRow(
        label_widget, options->enable_image_scaling_check_box
    );

    options->scaling_options_container = new QWidget();
    auto scaling_layout = new QFormLayout(options->scaling_options_container);
    scaling_layout->setContentsMargins(20, 0, 0, 0);
    scaling_layout->setHorizontalSpacing(10);
    scaling_layout->setLabelAlignment(Qt::AlignLeft);

    // Width
    options->width_spin_box = new QSpinBox();
    options->width_spin_box->setRange(100, 4'000);
    options->width_spin_box->setSingleStep(100);
    options->width_spin_box->setValue(1440);
    auto width_label = new QLabel("Width");
    scaling_layout->addRow(width_label, options->width_spin_box);

    // Height
    options->height_spin_box = new QSpinBox();
    options->height_spin_box->setRange(100, 4'000);
    options->height_spin_box->setSingleStep(100);
    options->height_spin_box->setValue(1920);
    auto height_label = new QLabel("Height");
    scaling_layout->addRow(height_label, options->height_spin_box);

    // Resampler
    auto resampler_label_with_info = create_widget_with_info(
        style, new QLabel("Resampler"), RESAMPLER_TOOLTIP
    );
    options->resampler_combo_box = new QComboBox();
    options->resampler_combo_box->addItems(
        {"Bicubic interpolation",
         "Bilinear interpolation",
         "Lanczos 2",
         "Lanczos 3",
         "Magic Kernel Sharp 2013",
         "Magic Kernel Sharp 2021",
         "Mitchell",
         "Nearest neighbour"}
    );
    options->resampler_combo_box->setCurrentText("Magic Kernel Sharp 2021");
    scaling_layout->addRow(
        resampler_label_with_info, options->resampler_combo_box
    );

    options->settings_layout->addWidget(options->scaling_options_container);
}

void add_quantization_widgets(QStyle *style, Options *options) {
    auto label_widget = create_widget_with_info(
        style, new QLabel("Quantize pages"), QUANTIZE_TOOLTIP
    );
    options->enable_image_quantization_check_box = new QCheckBox("Enable");
    options->enable_image_quantization_check_box->setChecked(true);

    options->settings_layout->addRow(
        label_widget, options->enable_image_quantization_check_box
    );

    options->quantization_options_container = new QWidget();
    auto quantization_layout
        = new QFormLayout(options->quantization_options_container);
    quantization_layout->setContentsMargins(20, 0, 0, 0);
    quantization_layout->setHorizontalSpacing(10);
    quantization_layout->setLabelAlignment(Qt::AlignLeft);

    // Bit depth
    auto bit_depth_label = create_widget_with_info(
        style, new QLabel("Bit depth"), BIT_DEPTH_TOOLTIP
    );
    options->bit_depth_combo_box = new QComboBox();
    options->bit_depth_combo_box->addItems({"1", "2", "4", "8", "16"});
    options->bit_depth_combo_box->setCurrentText("4");
    quantization_layout->addRow(bit_depth_label, options->bit_depth_combo_box);

    // Dithering
    auto dithering_label = create_widget_with_info(
        style, new QLabel("Dithering"), DITHERING_TOOLTIP
    );
    options->dithering_spin_box = new QDoubleSpinBox();
    options->dithering_spin_box->setRange(0.0, 1.0);
    options->dithering_spin_box->setSingleStep(0.1);
    options->dithering_spin_box->setValue(1.0);
    quantization_layout->addRow(dithering_label, options->dithering_spin_box);

    options->settings_layout->addWidget(
        options->quantization_options_container
    );
}

void add_image_format_widgets(QStyle *style, Options *options) {
    options->image_format_combo_box
        = create_combo_box({"AVIF", "JPEG", "JPEG XL", "PNG", "WebP"}, "PNG");
    auto image_format_label = create_widget_with_info(
        style, new QLabel("Image format"), IMG_FORMAT_TOOLTIP
    );

    options->settings_layout->addRow(
        image_format_label, options->image_format_combo_box
    );

    auto image_format_options_container = new QWidget();
    auto image_format_layout = new QFormLayout(image_format_options_container);
    image_format_layout->setContentsMargins(20, 0, 0, 0);
    image_format_layout->setHorizontalSpacing(10);
    image_format_layout->setLabelAlignment(Qt::AlignLeft);

    // Compression type
    options->image_compression_type_label = new QLabel("Compression type");
    options->image_compression_type_combo_box = new QComboBox();
    options->image_compression_type_combo_box->addItems({"Lossless", "Lossy"});
    options->image_compression_type_combo_box->setCurrentText("Lossless");
    image_format_layout->addRow(
        options->image_compression_type_label,
        options->image_compression_type_combo_box
    );
    options->image_compression_type_label->setVisible(false);
    options->image_compression_type_combo_box->setVisible(false);

    // Compression effort
    options->image_compression_label = new QLabel("Compression effort");
    options->image_compression_spin_box = new QSpinBox();
    options->image_compression_spin_box->setRange(0, 9);
    options->image_compression_spin_box->setSingleStep(1);
    options->image_compression_spin_box->setValue(6);
    image_format_layout->addRow(
        options->image_compression_label, options->image_compression_spin_box
    );

    // Quality label container (for dynamic switching)
    auto quality_label_container = new QWidget();
    auto quality_label_hbox = new QHBoxLayout(quality_label_container);
    quality_label_hbox->setContentsMargins(0, 0, 0, 0);
    quality_label_hbox->setSpacing(0);

    options->image_quality_label_original = new QLabel("Quality");
    options->image_quality_label_jpeg_xl = new QComboBox();
    options->image_quality_label_jpeg_xl->addItems({"Distance", "Quality"});
    options->image_quality_label_jpeg_xl->setCurrentText("Distance");
    options->image_quality_label_jpeg_xl->setVisible(false);

    quality_label_hbox->addWidget(options->image_quality_label_original);
    quality_label_hbox->addWidget(options->image_quality_label_jpeg_xl);

    // Quality spin box
    options->image_quality_spin_box = new QDoubleSpinBox();
    options->image_quality_spin_box->setRange(0, 100);
    options->image_quality_spin_box->setSingleStep(1);
    options->image_quality_spin_box->setValue(50);
    options->image_quality_spin_box->setDecimals(0);
    options->image_quality_spin_box->setVisible(false);
    options->image_quality_label = options->image_quality_label_original;
    options->image_quality_label->setVisible(false); // Initial hidden state

    image_format_layout->addRow(
        quality_label_container, options->image_quality_spin_box
    );

    options->settings_layout->addWidget(image_format_options_container);
}

void add_parallel_workers_widget(QStyle *style, Options *options) {
    auto label
        = create_widget_with_info(style, new QLabel("Parallel workers"), "");

    options->workers_spin_box = new QSpinBox();
    auto threads = std::thread::hardware_concurrency();
    options->workers_spin_box->setRange(1, threads);
    options->workers_spin_box->setValue(threads);

    options->settings_layout->addRow(label, options->workers_spin_box);
}
