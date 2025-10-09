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

    auto label = new QLabel("PDF pixel density (PPI)");
    auto control_container = create_control_with_info(
        style, options->pdf_pixel_density_spin_box, PDF_TOOLTIP
    );

    options->settings_layout->addRow(label, control_container);
}

void add_convert_to_greyscale_widget(QStyle *style, Options *options) {
    auto label = new QLabel("Convert pages to greyscale");
    options->convert_to_greyscale = new QCheckBox("Enable");
    options->convert_to_greyscale->setChecked(true);
    auto control_container = create_control_with_info(
        style, options->convert_to_greyscale, GREYSCALE_TOOLTIP
    );

    options->settings_layout->addRow(label, control_container);
}

void add_double_page_spread_widget(QStyle *style, Options *options) {
    auto label = new QLabel("Double-page spread actions");
    options->double_page_spread_combo_box = create_combo_box(
        {"Rotate page", "Split into two pages", "Both", "None"}, "Rotate page"
    );
    auto control_container = create_control_with_info(
        style, options->double_page_spread_combo_box, DOUBLE_PAGE_SPREAD_TOOLTIP
    );

    options->settings_layout->addRow(label, control_container);

    // Rotation options
    options->rotation_options_container = new QWidget();
    auto rotation_layout = new QFormLayout(options->rotation_options_container);
    rotation_layout->setContentsMargins(20, 0, 0, 0);
    rotation_layout->setHorizontalSpacing(10);
    rotation_layout->setLabelAlignment(Qt::AlignLeft);

    auto rotation_label = new QLabel("Rotation direction");
    options->rotation_direction_combo_box = new QComboBox();
    options->rotation_direction_combo_box->addItems(
        {"Clockwise", "Counterclockwise"}
    );

    rotation_layout->addRow(
        rotation_label, options->rotation_direction_combo_box
    );

    options->settings_layout->addWidget(options->rotation_options_container);
}

void add_remove_spine_widget(QStyle *style, Options *options) {
    auto label = new QLabel("Remove spines");
    options->remove_spine_check_box = new QCheckBox("Enable");
    options->remove_spine_check_box->setChecked(true);
    auto control_container = create_control_with_info(
        style, options->remove_spine_check_box, REMOVE_SPINE_TOOLTIP
    );

    options->settings_layout->addRow(label, control_container);
}

void add_contrast_widget(QStyle *style, Options *options) {
    auto label = new QLabel("Stretch contrast");
    options->contrast_check_box = new QCheckBox("Enable");
    options->contrast_check_box->setChecked(true);
    auto control_container = create_control_with_info(
        style, options->contrast_check_box, CONTRAST_TOOLTIP
    );

    options->settings_layout->addRow(label, control_container);
}

void add_scaling_widgets(QStyle *style, Options *options) {
    auto label = new QLabel("Scale pages");
    options->enable_image_scaling_check_box = new QCheckBox("Enable");
    auto enable_container = create_control_with_info(
        style, options->enable_image_scaling_check_box, SCALE_TOOLTIP
    );

    options->settings_layout->addRow(label, enable_container);

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
    auto resampler_label = new QLabel("Resampler");
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
    auto resampler_container = create_control_with_info(
        style, options->resampler_combo_box, RESAMPLER_TOOLTIP
    );
    scaling_layout->addRow(resampler_label, resampler_container);

    options->settings_layout->addWidget(options->scaling_options_container);
}

void add_quantization_widgets(QStyle *style, Options *options) {
    auto label = new QLabel("Quantize pages");
    options->enable_image_quantization_check_box = new QCheckBox("Enable");
    options->enable_image_quantization_check_box->setChecked(true);
    auto enable_container = create_control_with_info(
        style, options->enable_image_quantization_check_box, QUANTIZE_TOOLTIP
    );

    options->settings_layout->addRow(label, enable_container);

    options->quantization_options_container = new QWidget();
    auto quantization_layout
        = new QFormLayout(options->quantization_options_container);
    quantization_layout->setContentsMargins(20, 0, 0, 0);
    quantization_layout->setHorizontalSpacing(10);
    quantization_layout->setLabelAlignment(Qt::AlignLeft);

    // Bit depth
    auto bit_depth_label = new QLabel("Bit depth");
    options->bit_depth_combo_box = new QComboBox();
    options->bit_depth_combo_box->addItems({"1", "2", "4", "8", "16"});
    options->bit_depth_combo_box->setCurrentText("4");
    auto bit_depth_container = create_control_with_info(
        style, options->bit_depth_combo_box, BIT_DEPTH_TOOLTIP
    );
    quantization_layout->addRow(bit_depth_label, bit_depth_container);

    // Dithering
    auto dithering_label = new QLabel("Dithering");
    options->dithering_spin_box = new QDoubleSpinBox();
    options->dithering_spin_box->setRange(0.0, 1.0);
    options->dithering_spin_box->setSingleStep(0.1);
    options->dithering_spin_box->setValue(1.0);
    auto dithering_container = create_control_with_info(
        style, options->dithering_spin_box, DITHERING_TOOLTIP
    );
    quantization_layout->addRow(dithering_label, dithering_container);

    options->settings_layout->addWidget(
        options->quantization_options_container
    );
}

void add_image_format_widgets(QStyle *style, Options *options) {
    options->image_format_combo_box
        = create_combo_box({"AVIF", "JPEG", "JPEG XL", "PNG", "WebP"}, "PNG");
    auto image_format_label = new QLabel("Image format");
    auto image_format_container = create_control_with_info(
        style, options->image_format_combo_box, IMG_FORMAT_TOOLTIP
    );

    options->settings_layout->addRow(
        image_format_label, image_format_container
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

void add_parallel_workers_widget(QStyle *, Options *options) {
    auto label = new QLabel("Parallel workers");

    options->workers_spin_box = new QSpinBox();
    auto threads = std::thread::hardware_concurrency();
    options->workers_spin_box->setRange(1, threads);
    options->workers_spin_box->setValue(threads);

    options->settings_layout->addRow(label, options->workers_spin_box);
}
