#include "include/options.hpp"
#include "include/ui_constants.hpp"
#include "include/window_util.hpp"
#include <QFormLayout>
#include <QLabel>
#include <QSpinBox>
#include <QStyle>
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
    auto rotation_layout
        = create_container_layout(options->rotation_options_container);

    auto rotation_label = new QLabel("Rotation direction");
    rotation_layout->addWidget(rotation_label);

    auto radio_group_layout = new QHBoxLayout();
    options->clockwise_radio = new QRadioButton("Clockwise");
    options->counter_clockwise_radio = new QRadioButton("Counterclockwise");
    options->clockwise_radio->setChecked(true);
    radio_group_layout->addWidget(options->clockwise_radio);
    radio_group_layout->addWidget(options->counter_clockwise_radio);
    radio_group_layout->addStretch();

    rotation_layout->addLayout(radio_group_layout);

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
    auto scaling_layout
        = create_container_layout(options->scaling_options_container);

    options->width_spin_box = create_spin_box_with_label(
        scaling_layout, new QLabel("Width"), 100, 4'000, 100, 1440
    );
    options->height_spin_box = create_spin_box_with_label(
        scaling_layout, new QLabel("Height"), 100, 4'000, 100, 1920
    );

    // Resampler
    auto resampler_label_with_info = create_widget_with_info(
        style, new QLabel("Resampler"), RESAMPLER_TOOLTIP
    );
    options->resampler_combo_box = create_combo_box_with_layout(
        scaling_layout,
        resampler_label_with_info,
        {"Bicubic interpolation",
         "Bilinear interpolation",
         "Lanczos 2",
         "Lanczos 3",
         "Magic Kernel Sharp 2013",
         "Magic Kernel Sharp 2021",
         "Mitchell",
         "Nearest neighbour"},
        "Magic Kernel Sharp 2021"
    );

    scaling_layout->addStretch();

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
        = create_container_layout(options->quantization_options_container);

    auto bit_depth_label = create_widget_with_info(
        style, new QLabel("Bit depth"), BIT_DEPTH_TOOLTIP
    );
    options->bit_depth_combo_box = create_combo_box_with_layout(
        quantization_layout, bit_depth_label, {"1", "2", "4", "8", "16"}, "4"
    );

    auto dithering_label = create_widget_with_info(
        style, new QLabel("Dithering"), DITHERING_TOOLTIP
    );
    options->dithering_spin_box = create_double_spin_box(
        quantization_layout, dithering_label, 0.0, 1.0, 0.1, 1.0
    );

    quantization_layout->addStretch();

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
    auto image_format_layout
        = create_container_layout(image_format_options_container);

    options->image_compression_type_label = new QLabel("Compression type");
    options->image_compression_type_combo_box = create_combo_box_with_layout(
        image_format_layout,
        options->image_compression_type_label,
        {"Lossless", "Lossy"},
        "Lossless"
    );
    options->image_compression_type_label->setVisible(false);
    options->image_compression_type_combo_box->setVisible(false);

    options->image_compression_label = new QLabel("Compression effort");
    options->image_compression_spin_box = create_spin_box_with_label(
        image_format_layout, options->image_compression_label, 0, 9, 1, 6
    );

    options->image_quality_label_original = new QLabel("Quality");
    options->image_quality_label_jpeg_xl
        = create_combo_box({"Distance", "Quality"}, "Distance");
    options->image_quality_label_jpeg_xl->setVisible(false);
    image_format_layout->addWidget(options->image_quality_label_jpeg_xl);
    options->image_quality_label = options->image_quality_label_original;

    options->image_quality_spin_box = create_double_spin_box(
        image_format_layout, options->image_quality_label, 0, 100, 1, 50
    );
    options->image_quality_label->setVisible(false);
    options->image_quality_spin_box->setDecimals(0);
    options->image_quality_spin_box->setVisible(false);

    image_format_layout->addStretch();

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
